#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <pwd.h>

#include <fcntl.h>

#include <pthread.h>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#include <dirent.h>

// @copy: from winapi, should be somewhere else for cross-platform
//
#define MAX_TITLE_COUNT 1024

// SDL2 functions and types
//
// @todo: native windowing, sound and input handling? instead of using SDL2
//
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

typedef void SDL2_SDL_SetWindowMinimumSize(SDL_Window *, int, int);
typedef void SDL2_SDL_SetWindowMaximumSize(SDL_Window *, int, int);
typedef void SDL2_SDL_SetWindowTitle(SDL_Window *, const char *);
typedef void SDL2_SDL_PauseAudioDevice(SDL_AudioDeviceID, int);

typedef int SDL2_SDL_Init(Uint32);
typedef int SDL2_SDL_InitSubSystem(Uint32);
typedef int SDL2_SDL_PollEvent(SDL_Event *);
typedef int SDL2_SDL_SetWindowFullscreen(SDL_Window *, SDL_bool);
typedef int SDL2_SDL_SetWindowResizable(SDL_Window *, SDL_bool);
typedef int SDL2_SDL_GetWindowSize(SDL_Window *, int *, int *);
typedef int SDL2_SDL_QueueAudio(SDL_AudioDeviceID, const void *, Uint32);
typedef int SDL2_SDL_GetNumVideoDisplays(void);
typedef int SDL2_SDL_GetDesktopDisplayMode(int, SDL_DisplayMode *);

typedef Uint32 SDL2_SDL_GetQueuedAudioSize(SDL_AudioDeviceID);

typedef const char *SDL2_SDL_GetDisplayName(int);
typedef const char *SDL2_SDL_GetError(void);

typedef SDL_AudioDeviceID SDL2_SDL_OpenAudioDevice(const char *, int, const SDL_AudioSpec *, SDL_AudioSpec *, int);

typedef SDL_Window *SDL2_SDL_CreateWindow(const char *, int, int, int, int, Uint32);

// gl functions passed to the renderering backend
//
typedef SDL_GLContext SDL2_SDL_GL_CreateContext(SDL_Window *window);

typedef void *SDL2_SDL_GL_GetProcAddress(const char *);

typedef int  SDL2_SDL_GL_SetSwapInterval(int);
typedef int  SDL2_SDL_GL_SetAttribute(SDL_GLattr, int);
typedef void SDL2_SDL_GL_SwapWindow(SDL_Window *);
typedef void SDL2_SDL_GL_DeleteContext(SDL_GLContext);
typedef void SDL2_SDL_GL_GetDrawableSize(SDL_Window *window, int *, int *);

typedef SDL_bool SDL2_SDL_GetWindowWMInfo(SDL_Window *, SDL_SysWMinfo *);

#define SDL2_FUNCTION_POINTER(name) SDL2_SDL_##name *name

// :renderer_core
//
typedef struct SDL2_WindowData SDL2_WindowData;
struct SDL2_WindowData {
    SDL_Window *window;

    SDL2_FUNCTION_POINTER(GL_CreateContext);
    SDL2_FUNCTION_POINTER(GL_GetProcAddress);
    SDL2_FUNCTION_POINTER(GL_SetSwapInterval);
    SDL2_FUNCTION_POINTER(GL_SetAttribute);
    SDL2_FUNCTION_POINTER(GL_SwapWindow);
    SDL2_FUNCTION_POINTER(GL_DeleteContext);
    SDL2_FUNCTION_POINTER(GetWindowWMInfo);
};

typedef struct SDL2_Context SDL2_Context;
struct SDL2_Context {
    void *lib;
    SDL_Window *window;

    U32 window_state;
    Buffer title;

    struct {
        B32 enabled;

        SDL_AudioDeviceID device;
        SDL_AudioSpec format;

        S16 *samples;
    } audio;

    SDL2_FUNCTION_POINTER(Init);
    SDL2_FUNCTION_POINTER(InitSubSystem);
    SDL2_FUNCTION_POINTER(PollEvent);
    SDL2_FUNCTION_POINTER(SetWindowFullscreen);
    SDL2_FUNCTION_POINTER(SetWindowResizable);
    SDL2_FUNCTION_POINTER(CreateWindow);
    SDL2_FUNCTION_POINTER(SetWindowMinimumSize);
    SDL2_FUNCTION_POINTER(SetWindowMaximumSize);
    SDL2_FUNCTION_POINTER(SetWindowTitle);
    SDL2_FUNCTION_POINTER(GetWindowSize);
    SDL2_FUNCTION_POINTER(GetError);
    SDL2_FUNCTION_POINTER(OpenAudioDevice);
    SDL2_FUNCTION_POINTER(QueueAudio);
    SDL2_FUNCTION_POINTER(GetQueuedAudioSize);
    SDL2_FUNCTION_POINTER(PauseAudioDevice);
    SDL2_FUNCTION_POINTER(GetNumVideoDisplays);
    SDL2_FUNCTION_POINTER(GetDesktopDisplayMode);
    SDL2_FUNCTION_POINTER(GetDisplayName);

    SDL2_FUNCTION_POINTER(GL_GetDrawableSize);
};

#undef SDL2_FUNCTION_POINTER

typedef struct Linux_Context Linux_Context;
struct Linux_Context {
    M_Arena *arena;
    Logger logger;

    Xi_Engine xi;

    SDL2_Context SDL;

    B32 quitting;
    B32 did_update;

    U64 start_ns;
    U64 resolution_ns;

    S64 accum_ns;
    S64 fixed_ns;
    S64 clamp_ns;

    struct {
        B32 valid;
        void *lib;

        SDL2_WindowData   window; // move this to xiSDL2Context
        RendererInitProc *Init;
    } renderer;

    struct {
        void *lib;
        U64   time;

        const char *path;

        Xi_GameCode *code;
    } game;
};

// :note linux memory allocation functions
//
U64 OS_AllocationGranularity() {
    U64 result = sysconf(_SC_PAGE_SIZE);
    return result;
}

U64 OS_PageSize() {
    U64 result = sysconf(_SC_PAGE_SIZE);
    return result;
}

void *OS_MemoryReserve(U64 size) {
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    return (result == MAP_FAILED) ? 0 : result;
}

B32 OS_MemoryCommit(void *base, U64 size) {
    B32 result = (mprotect(base, size, PROT_READ | PROT_WRITE) == 0);
    return result;
}

void OS_MemoryDecommit(void *base, U64 size) {
    // No real decommit on POSIX
    //
    memset(base, 0, size);
    mprotect(base, size, PROT_NONE);
}

void OS_MemoryRelease(void *base, U64 size) {
    munmap(base, size);
}

// :note linux threading
//
FileScope void *Linux_ThreadProc(void *arg) {
    ThreadQueue *queue = cast(ThreadQueue *) arg;
    ThreadRun(queue);

    return 0;
}

