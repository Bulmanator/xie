#if !defined(XI_THREAD_POOL_H_)
#define XI_THREAD_POOL_H_

#if XI_COMPILER_CL
    #define XI_ATOMIC_VAR volatile
#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #define XI_ATOMIC_VAR _Atomic
#endif

#if XI_OS_WIN32
    // :note the interlocked variable access porition on msdn states that "simple reads and writes to properly
    // aligned 32-bit variables are atomic" and then states that "simple reads and writes to properly
    // aligned 64-bit variables are atomic on 64-bit windows", it is unclear whether this means _only_
    // 64-bit variables are atomic on 64-bit windows or if _both_ 32-bit and 64-bit variables are atomic
    // on 64-bit windows, to play it safe i am defining a futex to be 64-bit
    //
    typedef volatile xi_u64 xiFutex;
#elif XI_OS_LINUX
    // we are required that a futex for 'syscall(SYS_futex, ...)' be a u32
    //
    typedef _Atomic xi_u32 xiFutex;
#endif

typedef struct xiThreadPool xiThreadPool;

#define XI_THREAD_TASK_PROC(name) void name(xiThreadPool *pool, void *arg)
typedef XI_THREAD_TASK_PROC(xiThreadTaskProc);

typedef struct xiThreadTask {
    xiThreadTaskProc *proc;
    void *arg;
} xiThreadTask;

typedef struct xiThreadQueue {
    XI_ATOMIC_VAR xi_u64 head;
    XI_ATOMIC_VAR xi_u64 tail;

    xiThreadPool *pool;
    xi_u32 thread_index;

    xi_u32 task_limit;
    xiThreadTask *tasks;
} xiThreadQueue;

typedef struct xiThreadPool {
    xiFutex task_count;
    xiFutex tasks_available;

    xi_u32 task_limit;   // tasks per queue, must be a power of 2
    xi_u32 thread_count; // this includes the main thread
    xiThreadQueue *queues;
} xiThreadPool;

// enqueue a task to the calling threads work queue, if proc is null this function does nothing
//
extern XI_API void xi_thread_pool_enqueue(xiThreadPool *pool, xiThreadTaskProc *proc, void *arg);

// wait for all tasks currently enqueued on the pool to be completed
//
extern XI_API void xi_thread_pool_await_complete(xiThreadPool *pool);

#endif  // XI_THREAD_POOL_H_
