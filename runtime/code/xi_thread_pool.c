// os threading
//
XI_INTERNAL void xi_os_thread_start(xiThreadQueue *arg);

XI_INTERNAL void xi_os_futex_wait(xiFutex *futex, xiFutex value);
XI_INTERNAL void xi_os_futex_signal(xiFutex *futex); // one thread
XI_INTERNAL void xi_os_futex_broadcast(xiFutex *futex); // all threads

// atomic intrinsics
//
#if XI_COMPILER_CL
    #define XI_FUTEX_INCREMENT(futex) _InterlockedIncrement64(futex)
    #define XI_FUTEX_DECREMENT(futex) _InterlockedDecrement64(futex)

    // @todo: these should probably be part of the public facing api
    //
    #define XI_ATOMIC_ADD_U64(target, value) _InterlockedExchangeAdd64((__int64 volatile *) target, value)

    #define XI_ATOMIC_EXCHANGE_U64(target, value) _InterlockedExchange64(target, value)
    #define XI_ATOMIC_COMPARE_EXCHANGE_U64(target, value, compare) \
                        (_InterlockedCompareExchange64(target, value, compare) == (__int64) compare)

#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #error "incomplete implementation"
#endif


XI_INTERNAL void xi_thread_queue_init(xiArena *arena, xiThreadPool *pool, xiThreadQueue *queue, xi_u32 index) {
    queue->pool = pool;
    queue->thread_index = index;

    queue->task_limit = pool->task_limit;
    queue->tasks = xi_arena_push_array(arena, xiThreadTask, queue->task_limit);

    queue->head = queue->tail = 0;
}

XI_INTERNAL void xi_thread_queue_enqueue(xiThreadQueue *queue, xiThreadTask task) {
    xiThreadPool *pool = queue->pool;

    // immediately broadcast to all threads away that there will be a new task available, it won't
    // actually be able to be stolen until we update the head on the queue however
    //
    XI_FUTEX_INCREMENT(&pool->task_count);

    // we don't have to loop on this as we know the calling thread is the only thread that is
    // allowed to enqueue to the queue
    //
    xi_u64 mask = queue->task_limit - 1;
    xi_u64 head = queue->head;
    xi_u64 next = (head + 1) & mask;

    XI_ASSERT(queue->tail != next);

    queue->tasks[head] = task;

    XI_ATOMIC_EXCHANGE_U64(&queue->head, next); // @todo: is a full exchange really needed here?

    // broadcast to all sleeping worker threads that there is a new task available so they can
    // try to steal it
    //
    XI_FUTEX_INCREMENT(&pool->tasks_available);
    xi_os_futex_broadcast(&pool->tasks_available);
}

XI_INTERNAL xi_b32 xi_thread_queue_dequeue(xiThreadQueue *queue, xiThreadTask *task) {
    xi_b32 result = true;

    xi_u64 mask = queue->task_limit - 1;
    xi_u64 tail;
    xi_u64 next;
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
    while (!XI_ATOMIC_COMPARE_EXCHANGE_U64(&queue->tail, next, tail));

    if (result) {
        *task = queue->tasks[tail];
    }

    return result;
}

XI_INTERNAL XI_THREAD_VAR xi_u32 __tls_thread_index;

XI_INTERNAL void xi_thread_run(xiThreadQueue *queue) {
    xiThreadPool *pool = queue->pool;
    __tls_thread_index = queue->thread_index;

    xiThreadTask task;
    for (;;) {
        while (xi_thread_queue_dequeue(queue, &task)) {
            task.proc(pool, task.arg);

            if (XI_FUTEX_DECREMENT(&pool->task_count) == 0) {
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
                XI_ASSERT(queue->head == queue->tail);

                xi_os_futex_signal(&pool->task_count); // wake threads in xi_thread_pool_await_complete
                break;
            }
        }

        while (pool->task_count != 0) {
            xi_u32 index = __tls_thread_index;
            for (xi_u32 it = 0; it < pool->thread_count; ++it) {
                if (pool->task_count == 0) { break; }

                index = (index + 1) % pool->thread_count;
                xiThreadQueue *other = &pool->queues[index];
                if (xi_thread_queue_dequeue(other, &task)) {
                    task.proc(pool, task.arg);

                    if (XI_FUTEX_DECREMENT(&pool->task_count) == 0) {
                        // same as comment above
                        //
                        // :wait_signal
                        //
                        xi_os_futex_signal(&pool->task_count);
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
        xiFutex count = pool->tasks_available;

        // reset the temporary arena to release any memory we used as we are no longer executing
        // any tasks so it isn't needed anymore
        //
        xi_temp_reset();

        xi_os_futex_wait(&pool->tasks_available, count);
    }
}

XI_INTERNAL void xi_thread_pool_init(xiArena *arena, xiThreadPool *pool) {
    if (pool->task_limit == 0) {
        pool->task_limit = (1 << 13);
    }
    else {
        pool->task_limit = xi_pow2_nearest_u32(pool->task_limit);
    }

    pool->queues = xi_arena_push_array(arena, xiThreadQueue, pool->thread_count);

    xi_thread_queue_init(arena, pool, &pool->queues[0], 0);

    for (xi_u32 it = 1; it < pool->thread_count; ++it) {
        xiThreadQueue *queue = &pool->queues[it];

        xi_thread_queue_init(arena, pool, queue, it);
        xi_os_thread_start(queue);
    }
}

// these are the exported api functions
//
void xi_thread_pool_enqueue(xiThreadPool *pool, xiThreadTaskProc *proc, void *arg) {
    if (proc) {
        // we don't queue anything if we aren't supplied a function to call
        //
        xiThreadQueue *queue = &pool->queues[__tls_thread_index];

        xiThreadTask task;
        task.proc = proc;
        task.arg  = arg;

        xi_thread_queue_enqueue(queue, task);
    }
}

void xi_thread_pool_await_complete(xiThreadPool *pool) {
    xiThreadQueue *queue = &pool->queues[__tls_thread_index];

    xiThreadTask task;
    while (pool->task_count != 0) {
        while (xi_thread_queue_dequeue(queue, &task)) {
            // there is work on our queue to execute
            //
            task.proc(pool, task.arg);
            XI_FUTEX_DECREMENT(&pool->task_count);
        }

        xiFutex count = pool->task_count;
        if (count != 0) {
            xi_os_futex_wait(&pool->task_count, count);
        }
    }
}