void OS_ThreadStart(ThreadQueue *arg) {
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);

    pthread_t id;
    int result = pthread_create(&id, &attrs, Linux_ThreadProc, (void *) arg);
    if (result != 0) {
        // @todo: logging...
        //
    }

    pthread_attr_destroy(&attrs);
}

void OS_FutexWait(Futex *futex, Futex value) {
    for (;;) {
        int result = syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, value, 0, 0, 0);
        if (result == -1) {
            if (errno == EAGAIN) {
                break;
            }
        }
        else {
            if (*futex != value) { break; }
        }
    }
}

void OS_FutexSignal(Futex *futex) {
    syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0, 0);
}

void OS_FutexBroadcast(Futex *futex) {
    syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX, 0, 0, 0);
}

// :note linux file system functions
//
typedef U32 Linux_PathType;
enum {
    LINUX_PATH_TYPE_EXECUTABLE = 0,
    LINUX_PATH_TYPE_WORKING,
    LINUX_PATH_TYPE_USER,
    LINUX_PATH_TYPE_TEMP
};

FileScope const char *Linux_SystemPathGet(Linux_PathType type) {
    const char *result = "/tmp"; // on error, not much we can do use /tmp

    M_Arena *temp = M_TempGet();

    switch (type) {
        case LINUX_PATH_TYPE_EXECUTABLE: {
            // ENAMETOOLONG can occur, readlinkat instead?
            //
            char *path  = M_ArenaPush(temp, char, PATH_MAX + 1);
            ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);

            if (len > 0) {
                while ((len > 0) && (path[len] != '/')) {
                    // walk back the path until we find the first slash, this
                    // will chop off the executable name
                    //
                    len -= 1;
                }

                if (len == 1) { len += 1; }

                path[len] = 0; // null-terminate, if len = 1, root dir

                result = path;

                // remove the unused size
                //
                M_ArenaPop(temp, char, PATH_MAX - len);
            }
            else {
                M_ArenaPop(temp, char, PATH_MAX + 1); // error remove everything
            }
        }
        break;
        case LINUX_PATH_TYPE_WORKING: {
            result = M_ArenaPush(temp, char, PATH_MAX + 1);
            getcwd((char *) result, PATH_MAX);
        }
        break;
        case LINUX_PATH_TYPE_USER: {
            uid_t uid = getuid();
            struct passwd *pwd = getpwuid(uid);
            if (pwd) {
                const char *home = pwd->pw_dir;
                Str8 user_dir = Str8_Format(temp, "%s/.local/share", home);
                OS_DirectoryCreate(user_dir);

                Str8 copy = Str8_PushCopy(temp, user_dir);
                result    = cast(const char *) copy.data;
            }
        }
        break;
        case LINUX_PATH_TYPE_TEMP: {
            // POSIX compliant systems have /tmp as a fixed location
            //
            Str8 tmp = Str8_PushCopy(temp, Str8_Literal("/tmp"));
            result   = cast(const char *) tmp.data;
        }
        break;
    }

    return result;
}

FileScope char *Linux_RealpathGet(Str8 path) {
    char *result;

    M_Arena *temp = M_TempGet();

    // We have to do this ourselves because normal realpath will fail if the specified location
    // doesn't exist... even though you may well want to canonicalize a location that has yet
    // to be created
    //

    if (path.count == 0) {
        // @todo: maybe this should be interpreted as 'the working directory'
        //
        result = "";
    }
    else {
        result = M_ArenaPush(temp, char, PATH_MAX + 1);

        const char *cpath;
        if (path.data[0] == '/') {
            // is aboslute just copy the entire path
            //
            Str8 copy = Str8_PushCopy(temp, path);
            cpath     = cast(const char *) copy.data;
        }
        else {
            // may be relative
            //
            const char *exe_path  = Linux_SystemPathGet(LINUX_PATH_TYPE_WORKING);
            Str8        full_path = Str8_Format(temp, "%s/%.*s", exe_path, Str8_Arg(path));

            Str8 copy = Str8_PushCopy(temp, full_path);
            cpath     = cast(const char *) copy.data;
        }

        U64 count = 0;
        const char *at = cpath;

        while (at[0] != 0) {
            if (at[0] == '.' && at[1] == '.') {
                // move back a segment and skip .. relative segment
                //
                if (at[2] == 0 || at[2] == '/') {
                    if (count > 1) {
                        count -= 1;
                        while ((count > 1) && result[count - 1] != '/') {
                            count -= 1;
                        }
                    }

                    at += (at[2] == 0) ? 2 : 3;
                }
            }
            else if (at[0] == '.' && (at[1] == 0 || at[1] == '/')) {
                at += ((at[1] == 0) ? 1 : 2);
            }
            else {
                result[count] = at[0];
                count += 1;
                at += 1;
            }

            if (count >= PATH_MAX) { break; }
        }

        // remove trailing forward slash
        //
        if (count > 1 && result[count - 1] == '/') { count -= 1; }

        result[count] = 0; // null terminate

        M_ArenaPop(temp, char, PATH_MAX - count);
    }

    return result;
}

void Linux_DirectoryListBuilderGetRecursive(M_Arena *arena, DirectoryListBuilder *builder, const char *path, B32 recursive) {
    // @todo: for full path length support (as there is technically no
    // defined limit) we should probably be using fopendir with openat, not
    // sure how this will interact with realpath
    //
    DIR *dir = opendir(path);
    if (dir) {
        for (struct dirent *entry = readdir(dir); entry != 0; entry = readdir(dir)) {
            // ignore hidden files
            //
            if (entry->d_name[0] == '.') { continue; } // :cf not right

            M_Arena *temp = M_TempGet();

            Str8 full_path = Str8_Format(arena, "%s/%s", path, entry->d_name);
            Str8 new_path  = Str8_PushCopy(temp, full_path);

            struct stat sb;
            if (stat((const char *) new_path.data, &sb) == 0) {
                B32  is_dir   = (sb.st_mode & S_IFMT) == S_IFDIR;
                U64  time     = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
                Str8 basename = Str8_RemoveBeforeLast(full_path, '/');

                DirectoryEntry entry;
                entry.type     = is_dir ? DIRECTORY_ENTRY_TYPE_DIRECTORY : DIRECTORY_ENTRY_TYPE_FILE;
                entry.path     = full_path;
                entry.basename = basename;
                entry.size     = sb.st_size;
                entry.time     = time;

                DirectoryListBuilderAppend(builder, entry);

                if (recursive && is_dir) {
                    const char *next_dir = cast(const char *) new_path.data;
                    Linux_DirectoryListBuilderGetRecursive(arena, builder, next_dir, recursive);
                }
            }
        }
    }
}

void OS_DirectoryListBuild(M_Arena *arena, DirectoryListBuilder *builder, Str8 path, B32 recursive) {
    const char *cpath = Linux_RealpathGet(path);
    if (cpath != 0) {
        Linux_DirectoryListBuilderGetRecursive(arena, builder, cpath, recursive);
    }
}

