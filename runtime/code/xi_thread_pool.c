// os threading
//
FileScope void OS_ThreadStart(ThreadQueue *arg);

FileScope void OS_FutexWait(Futex *futex, Futex value);
FileScope void OS_FutexSignal(Futex *futex); // one thread
FileScope void OS_FutexBroadcast(Futex *futex); // all threads

FileScope void ThreadQueueInit(M_Arena *arena, ThreadPool *pool, ThreadQueue *queue, U32 index) {
    queue->pool         = pool;
    queue->thread_index = index;

    queue->task_limit = pool->task_limit;
    queue->tasks      = M_ArenaPush(arena, ThreadTask, queue->task_limit);

    queue->head = queue->tail = 0;
}

FileScope void ThreadQueueEnqueue(ThreadQueue *queue, ThreadTask task) {
    ThreadPool *pool = queue->pool;

    // immediately broadcast to all threads away that there will be a new task available, it won't
    // actually be able to be stolen until we update the head on the queue however
    //
    U32AtomicIncrement(&pool->task_count);

    // XI_FUTEX_INCREMENT(&pool->task_count);

    // we don't have to loop on this as we know the calling thread is the only thread that is
    // allowed to enqueue to the queue
    //
    U64 mask = queue->task_limit - 1;
    U64 head = queue->head;
    U64 next = (head + 1) & mask;

    Assert(queue->tail != next);

    queue->tasks[head] = task;

    U64AtomicExchange(&queue->head, next); // @todo: is a full exchange really needed here?

    // broadcast to all sleeping worker threads that there is a new task available so they can
    // try to steal it
    //
    U32AtomicIncrement(&pool->tasks_available);
    OS_FutexBroadcast(&pool->tasks_available);
}

FileScope B32 ThreadQueueDequeue(ThreadQueue *queue, ThreadTask *task) {
    B32 result = true;

    U64 mask = queue->task_limit - 1;
    U64 tail;
    U64 next;

    do {
        tail = queue->tail;
        if (tail == queue->head) {
            // no work left so return false
            //
            result = false;
            break;
        }

        // multiple threads can be attempting to dequeue, whether it is the owning thread or others
        // trying to steal work, this means we have to loop on the exchange to make sure we are
        // the thread that exchanged the value
        //
        next = (tail + 1) & mask;
    }
    while (!U64AtomicCompareExchange(&queue->tail, next, tail));

    if (result) { *task = queue->tasks[tail]; }

    return result;
}

GlobalVar ThreadVar U32 __tls_thread_index;

FileScope void ThreadRun(ThreadQueue *queue) {
    ThreadPool *pool   = queue->pool;
    __tls_thread_index = queue->thread_index;

    ThreadTask task;
    for (;;) {
        while (ThreadQueueDequeue(queue, &task)) {
            task.Proc(pool, task.arg);

            if (U32AtomicDecrement(&pool->task_count) == 0) {
                // we can safely break here, as there are no tasks left in the pool. if, however,
                // in the time it takes us signal and break out the loop there are more tasks available
                // we will attempt to steal them by entering the if statement below.
                //
                // we can guarantee these new tasks are _not_ on our queue as we are here and we are
                // the only thread allowed to enqueue new tasks onto our queue, thus we have to steal
                // the new tasks anyway
                //
                // the assert below will confirm this as queue->head == queue->tail either means
                // our queue is full or our queue is empty, as the num_tasks is now zero we know it
                // should be empty
                //
                // :wait_signal
                //
                Assert(queue->head == queue->tail);

                OS_FutexSignal(&pool->task_count); // wake threads in xi_thread_pool_await_complete
                break;
            }
        }

        while (pool->task_count != 0) {
            U32 index = __tls_thread_index;

            for (U32 it = 0; it < pool->num_threads; ++it) {
                if (pool->task_count == 0) { break; }

                // Attempt to steal work from other thread queues in the pool
                //
                index = (index + 1) % pool->num_threads;
                ThreadQueue *other = &pool->queues[index];
                if (ThreadQueueDequeue(other, &task)) {
                    task.Proc(pool, task.arg);

                    if (U32AtomicDecrement(&pool->task_count) == 0) {
                        // :wait_signal
                        //
                        OS_FutexSignal(&pool->task_count);
                        break;
                    }
                }
                else {
                    // we failed to dequeue from this thread's queue so continue on and try to
                    // find another
                    //
                    continue;
                }
            }
        }

        // we retrieve the number of tasks available _before_ resetting our temporary memory as it could
        // take the os an arbitary amount of time to do that, thus there could be new work queued in the
        // time we spent resetting, however, as we are down here and assuming there are no more tasks for
        // us to execute, the thread would miss the new tasks and then sleep forever (or until something
        // else was queued), potentially delaying important work for a long time
        //
        // we could simply reset the temporary arena _after_ we wake up, however, the threads may allocate
        // large buffers (especially when importing assets) and we don't really want that unused memory
        // lying about while we're sleeping
        //
        Futex count = pool->tasks_available;

        // reset the temporary arena to release any memory we used as we are no longer executing
        // any tasks so it isn't needed anymore
        //
        M_TempReset();

        OS_FutexWait(&pool->tasks_available, count);
    }
}

FileScope void ThreadPoolInit(M_Arena *arena, ThreadPool *pool, U32 processor_count) {
    if (pool->num_threads == 0) {
        pool->num_threads = processor_count;
    }

    if (pool->task_limit == 0) {
        pool->task_limit = (1 << 13);
    }
    else {
        pool->task_limit = U32_Pow2Nearest(pool->task_limit);
    }

    pool->queues = M_ArenaPush(arena, ThreadQueue, pool->num_threads);

    // Init the 'main' thread queue
    //
    ThreadQueueInit(arena, pool, &pool->queues[0], 0);

    // Init the other worker thread queues
    //
    for (U32 it = 1; it < pool->num_threads; ++it) {
        ThreadQueue *queue = &pool->queues[it];

        ThreadQueueInit(arena, pool, queue, it);
        OS_ThreadStart(queue);
    }
}

// these are the exported api functions
//
void ThreadPoolEnqueue(ThreadPool *pool, ThreadTaskProc *Proc, void *arg) {
    if (Proc) {
        // we don't queue anything if we aren't supplied a function to call!
        //
        ThreadQueue *queue = &pool->queues[__tls_thread_index];

        ThreadTask task;
        task.Proc = Proc;
        task.arg  = arg;

        ThreadQueueEnqueue(queue, task);
    }
}

void ThreadPoolAwaitComplete(ThreadPool *pool) {
    ThreadQueue *queue = &pool->queues[__tls_thread_index];

    // @todo: As a call to this function implies the calling thread cannot proceed
    // until all work on the pool has been completed we should get it to steal work from
    // other queues if it has completed all work on its own queue
    //

    ThreadTask task;
    while (pool->task_count != 0) {
        while (ThreadQueueDequeue(queue, &task)) {
            // There is work on our queue to execute
            //
            task.Proc(pool, task.arg);
            U32AtomicDecrement(&pool->task_count);
        }

        Futex count = pool->task_count;
        if (count != 0) { OS_FutexWait(&pool->task_count, count); }
    }
}
