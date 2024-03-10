#if !defined(XI_THREAD_POOL_H_)
#define XI_THREAD_POOL_H_

typedef volatile U32 Futex;

typedef struct ThreadPool ThreadPool;

#define THREAD_TASK_PROC(name) void name(ThreadPool *pool, void *arg)
typedef THREAD_TASK_PROC(ThreadTaskProc);

typedef struct ThreadTask ThreadTask;
struct ThreadTask {
    ThreadTaskProc *Proc;
    void *arg;
};

typedef struct ThreadQueue ThreadQueue;
struct ThreadQueue {
    volatile U64 head;
    volatile U64 tail;

    U32 thread_index;

    ThreadPool *pool;

    U32 task_limit;
    ThreadTask *tasks;
};

struct ThreadPool {
    Futex task_count;
    Futex tasks_available;

    U32 task_limit;  // tasks per queue, must be a power of 2
    U32 num_threads; // this includes the main thread
    ThreadQueue *queues;
};

// enqueue a task to the calling threads work queue, if proc is null this function does nothing
//
Func void ThreadPoolEnqueue(ThreadPool *pool, ThreadTaskProc *Proc, void *arg);

// wait for all tasks currently enqueued on the pool to be completed
//
Func void ThreadPoolAwaitComplete(ThreadPool *pool);

// Compiler intrinsic atomics
//
// Increment, Decrement return new value stored in 'dst'
// Others return previous value
//
// @todo: this was a mistake, should make these consistent to always return the 'old' value of dst
// leaving as is temporarily to prevent breaking stuff
//
Inline U32 U32AtomicIncrement(volatile U32 *dst);
Inline U32 U32AtomicDecrement(volatile U32 *dst);
Inline U32 U32AtomicExchange(volatile U32 *dst, U32 value);
Inline B32 U32AtomicCompareExchange(volatile U32 *dst, U32 value, U32 compare);

Inline U64 U64AtomicAdd(volatile U64 *dst, U64 amount);
Inline U64 U64AtomicExchange(volatile U64 *dst, U64 value);
Inline B32 U64AtomicCompareExchange(volatile U64 *dst, U64 value, U64 compare);

#endif  // XI_THREAD_POOL_H_