B32 OS_DirectoryEntryGetByPath(M_Arena *arena, DirectoryEntry *entry, Str8 path) {
    B32 result = false;

    const char *cpath = Linux_RealpathGet(path);

    if (cpath != 0) {
        struct stat sb;
        if (stat(cpath, &sb) == 0) {
            B32  is_dir   = (sb.st_mode & S_IFMT) == S_IFDIR;
            U64  time     = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
            Str8 basename = Str8_PushCopy(arena, Str8_RemoveAfterLast(path, '/'));

            entry->type     = is_dir ? DIRECTORY_ENTRY_TYPE_DIRECTORY : DIRECTORY_ENTRY_TYPE_FILE;
            entry->path     = path;
            entry->basename = basename;
            entry->size     = sb.st_size;
            entry->time     = time;

            result = true;
        }
    }

    return result;
}

B32 OS_DirectoryCreate(Str8 path) {
    B32 result = true;

    char *cpath = Linux_RealpathGet(path);

    // there will always be a fowrard slash as linux_realpath_get converts to absolute
    //
    U64 it = 1;
    while (cpath[it] != 0) {
        if (cpath[it] == '/') {
            cpath[it] = '\0'; // null-terminate this segment

            if (mkdir(cpath, 0755) != 0) {
                if (errno != EEXIST) {
                    result = false;
                    break;
                }
            }

            cpath[it] = '/'; // restore the forward slash
        }

        it += 1;
    }

    if (result) {
        if (mkdir(cpath, 0755) != 0) {
            if (errno == EEXIST) {
                struct stat sb;
                if (stat(cpath, &sb) == 0) {
                    result = (sb.st_mode & S_IFMT) == S_IFDIR;
                }
            }
        }
    }

    return result;
}

void OS_DirectoryDelete(Str8 path) {
    char *cpath = Linux_RealpathGet(path);
    rmdir(cpath);
}

FileHandle OS_FileOpen(Str8 path, FileAccessFlags access) {
    FileHandle result;

    result.status = FILE_HANDLE_STATUS_VALID;
    result.v      = 0;

    char *cpath = Linux_RealpathGet(path);

    int flags = 0;
    if (access == FILE_ACCESS_FLAG_READWRITE) {
        flags = O_RDWR | O_CREAT;
    }
    else if (access == FILE_ACCESS_FLAG_READ) {
        flags = O_RDONLY;
    }
    else if (access == FILE_ACCESS_FLAG_WRITE) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    }

    int fd = open(cpath, flags, 0644);

    result.v = cast(U64) fd;

    if (fd < 0) { result.status = FILE_HANDLE_STATUS_FAILED_OPEN; }
    return result;
}

void OS_FileClose(FileHandle *handle) {
    int fd = cast(int) handle->v;
    if (fd >= 0) {
        close(fd);
    }

    handle->status = FILE_HANDLE_STATUS_CLOSED;
}

void OS_FileDelete(Str8 path) {
    char *cpath = Linux_RealpathGet(path);
    unlink(cpath);
}

B32 OS_FileRead(FileHandle *handle, void *dst, U64 offset, U64 size) {
    B32 result = false;

    if (handle->status == FILE_HANDLE_STATUS_VALID) {
        result = true;

        int fd = cast(int) handle->v;

        Assert(fd >= 0);
        Assert(offset != FILE_OFFSET_APPEND);

        U8 *data = cast(U8 *) dst;
        while (size > 0) {
            int nread = pread(fd, data, size, offset);
            if (nread < 0) {
                result = false;
                handle->status = FILE_HANDLE_STATUS_FAILED_READ;
                break;
            }

            Assert(size >= nread);

            data   += nread;
            offset += nread;
            size   -= nread;
        }
    }

    return result;
}

B32 OS_FileWrite(FileHandle *handle, void *src, U64 offset, U64 size) {
    B32 result = false;
    if (handle->status == FILE_HANDLE_STATUS_VALID) {
        result = true;

        int fd = cast(int) handle->v;

        Assert(fd >= 0);

        if (offset == FILE_OFFSET_APPEND) {
            lseek(fd, 0, SEEK_END); // :threading ??

            // we are appending to the end so just use normal write
            //
            U8 *data = cast(U8 *) src;
            while (size > 0) {
                int nwritten = write(fd, data, size);
                if (nwritten < 0) {
                    result = false;
                    handle->status = FILE_HANDLE_STATUS_FAILED_WRITE;
                    break;
                }

                Assert(size >= nwritten);

                data   += nwritten;
                offset += nwritten;
                size   -= nwritten;
            }
        }
        else {
            // specific offset use pwrite
            //
            U8 *data = cast(U8 *) src;
            while (size > 0) {
                int nwritten = pwrite(fd, data, size, offset);
                if (nwritten < 0) {
                    result = false;
                    handle->status = FILE_HANDLE_STATUS_FAILED_WRITE;
                    break;
                }

                Assert(size >= nwritten);

                data   += nwritten;
                offset += nwritten;
                size   -= nwritten;
            }
        }
    }

    return result;
}

// :note game code functions
//
FileScope void Linux_GameCodeInit(Linux_Context *context) {
    Xi_GameCode *game = context->game.code;

    if (game->reloadable) {
        M_Arena *temp = M_TempGet();

        const char *cexe_dir = Linux_SystemPathGet(LINUX_PATH_TYPE_EXECUTABLE);
        Str8        exe_dir  = Str8_WrapNullTerminated(cexe_dir);

        DirectoryList all     = DirectoryListGet(temp, exe_dir, false);
        DirectoryList so_list = DirectoryListFilterForSuffix(temp, &all, Str8_Literal(".so"));

        for (U32 it = 0; it < so_list.num_entries; ++it) {
            DirectoryEntry *entry = &so_list.entries[it];

            Str8 path = Str8_PushCopy(temp, entry->path);
            void *lib = dlopen((const char *) path.data, RTLD_LAZY | RTLD_LOCAL);

            if (lib) {
                Xi_GameInitProc     *Init     = dlsym(lib, "__xi_game_init");
                Xi_GameSimulateProc *Simulate = dlsym(lib, "__xi_game_simulate");
                Xi_GameRenderProc   *Render   = dlsym(lib, "__xi_game_render");

                if (Init != 0 && Simulate != 0 && Render != 0) {
                    Str8 copy = Str8_PushCopy(context->arena, entry->path);
                    context->game.path = cast(const char *) copy.data;

                    struct stat sb;
                    if (stat(context->game.path, &sb) == 0) {
                        context->game.lib = lib;

                        game->Init     = Init;
                        game->Simulate = Simulate;
                        game->Render   = Render;

                        context->game.time = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
                        break;
                    }
                }
                else {
                    dlclose(lib);
                }
            }
        }
    }

    if (!game->Init)     { game->Init     = __xi_game_init;     }
    if (!game->Simulate) { game->Simulate = __xi_game_simulate; }
    if (!game->Render)   { game->Render   = __xi_game_render;   }
}

FileScope void Linux_GameCodeReload(Linux_Context *context) {
    struct stat sb;
    if (stat(context->game.path, &sb) == 0) {
        Xi_GameCode *game = context->game.code;

        U64 time = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
        if (time != context->game.time) {
            if (context->game.lib) {
                dlclose(context->game.lib);

                game->Init     = __xi_game_init;
                game->Simulate = __xi_game_simulate;
                game->Render   = __xi_game_render;
            }

            void *new_lib = dlopen(context->game.path, RTLD_LAZY | RTLD_LOCAL);
            if (new_lib) {
                Xi_GameInitProc     *Init     = dlsym(new_lib, "__xi_game_init");
                Xi_GameSimulateProc *Simulate = dlsym(new_lib, "__xi_game_simulate");
                Xi_GameRenderProc   *Render   = dlsym(new_lib, "__xi_game_render");

                if (Init && Simulate && Render) {
                    context->game.lib  = new_lib;
                    context->game.time = time;

                    game->Init     = Init;
                    game->Simulate = Simulate;
                    game->Render   = Render;

                    // call the init function of the game to notify it has been reloaded
                    //
                    game->Init(&context->xi, XI_GAME_RELOADED);
                }
            }
        }
    }
}

FileScope SDL2_Context *SDL2_Load(Linux_Context *context) {
    SDL2_Context *result = 0;

    SDL2_Context *SDL = &context->SDL;
    SDL->lib = dlopen("libSDL2.so", RTLD_LAZY | RTLD_LOCAL);
    if (!SDL->lib) {
        Log(&context->logger, Str8_Literal("init"), "failed to find libSDL2.so globally... trying locally");

        const char *exe_dir = Linux_SystemPathGet(LINUX_PATH_TYPE_EXECUTABLE);

        M_Arena *temp = M_TempGet();

        // @todo: maybe Str8_Format should also guarantee a null-terminated string?
        //
        Str8 local_sdl    = Str8_PushCopy(temp, Str8_Format(temp, "%s/libSDL2.so", exe_dir));
        const char *cpath = cast(const char *) local_sdl.data;

        SDL->lib = dlopen(cpath, RTLD_NOW | RTLD_GLOBAL);
    }

    if (SDL->lib) {
        SDL2_WindowData *window = &context->renderer.window;

#define SDL2_FUNCTION_LOAD(x, name) (x)->name = (SDL2_SDL_##name *) dlsym(SDL->lib, Stringify(SDL_##name)); Assert((x)->name != 0)
        SDL2_FUNCTION_LOAD(SDL, Init);
        SDL2_FUNCTION_LOAD(SDL, InitSubSystem);
        SDL2_FUNCTION_LOAD(SDL, PollEvent);
        SDL2_FUNCTION_LOAD(SDL, SetWindowFullscreen);
        SDL2_FUNCTION_LOAD(SDL, SetWindowResizable);
        SDL2_FUNCTION_LOAD(SDL, CreateWindow);
        SDL2_FUNCTION_LOAD(SDL, SetWindowMinimumSize);
        SDL2_FUNCTION_LOAD(SDL, SetWindowMaximumSize);
        SDL2_FUNCTION_LOAD(SDL, SetWindowTitle);
        SDL2_FUNCTION_LOAD(SDL, GetWindowSize);
        SDL2_FUNCTION_LOAD(SDL, GetError);
        SDL2_FUNCTION_LOAD(SDL, OpenAudioDevice);
        SDL2_FUNCTION_LOAD(SDL, QueueAudio);
        SDL2_FUNCTION_LOAD(SDL, GetQueuedAudioSize);
        SDL2_FUNCTION_LOAD(SDL, PauseAudioDevice);
        SDL2_FUNCTION_LOAD(SDL, GetNumVideoDisplays);
        SDL2_FUNCTION_LOAD(SDL, GetDesktopDisplayMode);
        SDL2_FUNCTION_LOAD(SDL, GetDisplayName);

        SDL2_FUNCTION_LOAD(SDL, GL_GetDrawableSize);

        // gl functions
        //
        SDL2_FUNCTION_LOAD(window, GL_CreateContext);
        SDL2_FUNCTION_LOAD(window, GL_GetProcAddress);
        SDL2_FUNCTION_LOAD(window, GL_SetSwapInterval);
        SDL2_FUNCTION_LOAD(window, GL_SetAttribute);
        SDL2_FUNCTION_LOAD(window, GL_SwapWindow);
        SDL2_FUNCTION_LOAD(window, GL_DeleteContext);
        SDL2_FUNCTION_LOAD(window, GetWindowWMInfo);
#undef  SDL2_FUNCTION_LOAD

        if (SDL->Init(SDL_INIT_VIDEO) == 0) {
            result = SDL;
        }
        else {
            Log(&context->logger, Str8_Literal("init"), "failed to initialise (%s)", SDL->GetError());
        }
    }
    else {
        Log(&context->logger, Str8_Literal("init"), "failed to load libSDL2.so");
    }

    return result;
}

FileScope void SDL2_DisplaysEnumerate(Linux_Context *context) {
    SDL2_Context *SDL = &context->SDL;

    int count = SDL->GetNumVideoDisplays();
    if (count > 0) {
        Xi_Engine *xi = &context->xi;

        xi->system.num_displays = count;
        xi->system.displays     = M_ArenaPush(context->arena, DisplayInfo, count);

        for (int it = 0; it < count; ++it) {
            DisplayInfo *display = &xi->system.displays[it];

            SDL_DisplayMode mode;
            if (SDL->GetDesktopDisplayMode(it, &mode) == 0) {
                const char *cname = SDL->GetDisplayName(it);
                if (cname != 0) {
                    Str8 name     = Str8_WrapNullTerminated(cname);
                    display->name = Str8_PushCopy(context->arena, name);
                }

                // @incomplete: sdl doesn't provide a way to calculate scale without first creating a
                // window and checking the differences between GetDrawableSize and GetWindowSize
                //
                display->width        = mode.w;
                display->height       = mode.h;
                display->refresh_rate = mode.refresh_rate;
                display->scale        = 1.0f;
            }
        }
    }
}

FileScope void SDL2_AudioInit(Linux_Context *context) {
    SDL2_Context *SDL = &context->SDL;

    AudioPlayer *player = &context->xi.audio_player;
    AudioPlayerInit(context->arena, player);

    if (SDL->InitSubSystem(SDL_INIT_AUDIO) == 0) {
        int flags = SDL_AUDIO_ALLOW_SAMPLES_CHANGE;

        SDL_AudioSpec format = { 0 };
        format.freq     = player->sample_rate;
        format.format   = AUDIO_S16LSB;
        format.channels = 2;
        format.samples  = U32_Pow2Nearest(player->frame_count);
        format.callback = 0;
        format.userdata = 0;

        SDL_AudioSpec obtained;

        SDL->audio.device = SDL->OpenAudioDevice(0, SDL_FALSE, &format, &obtained, flags);
        if (SDL->audio.device != 0) {
            SDL->audio.format  = obtained;
            SDL->audio.samples = M_ArenaPush(context->arena, S16, obtained.samples << 1);

            SDL->PauseAudioDevice(SDL->audio.device, 0);

            SDL->audio.enabled = true;
        }
        else {
            Log(&context->logger, Str8_Literal("audio"), "no device found... disabling audio");
        }
    }
    else {
        Log(&context->logger, Str8_Literal("audio"), "failed to initialise (%s)", SDL->GetError());
    }

}

FileScope void SDL2_AudioMix(Linux_Context *context) {
    SDL2_Context *SDL = &context->SDL;
    Xi_Engine    *xi  = &context->xi;

    if (SDL->audio.enabled) {
        // @hardcode: divide by 4 as a single frame is (channels * sizeof(sample)) which
        // in our case is only supported to be (2 * sizeof(s16)) == 4
        //
        // we will probably want to support more channels, and maybe even different sample
        // formats in the future
        //
        // :frame_count
        //
        U32 max_frames    = SDL->audio.format.samples;
        U32 queued_frames = SDL->GetQueuedAudioSize(SDL->audio.device) >> 2;

        Assert(queued_frames <= max_frames);

        U32 frame_count = max_frames - queued_frames;
        if (frame_count > 0) {
            F32 dt = cast(F32) xi->time.delta.s;
            AudioPlayerUpdate(&xi->audio_player, &xi->assets, SDL->audio.samples, frame_count, dt);

            SDL->QueueAudio(SDL->audio.device, SDL->audio.samples, frame_count << 2); // :frame_count
        }
    }
}

GlobalVar U8 tbl_sdl2_scancode_to_input_key[] = {
    [SDL_SCANCODE_UNKNOWN] = KEYBOARD_KEY_UNKNOWN,

    // general keys
    //
    [SDL_SCANCODE_RETURN]    = KEYBOARD_KEY_RETURN,
    [SDL_SCANCODE_ESCAPE]    = KEYBOARD_KEY_ESCAPE,
    [SDL_SCANCODE_BACKSPACE] = KEYBOARD_KEY_BACKSPACE,
    [SDL_SCANCODE_TAB]       = KEYBOARD_KEY_TAB,
    [SDL_SCANCODE_HOME]      = KEYBOARD_KEY_HOME,
    [SDL_SCANCODE_PAGEUP]    = KEYBOARD_KEY_PAGEUP,
    [SDL_SCANCODE_DELETE]    = KEYBOARD_KEY_DELETE,
    [SDL_SCANCODE_END]       = KEYBOARD_KEY_END,
    [SDL_SCANCODE_PAGEDOWN]  = KEYBOARD_KEY_PAGEDOWN,

    // arrow keys
    //
    [SDL_SCANCODE_RIGHT] = KEYBOARD_KEY_RIGHT, [SDL_SCANCODE_LEFT] = KEYBOARD_KEY_LEFT,
    [SDL_SCANCODE_DOWN]  = KEYBOARD_KEY_DOWN,  [SDL_SCANCODE_UP]   = KEYBOARD_KEY_UP,

    // f keys
    //
    [SDL_SCANCODE_F1]  = KEYBOARD_KEY_F1,  [SDL_SCANCODE_F2]  = KEYBOARD_KEY_F2,
    [SDL_SCANCODE_F3]  = KEYBOARD_KEY_F3,  [SDL_SCANCODE_F4]  = KEYBOARD_KEY_F4,
    [SDL_SCANCODE_F5]  = KEYBOARD_KEY_F5,  [SDL_SCANCODE_F6]  = KEYBOARD_KEY_F6,
    [SDL_SCANCODE_F7]  = KEYBOARD_KEY_F7,  [SDL_SCANCODE_F8]  = KEYBOARD_KEY_F8,
    [SDL_SCANCODE_F9]  = KEYBOARD_KEY_F9,  [SDL_SCANCODE_F10] = KEYBOARD_KEY_F10,
    [SDL_SCANCODE_F11] = KEYBOARD_KEY_F11, [SDL_SCANCODE_F12] = KEYBOARD_KEY_F12,

    // more general
    //
    [SDL_SCANCODE_INSERT]     = KEYBOARD_KEY_INSERT,
    [SDL_SCANCODE_SPACE]      = KEYBOARD_KEY_SPACE,
    [SDL_SCANCODE_APOSTROPHE] = KEYBOARD_KEY_QUOTE,
    [SDL_SCANCODE_COMMA]      = KEYBOARD_KEY_COMMA,
    [SDL_SCANCODE_MINUS]      = KEYBOARD_KEY_MINUS,
    [SDL_SCANCODE_PERIOD]     = KEYBOARD_KEY_PERIOD,
    [SDL_SCANCODE_SLASH]      = KEYBOARD_KEY_SLASH,

    // numbers
    //
    [SDL_SCANCODE_0] = KEYBOARD_KEY_0, [SDL_SCANCODE_1] = KEYBOARD_KEY_1,
    [SDL_SCANCODE_2] = KEYBOARD_KEY_2, [SDL_SCANCODE_3] = KEYBOARD_KEY_3,
    [SDL_SCANCODE_4] = KEYBOARD_KEY_4, [SDL_SCANCODE_5] = KEYBOARD_KEY_5,
    [SDL_SCANCODE_6] = KEYBOARD_KEY_6, [SDL_SCANCODE_7] = KEYBOARD_KEY_7,
    [SDL_SCANCODE_8] = KEYBOARD_KEY_8, [SDL_SCANCODE_9] = KEYBOARD_KEY_9,

    // more more general
    //
    [SDL_SCANCODE_SEMICOLON] = KEYBOARD_KEY_SEMICOLON,
    [SDL_SCANCODE_EQUALS]    = KEYBOARD_KEY_EQUALS,

    // modifiers
    //
    [SDL_SCANCODE_LSHIFT] = KEYBOARD_KEY_LSHIFT,
    [SDL_SCANCODE_LCTRL]  = KEYBOARD_KEY_LCTRL,
    [SDL_SCANCODE_LALT]   = KEYBOARD_KEY_LALT,

    [SDL_SCANCODE_RSHIFT] = KEYBOARD_KEY_RSHIFT,
    [SDL_SCANCODE_RCTRL]  = KEYBOARD_KEY_RCTRL,
    [SDL_SCANCODE_RALT]   = KEYBOARD_KEY_RALT,

    // more more more general
    //
    [SDL_SCANCODE_LEFTBRACKET]  = KEYBOARD_KEY_LBRACKET,
    [SDL_SCANCODE_BACKSLASH]    = KEYBOARD_KEY_BACKSLASH,
    [SDL_SCANCODE_RIGHTBRACKET] = KEYBOARD_KEY_RBRACKET,
    [SDL_SCANCODE_GRAVE]        = KEYBOARD_KEY_GRAVE,

    // letters
    //
    [SDL_SCANCODE_A] = KEYBOARD_KEY_A, [SDL_SCANCODE_B] = KEYBOARD_KEY_B,
    [SDL_SCANCODE_C] = KEYBOARD_KEY_C, [SDL_SCANCODE_D] = KEYBOARD_KEY_D,
    [SDL_SCANCODE_E] = KEYBOARD_KEY_E, [SDL_SCANCODE_F] = KEYBOARD_KEY_F,
    [SDL_SCANCODE_G] = KEYBOARD_KEY_G, [SDL_SCANCODE_H] = KEYBOARD_KEY_H,
    [SDL_SCANCODE_I] = KEYBOARD_KEY_I, [SDL_SCANCODE_J] = KEYBOARD_KEY_J,
    [SDL_SCANCODE_K] = KEYBOARD_KEY_K, [SDL_SCANCODE_L] = KEYBOARD_KEY_L,
    [SDL_SCANCODE_M] = KEYBOARD_KEY_M, [SDL_SCANCODE_N] = KEYBOARD_KEY_N,
    [SDL_SCANCODE_O] = KEYBOARD_KEY_O, [SDL_SCANCODE_P] = KEYBOARD_KEY_P,
    [SDL_SCANCODE_Q] = KEYBOARD_KEY_Q, [SDL_SCANCODE_R] = KEYBOARD_KEY_R,
    [SDL_SCANCODE_S] = KEYBOARD_KEY_S, [SDL_SCANCODE_T] = KEYBOARD_KEY_T,
    [SDL_SCANCODE_U] = KEYBOARD_KEY_U, [SDL_SCANCODE_V] = KEYBOARD_KEY_V,
    [SDL_SCANCODE_W] = KEYBOARD_KEY_W, [SDL_SCANCODE_X] = KEYBOARD_KEY_X,
    [SDL_SCANCODE_Y] = KEYBOARD_KEY_Y, [SDL_SCANCODE_Z] = KEYBOARD_KEY_Z
};

FileScope void Linux_ContextUpdate(Linux_Context *context) {
    Xi_Engine    *xi  = &context->xi;
    SDL2_Context *SDL = &context->SDL;

    struct timespec timer_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &timer_end);

    U64 end_ns   = (1000000000 * timer_end.tv_sec) + timer_end.tv_nsec;
    U64 delta_ns = end_ns - context->start_ns;

    delta_ns = Min(delta_ns, context->clamp_ns);

    context->accum_ns += delta_ns;

    xi->time.ticks    = end_ns;
    context->start_ns = end_ns;

    xi->time.delta.ns = (context->fixed_ns);
    xi->time.delta.us = (context->fixed_ns / 1000);
    xi->time.delta.ms = (context->fixed_ns / 1000000);
    xi->time.delta.s  = (context->fixed_ns / 1000000000.0);

    xi->time.total.ns += (delta_ns);
    xi->time.total.us  = cast(U64) ((xi->time.total.ns / 1000.0) + 0.5);
    xi->time.total.ms  = cast(U64) ((xi->time.total.ns / 1000000.0) + 0.5);
    xi->time.total.s   = (xi->time.total.ns / 1000000000.0);

    SDL2_AudioMix(context);

    // process windows messages that our application received
    //
    InputMouse    *mouse = &xi->mouse;
    InputKeyboard *kb    = &xi->keyboard;

    // :step_input
    //
    if (context->did_update) {
        // reset mouse params for this frame
        //
        mouse->connected    = true;
        mouse->active       = false;
        mouse->delta.screen = V2S(0, 0);
        mouse->delta.clip   = V2F(0, 0);
        mouse->delta.wheel  = V2F(0, 0);

        mouse->left.pressed   = mouse->left.released   = false;
        mouse->middle.pressed = mouse->middle.released = false;
        mouse->right.pressed  = mouse->right.released  = false;
        mouse->x0.pressed     = mouse->x0.released     = false;
        mouse->x1.pressed     = mouse->x1.released     = false;

        // reset keyboard params for this frame
        //
        kb->connected = true;
        kb->active    = false;

        for (U32 it = 0; it < KEYBOARD_KEY_COUNT; ++it) {
            kb->keys[it].pressed  = false;
            kb->keys[it].released = false;
        }
    }

    context->did_update = false;

    // we don't handle programatic resizes if the user has manually resized the window
    //
    B32 user_resize = false;

    int window_width = 0, window_height = 0;
    SDL->GetWindowSize(SDL->window, &window_width, &window_height);

    SDL_Event event;
    while (SDL->PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                context->quitting = true;
            }
            break;
            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                // @todo: sdl seems to be dropping key up events on wayland for some reason
                // the key will get stuck on because a key up event never gets posted even
                // though the key was released
                //
                B32 down     = (event.type == SDL_KEYDOWN);
                U32 scancode = event.key.keysym.scancode;

                if (scancode < ArraySize(tbl_sdl2_scancode_to_input_key)) {
                    U32 index = tbl_sdl2_scancode_to_input_key[scancode];

                    InputButton *key = &kb->keys[index];
                    InputButtonHandle(key, down);
                }

                // handle modifiers for easy access booleans
                //
                switch (scancode) {
                    case SDL_SCANCODE_LALT:
                    case SDL_SCANCODE_RALT: {
                        InputButton *left  = &kb->keys[KEYBOARD_KEY_LALT];
                        InputButton *right = &kb->keys[KEYBOARD_KEY_RALT];

                        kb->alt = (left->down || right->down);
                    }
                    break;
                    case SDL_SCANCODE_LCTRL:
                    case SDL_SCANCODE_RCTRL: {
                        InputButton *left  = &kb->keys[KEYBOARD_KEY_LCTRL];
                        InputButton *right = &kb->keys[KEYBOARD_KEY_RCTRL];

                        kb->ctrl = (left->down || right->down);
                    }
                    break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT: {
                        InputButton *left  = &kb->keys[KEYBOARD_KEY_LSHIFT];
                        InputButton *right = &kb->keys[KEYBOARD_KEY_RSHIFT];

                        kb->shift = (left->down || right->down);
                    }
                    break;
                }

                kb->active = true;
            }
            break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN: {
                InputButton *button = 0;

                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:   { button = &mouse->left;   } break;
                    case SDL_BUTTON_MIDDLE: { button = &mouse->middle; } break;
                    case SDL_BUTTON_RIGHT:  { button = &mouse->right;  } break;
                    case SDL_BUTTON_X1:     { button = &mouse->x0;     } break;
                    case SDL_BUTTON_X2:     { button = &mouse->x1;     } break;
                    default: {} break;
                }

                if (button) {
                    B32 down = (event.type == SDL_MOUSEBUTTONDOWN);
                    InputButtonHandle(button, down);

                    mouse->active = true;
                }
            }
            break;
            case SDL_MOUSEMOTION: {
                Vec2S pt = V2S(event.motion.x, event.motion.y);

                Vec2S old_screen = mouse->position.screen;
                Vec2F old_clip   = mouse->position.clip;

                // update screen position
                //
                mouse->position.screen = pt;

                // update clip position
                //
                mouse->position.clip.x = -1.0f + (2.0f * (pt.x / cast(F32) window_width));
                mouse->position.clip.y =  1.0f - (2.0f * (pt.y / cast(F32) window_height));

                Vec2S delta_screen = V2S_Sub(mouse->position.screen, old_screen);
                Vec2F delta_clip   = V2F_Sub(mouse->position.clip,   old_clip);

                mouse->delta.screen = V2S_Add(mouse->delta.screen, delta_screen);
                mouse->delta.clip   = V2F_Add(mouse->delta.clip,   delta_clip);

                mouse->active = true;
            }
            break;
            case SDL_MOUSEWHEEL: {
                mouse->position.wheel.x += event.wheel.preciseX;
                mouse->delta.wheel.x    += event.wheel.preciseX;

                mouse->position.wheel.y += event.wheel.preciseY;
                mouse->delta.wheel.y    += event.wheel.preciseY;

                mouse->active = true;
            }
            break;
            case SDL_WINDOWEVENT: {
                // @todo: handle resizing
                //
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    case SDL_WINDOWEVENT_RESIZED: {
                        user_resize = true;
                    }
                    break;
                }
            }
            break;
        }
    }

    xi->quit = context->quitting;

    if (!Str8_Equal(xi->window.title, SDL->title.str, 0)) {
        S64 count = Min(SDL->title.limit, xi->window.title.count);

        MemoryCopy(SDL->title.data, xi->window.title.data, count);
        SDL->title.used = count;

        M_Arena *temp = M_TempGet();

        Str8 copy = Str8_PushCopy(temp, SDL->title.str);
        SDL->SetWindowTitle(SDL->window, (const char *) copy.data);
    }

    if (!user_resize) {
        // handle programamtic resizing
        //
        // @todo: implement this
        //

        // handle window state changes
        //
        if (xi->window.state != SDL->window_state) {
            switch (xi->window.state) {
                case WINDOW_STATE_WINDOWED:
                case WINDOW_STATE_WINDOWED_RESIZABLE:
                case WINDOW_STATE_WINDOWED_BORDERLESS: {
                    SDL->SetWindowFullscreen(SDL->window, SDL_FALSE);

                    if (xi->window.state == WINDOW_STATE_WINDOWED_RESIZABLE) {
                        SDL->SetWindowResizable(SDL->window, SDL_TRUE);

                        // these have been chosen arbitrarily
                        //
                        SDL->SetWindowMinimumSize(SDL->window, 640, 360);
                        SDL->SetWindowMaximumSize(SDL->window, 3840, 2160);
                    }
                    else {
                        // prevent the window from being resizable
                        //
                        int w, h;
                        if (SDL->GetWindowSize(SDL->window, &w, &h)) {
                            SDL->SetWindowMinimumSize(SDL->window, w, h);
                            SDL->SetWindowMaximumSize(SDL->window, w, h);
                        }

                        SDL->SetWindowResizable(SDL->window, SDL_FALSE);
                    }
                }
                break;
                case WINDOW_STATE_FULLSCREEN: {
                    SDL->SetWindowFullscreen(SDL->window, SDL_TRUE);
                }
                break;
            }

            SDL->window_state = xi->window.state;
        }
    }

    window_width  = 0;
    window_height = 0;

    SDL->GL_GetDrawableSize(SDL->window, &window_width, &window_height);
    if (window_width > 0 && window_height > 0) {
        xi->window.width  = window_width;
        xi->window.height = window_height;

        xi->renderer.setup.window_dim.w = window_width;
        xi->renderer.setup.window_dim.h = window_height;
    }

    xi->window.title = SDL->title.str;

    Xi_GameCode *game = context->game.code;
    if (game->reloadable) {
        Linux_GameCodeReload(context);
    }
}

extern int __xie_bootstrap_run(Xi_GameCode *game) {
    int result = 1;

    FileHandle cout = { FILE_HANDLE_STATUS_VALID, cast(U64) STDOUT_FILENO };
    FileHandle cerr = { FILE_HANDLE_STATUS_VALID, cast(U64) STDERR_FILENO };

    Linux_Context *context;
    {
        M_Arena *arena = M_ArenaAlloc(GB(4), 0);

        context = M_ArenaPush(arena, Linux_Context);
        context->arena = arena;

        context->logger = LoggerCreate(context->arena, cout, KB(512));
    }

    SDL2_Context *SDL = SDL2_Load(context); // result also stored context->SDL
    if (SDL) {
        Xi_Engine *xi = &context->xi;

        SDL2_DisplaysEnumerate(context);

        context->game.code = game;

        // setup our version numbers to notify the game code what version of
        // the engine they are using
        //
        xi->version.major = XI_VERSION_MAJOR;
        xi->version.minor = XI_VERSION_MINOR;
        xi->version.patch = XI_VERSION_PATCH;

        // setup out/err file handles for writing to the console, always open
        // on linux which is nice
        //
        xi->system.out = cout;
        xi->system.err = cerr;

        SDL->title.used   = 0;
        SDL->title.limit  = MAX_TITLE_COUNT;
        SDL->title.data   = M_ArenaPush(context->arena, U8, SDL->title.limit);

        xi->window.title = SDL->title.str;

        const char *exe_path     = Linux_SystemPathGet(LINUX_PATH_TYPE_EXECUTABLE);
        const char *temp_path    = Linux_SystemPathGet(LINUX_PATH_TYPE_TEMP);
        const char *user_path    = Linux_SystemPathGet(LINUX_PATH_TYPE_USER);
        const char *working_path = Linux_SystemPathGet(LINUX_PATH_TYPE_WORKING);

        xi->system.executable_path = Str8_PushCopy(context->arena, Str8_WrapNullTerminated(exe_path));
        xi->system.temp_path       = Str8_PushCopy(context->arena, Str8_WrapNullTerminated(temp_path));
        xi->system.user_path       = Str8_PushCopy(context->arena, Str8_WrapNullTerminated(user_path));
        xi->system.working_path    = Str8_PushCopy(context->arena, Str8_WrapNullTerminated(working_path));

        xi->system.num_cpus = sysconf(_SC_NPROCESSORS_ONLN); // vs _CONF?

        Linux_GameCodeInit(context);

        game->Init(xi, XI_ENGINE_CONFIGURE);

        if (xi->window.width == 0 || xi->window.height == 0) {
            xi->window.width  = 1280;
            xi->window.height = 720;
        }

        int display = Min(xi->window.display, xi->system.num_displays);

        int p = SDL_WINDOWPOS_CENTERED_DISPLAY(display);

        int w = xi->window.width;
        int h = xi->window.height;

        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

        if (xi->window.state == WINDOW_STATE_FULLSCREEN) {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        else if (xi->window.state == WINDOW_STATE_WINDOWED_RESIZABLE) {
            flags |= SDL_WINDOW_RESIZABLE;
        }

        const char *title = "xie";
        if (xi->window.title.count) {
            U64 count = Min(xi->window.title.count, SDL->title.limit);

            xi->window.title.count = count;

            M_Arena *temp = M_TempGet();

            // Null terminate
            //
            Str8 copy = Str8_PushCopy(temp, xi->window.title);
            title     = cast(const char *) copy.data;

            MemoryCopy(SDL->title.data, xi->window.title.data, count);
            SDL->title.used = count;
        }

        SDL->window = SDL->CreateWindow(title, p, p, w, h, flags);
        if (SDL->window) {
            if (xi->window.state != WINDOW_STATE_WINDOWED_RESIZABLE) {
                // this prevents windows from being resized and also prevents
                // them from being tiled on tiling window managers
                //
                SDL->SetWindowMinimumSize(SDL->window, w, h);
                SDL->SetWindowMaximumSize(SDL->window, w, h);
            }

            SDL->window_state = xi->window.state;
            xi->window.title  = SDL->title.str;

            {
                int actual_w, actual_h;
                SDL->GL_GetDrawableSize(SDL->window, &actual_w, &actual_h);

                xi->window.width  = actual_w;
                xi->window.height = actual_h;
            }

            RendererContext *renderer = &xi->renderer;
            {
                // @todo: this _realy_ shouldn't need this duplicated value anyway i should just pass the
                // context window to the renderer directly and it should work with that it requires the
                // window structure in xiContext to be an actual struct rather than an inline on so
                // i'll have to change it at some point.
                //
                // it will be more consistent this way
                //
                // :renderer_window . we can also do the 'platform' typedef thing to void to have platform
                // data stored directly on the window or something
                //
                renderer->setup.window_dim.w = xi->window.width;
                renderer->setup.window_dim.h = xi->window.height;

#if 0
                M_Arena *temp = M_TempGet();
                Str8 renderer_path = Str8_PushCopy(temp, Str8_Format(temp, "%s/xi_opengld.so", exe_path));

                const char *path = cast(const char *) renderer_path.data;
                context->renderer.lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);

                if (context->renderer.lib) {
                    context->renderer.Init = dlsym(context->renderer.lib, "GL_Init");
                    if (context->renderer.Init) {
                        SDL2_WindowData *window_data = &context->renderer.window;
                        window_data->window = SDL->window;

                        context->renderer.valid = context->renderer.Init(renderer, window_data);
                    }
                    else {
                        Log(&context->logger, Str8_Literal("renderer"), "xi_opengl_init not found");
                    }
                }
                else {
                    Log(&context->logger, Str8_Literal("renderer"), "failed to load xi_opengld.so");
                }
#else
                M_Arena *temp = M_TempGet();
                Str8 renderer_path = Str8_PushCopy(temp, Str8_Format(temp, "%s/xi_vulkand.so", exe_path));

                const char *path = cast(const char *) renderer_path.data;
                context->renderer.lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);

                if (context->renderer.lib) {
                    context->renderer.Init = dlsym(context->renderer.lib, "VK_Init");
                    if (context->renderer.Init) {
                        SDL2_WindowData *window_data = &context->renderer.window;
                        window_data->window = SDL->window;

                        context->renderer.valid = context->renderer.Init(renderer, window_data);
                    }
                    else {
                        Log(&context->logger, Str8_Literal("renderer"), "'VK_Init' not found in xi_vulkand.so");
                    }
                }
                else {
                    Log(&context->logger, Str8_Literal("renderer"), "Failed to load xi_vulkand.so");
                }
#

#endif
            }

            if (context->renderer.valid) {
                SDL2_AudioInit(context);

                ThreadPoolInit(context->arena, &xi->thread_pool, xi->system.num_cpus);
                AssetManagerInit(context->arena, &xi->assets, xi);

                // setup timing information
                //
                // @todo: there should probably be some sort of shared
                // 'defaults_set' call on the xiContext so we don't have to do
                // all of this in the os layer
                //
                {
                    if (xi->time.delta.fixed_hz == 0) {
                        // @todo: maybe this shoud just be the refresh rate?
                        //
                        xi->time.delta.fixed_hz = 100;
                    }

                    context->fixed_ns = (1000000000 / xi->time.delta.fixed_hz);

                    if (xi->time.delta.clamp_s <= 0.0f) {
                        xi->time.delta.clamp_s = 0.2f;
                        context->clamp_ns      = 200000000;
                    }
                    else {
                        context->clamp_ns = cast(U64) (1000000000 * xi->time.delta.clamp_s);
                    }

                    // set the resolution
                    //
                    struct timespec timer_res;
                    clock_getres(CLOCK_MONOTONIC_RAW, &timer_res);
                    context->resolution_ns = (1000000000 * timer_res.tv_sec) + timer_res.tv_nsec;

                    struct timespec timer_start;
                    clock_gettime(CLOCK_MONOTONIC_RAW, &timer_start);

                    context->start_ns = (1000000000 * timer_start.tv_sec) + timer_start.tv_nsec;
                    xi->time.ticks    = context->start_ns;
                }

                game->Init(xi, XI_GAME_INIT);

                LoggerFlush(&context->logger);

                while (true) {
                    Linux_ContextUpdate(context);

                    // :temp_usage
                    //
                    M_TempReset();

                    while (context->accum_ns >= context->fixed_ns) {
                        game->Simulate(xi);
                        context->accum_ns -= context->fixed_ns;

                        context->did_update = true;

                        // :multiple_updates
                        //
                        xi->audio_player.event_count = 0;

                        if (context->quitting || xi->quit) { break; }
                    }

                    if (context->accum_ns < 0) { context->accum_ns = 0; }

                    game->Render(xi, renderer);

                    // :single_worker
                    //
                    if (xi->thread_pool.num_threads == 1) { ThreadPoolAwaitComplete(&xi->thread_pool); }

                    renderer->Submit(renderer);
                    if (context->quitting || xi->quit) {
                        break;
                    }
                }

                ThreadPoolAwaitComplete(&xi->thread_pool);

                result = 0;
            }
            else {
                Log(&context->logger, Str8_Literal("renderer"), "failed to initialise");
            }
        }
        else {
            Log(&context->logger, Str8_Literal("window"), "failed to create (%s)", SDL->GetError());
        }
    }

    LoggerFlush(&context->logger);

    return result;
}

