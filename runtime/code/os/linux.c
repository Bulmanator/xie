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

#include <dirent.h>

// @copy: from winapi, should be somewhere else for cross-platform
//
#define XI_MAX_TITLE_COUNT 1024

// SDL2 functions and types
//
// @todo: native windowing, sound and input handling? instead of using SDL2
//
#include <SDL2/SDL.h>

typedef void xiSDL2_SDL_SetWindowMinimumSize(SDL_Window *, int, int);
typedef void xiSDL2_SDL_SetWindowMaximumSize(SDL_Window *, int, int);
typedef void xiSDL2_SDL_SetWindowTitle(SDL_Window *, const char *);
typedef void xiSDL2_SDL_PauseAudioDevice(SDL_AudioDeviceID, int);

typedef int xiSDL2_SDL_Init(Uint32);
typedef int xiSDL2_SDL_InitSubSystem(Uint32);
typedef int xiSDL2_SDL_PollEvent(SDL_Event *);
typedef int xiSDL2_SDL_SetWindowFullscreen(SDL_Window *, SDL_bool);
typedef int xiSDL2_SDL_SetWindowResizable(SDL_Window *, SDL_bool);
typedef int xiSDL2_SDL_GetWindowSize(SDL_Window *, int *, int *);
typedef int xiSDL2_SDL_QueueAudio(SDL_AudioDeviceID, const void *, Uint32);
typedef int xiSDL2_SDL_GetNumVideoDisplays(void);
typedef int xiSDL2_SDL_GetDesktopDisplayMode(int, SDL_DisplayMode *);

typedef Uint32 xiSDL2_SDL_GetQueuedAudioSize(SDL_AudioDeviceID);

typedef const char *xiSDL2_SDL_GetDisplayName(int);
typedef const char *xiSDL2_SDL_GetError(void);

typedef SDL_AudioDeviceID xiSDL2_SDL_OpenAudioDevice(const char *, int, const SDL_AudioSpec *, SDL_AudioSpec *, int);

typedef SDL_Window *xiSDL2_SDL_CreateWindow(const char *, int, int, int, int, Uint32);

// gl functions passed to the renderering backend
//
typedef SDL_GLContext xiSDL2_SDL_GL_CreateContext(SDL_Window *window);

typedef void *xiSDL2_SDL_GL_GetProcAddress(const char *);

typedef int  xiSDL2_SDL_GL_SetSwapInterval(int);
typedef int  xiSDL2_SDL_GL_SetAttribute(SDL_GLattr, int);
typedef void xiSDL2_SDL_GL_SwapWindow(SDL_Window *);
typedef void xiSDL2_SDL_GL_DeleteContext(SDL_GLContext);
typedef void xiSDL2_SDL_GL_GetDrawableSize(SDL_Window *window, int *, int *);

#define SDL2_FUNCTION_POINTER(name) xiSDL2_SDL_##name *name

// :renderer_core
//
typedef struct xiSDL2WindowData {
    SDL_Window *window;

    SDL2_FUNCTION_POINTER(GL_CreateContext);
    SDL2_FUNCTION_POINTER(GL_GetProcAddress);
    SDL2_FUNCTION_POINTER(GL_SetSwapInterval);
    SDL2_FUNCTION_POINTER(GL_SetAttribute);
    SDL2_FUNCTION_POINTER(GL_SwapWindow);
    SDL2_FUNCTION_POINTER(GL_DeleteContext);
} xiSDL2WindowData;

typedef struct xiSDL2Context {
    void *lib;
    SDL_Window *window;

    xi_u32 window_state;
    xi_buffer title;

    struct {
        xi_b32 enabled;

        SDL_AudioDeviceID device;
        SDL_AudioSpec format;

        xi_s16 *samples;
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
} xiSDL2Context;

#undef SDL2_FUNCTION_POINTER

typedef struct xiLinuxContext {
    xiArena arena;
    xiLogger logger;

    xiContext xi;

    xiSDL2Context SDL;

    xi_b32 quitting;

    xi_b32 did_update;
    xi_u64 start_ns;
    xi_u64 resolution_ns;

    xi_s64 accum_ns;
    xi_s64 fixed_ns;
    xi_s64 clamp_ns;

    struct {
        xi_b32 valid;
        void *lib;

        xiSDL2WindowData window; // move this to xiSDL2Context
        xiRendererInit *init;
    } renderer;

    struct {
        void *lib;
        xi_u64 time;

        const char *path;

        xiGameCode *code;
    } game;
} xiLinuxContext;

// :note utility functions
//
XI_INTERNAL const char *linux_str_to_cstr(xiArena *arena, xi_string str) {
    char *result = xi_arena_push_size(arena, str.count + 1);

    xi_memory_copy(result, str.data, str.count);
    result[str.count] = 0; // null-terminate

    return result;
}

// :note linux memory allocation functions
//
xi_uptr xi_os_allocation_granularity_get() {
    xi_uptr result = sysconf(_SC_PAGE_SIZE);
    return result;
}

xi_uptr xi_os_page_size_get() {
        xi_uptr result = sysconf(_SC_PAGE_SIZE);
    return result;
}

void *xi_os_virtual_memory_reserve(xi_uptr size) {
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (result == MAP_FAILED) { result = 0; }
    return result;
}

xi_b32 xi_os_virtual_memory_commit(void *base, xi_uptr size) {
    xi_b32 result = (mprotect(base, size, PROT_READ | PROT_WRITE) == 0);
    return result;
}

void xi_os_virtual_memory_decommit(void *base, xi_uptr size) {
    // sigh.. linux is really bad for this, it doesn't let you explicitly
    // commit and decommit pages, you just have to pay for the page fault,
    // our system relies on the fact tha unmapped pages are zeroed by default
    // for security reasons and apparently this is the only performant way to
    // do this on linux
    //
    memset(base, 0, size);
}

void xi_os_virtual_memory_release(void *base, xi_uptr size) {
    munmap(base, size);
}

// :note linux threading
//
XI_INTERNAL void *linux_thread_proc(void *arg) {
    xiThreadQueue *queue = (xiThreadQueue *) arg;
    xi_thread_run(queue);

    return 0;
}

void xi_os_thread_start(xiThreadQueue *arg) {
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);

    pthread_t id;
    int result = pthread_create(&id, &attrs, linux_thread_proc, (void *) arg);
    if (result != 0) {
        // @todo: logging...
        //
    }

    pthread_attr_destroy(&attrs);
}

void xi_os_futex_wait(xiFutex *futex, xiFutex value) {
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

void xi_os_futex_signal(xiFutex *futex) {
    syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0, 0);
}

void xi_os_futex_broadcast(xiFutex *futex) {
    syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX, 0, 0, 0);
}

// :note linux file system functions
//
enum xiLinuxPathType {
    LINUX_PATH_TYPE_EXECUTABLE = 0,
    LINUX_PATH_TYPE_WORKING,
    LINUX_PATH_TYPE_USER,
    LINUX_PATH_TYPE_TEMP
};

XI_INTERNAL const char *linux_system_path_get(xi_u32 type) {
    const char *result = "/tmp"; // on error, not much we can do use /tmp

    xiArena *temp = xi_temp_get();

    switch (type) {
        case LINUX_PATH_TYPE_EXECUTABLE: {
            // ENAMETOOLONG can occur, readlinkat instead?
            //
            char *path = xi_arena_push_size(temp, PATH_MAX + 1);
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
                xi_arena_pop_size(temp, PATH_MAX - len);
            }
            else {
                xi_arena_pop_size(temp, PATH_MAX + 1); // error remove everything
            }
        }
        break;
        case LINUX_PATH_TYPE_WORKING: {
            result = xi_arena_push_size(temp, PATH_MAX + 1);
            getcwd((char *) result, PATH_MAX);
        }
        break;
        case LINUX_PATH_TYPE_USER: {
            uid_t uid = getuid();
            struct passwd *pwd = getpwuid(uid);
            if (pwd) {
                const char *home = pwd->pw_dir;
                xi_string user_dir = xi_str_format(temp, "%s/.local/share", home);
                xi_os_directory_create(user_dir);

                result = linux_str_to_cstr(temp, user_dir);
            }
        }
        break;
        case LINUX_PATH_TYPE_TEMP: {
            // pretty sure this is the temp directory on all linux platforms
            //
            result = linux_str_to_cstr(temp, xi_str_wrap_const("/tmp"));
        }
        break;
    }

    return result;
}

XI_INTERNAL char *linux_realpath_get(xi_string path) {
    char *result;

    xiArena *temp = xi_temp_get();

    // once again linux people showing they are the masters of horrible api design,
    // realpath fails if the specified path doesn't exist which is absurd because you
    // _may well want_ to canonicalise a path that doesn't yet exist, say to create a file
    // or directory at said path, but you were handed a relative path.
    //
    //

    if (path.count == 0) {
        // @todo: maybe this should be interpreted as 'the working directory'
        //
        result = "";
    }
    else {
        result = xi_arena_push_size(temp, PATH_MAX + 1);

        const char *cpath;
        if (path.data[0] == '/') {
            // is aboslute just copy the entire path
            //
            cpath = linux_str_to_cstr(temp, path);
        }
        else {
            // may be relative
            //
            const char *exe_path = linux_system_path_get(LINUX_PATH_TYPE_WORKING);
            xi_string full_path = xi_str_format(temp, "%s/%.*s", exe_path, xi_str_unpack(path));

            cpath = linux_str_to_cstr(temp, full_path);
        }

        xi_uptr count = 0;
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

        xi_arena_pop_size(temp, PATH_MAX - count);
    }

    return result;
}

void linux_directory_list_builder_get_recursive(xiArena *arena,
        xiDirectoryListBuilder *builder, const char *path, xi_b32 recursive)
{
    // @todo: for full path length support (as there is technically no
    // defined limit) we should probably be using fopendir with openat, not
    // sure how this will interact with realpath
    //
    DIR *dir = opendir(path);
    if (dir) {
        for (struct dirent *entry = readdir(dir);
                entry != 0;
                entry = readdir(dir))
        {
            // ignore hidden files
            //
            if (entry->d_name[0] == '.') { continue; } // :cf not right

            xi_string full_path = xi_str_format(arena, "%s/%s", path, entry->d_name);

            xiArena *temp = xi_temp_get();
            const char *new_path = linux_str_to_cstr(temp, full_path);
            struct stat sb;
            if (stat(new_path, &sb) == 0) {
                xi_b32 is_dir = (sb.st_mode & S_IFMT) == S_IFDIR;
                xi_u64 time   = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;

                xi_uptr last_slash = 0;
                xi_str_find_last(full_path, &last_slash, '/');
                xi_string basename = xi_str_advance(full_path, last_slash + 1);

                xi_uptr last_dot = 0;
                if (xi_str_find_last(basename, &last_dot, '.')) {
                    // dot may not be found in the case of directory names and files
                    // without extensions
                    //
                    basename = xi_str_prefix(basename, last_dot);
                }

                xiDirectoryEntry entry;
                entry.type     = is_dir ? XI_DIRECTORY_ENTRY_TYPE_DIRECTORY : XI_DIRECTORY_ENTRY_TYPE_FILE;
                entry.path     = full_path;
                entry.basename = basename;
                entry.size     = sb.st_size;
                entry.time     = time;

                xi_directory_list_builder_append(builder, &entry);

                if (recursive && is_dir) {
                    linux_directory_list_builder_get_recursive(arena,
                            builder, new_path, recursive);
                }
            }
        }
    }
}

void xi_os_directory_list_build(xiArena *arena,
        xiDirectoryListBuilder *builder, xi_string path, xi_b32 recursive)
{
    const char *cpath = linux_realpath_get(path);
    if (cpath != 0) {
        linux_directory_list_builder_get_recursive(arena, builder, cpath, recursive);
    }
}

xi_b32 xi_os_directory_entry_get_by_path(xiArena *arena,
        xiDirectoryEntry *entry, xi_string path)
{
    xi_b32 result = false;

    const char *cpath = linux_realpath_get(path);

    if (cpath != 0) {
        struct stat sb;
        if (stat(cpath, &sb) == 0) {
            xi_b32 is_dir = (sb.st_mode & S_IFMT) == S_IFDIR;
            xi_u64 time   = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
            xi_uptr last_slash = 0;

            xi_str_find_last(path, &last_slash, '/');
            xi_string basename = xi_str_advance(path, last_slash + 1);

            xi_uptr last_dot = 0;
            if (xi_str_find_last(basename, &last_dot, '.')) {
                // dot may not be found in the case of directory names and files
                // without extensions
                //
                basename = xi_str_prefix(basename, last_dot);
            }

            entry->type     = is_dir ? XI_DIRECTORY_ENTRY_TYPE_DIRECTORY : XI_DIRECTORY_ENTRY_TYPE_FILE;
            entry->path     = path;
            entry->basename = basename;
            entry->size     = sb.st_size;
            entry->time     = time;

            result = true;
        }
    }

    return result;
}

xi_b32 xi_os_directory_create(xi_string path) {
    xi_b32 result = true;

    char *cpath = linux_realpath_get(path);

    // there will always be a fowrard slash as linux_realpath_get converts to absolute
    //
    xi_uptr it = 1;
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

void xi_os_directory_delete(xi_string path) {
    char *cpath = linux_realpath_get(path);
    rmdir(cpath);
}

xi_b32 xi_os_file_open(xiFileHandle *handle, xi_string path, xi_u32 access) {
    xi_b32 result = true;

    handle->status = XI_FILE_HANDLE_STATUS_VALID;
    handle->os     = 0;

    char *cpath = linux_realpath_get(path);

    int flags = 0;
    if (access == XI_FILE_ACCESS_FLAG_READWRITE) {
        flags = O_RDWR | O_CREAT;
    }
    else if (access == XI_FILE_ACCESS_FLAG_READ) {
        flags = O_RDONLY;
    }
    else if (access == XI_FILE_ACCESS_FLAG_WRITE) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    }

    int fd = open(cpath, flags, 0644);

    *(int *) &handle->os = fd; // :strict_aliasing

    if (fd < 0) {
        handle->status = XI_FILE_HANDLE_STATUS_FAILED_OPEN;
        result = false;
    }

    return result;
}

void xi_os_file_close(xiFileHandle *handle) {
    int fd = *(int *) &handle->os; // :strict_aliasing
    if (fd >= 0) {
        close(fd);
    }

    handle->status = XI_FILE_HANDLE_STATUS_CLOSED;
}

void xi_os_file_delete(xi_string path) {
    char *cpath = linux_realpath_get(path);
    unlink(cpath);
}

xi_b32 xi_os_file_read(xiFileHandle *handle, void *dst, xi_uptr offset, xi_uptr size) {
    xi_b32 result = false;

    if (handle->status == XI_FILE_HANDLE_STATUS_VALID) {
        result = true;

        int fd = *(int *) &handle->os; // :strict_aliasing

        XI_ASSERT(fd >= 0);
        XI_ASSERT(offset != XI_FILE_OFFSET_APPEND);

        xi_u8 *data = (xi_u8 *) dst;
        while (size > 0) {
            int nread = pread(fd, data, size, offset);
            if (nread < 0) {
                result = false;
                handle->status = XI_FILE_HANDLE_STATUS_FAILED_READ;
                break;
            }

            XI_ASSERT(size >= nread);

            data   += nread;
            offset += nread;
            size   -= nread;
        }
    }

    return result;
}

xi_b32 xi_os_file_write(xiFileHandle *handle, void *src, xi_uptr offset, xi_uptr size) {
    xi_b32 result = false;
    if (handle->status == XI_FILE_HANDLE_STATUS_VALID) {
        result = true;

        int fd = *(int *) &handle->os; // :strict_aliasing

        XI_ASSERT(fd >= 0);

        if (offset == XI_FILE_OFFSET_APPEND) {
            lseek(fd, 0, SEEK_END);

            // we are appending to the end so just use normal write
            //
            xi_u8 *data = (xi_u8 *) src;
            while (size > 0) {
                int nwritten = write(fd, data, size);
                if (nwritten < 0) {
                    result = false;
                    handle->status = XI_FILE_HANDLE_STATUS_FAILED_WRITE;
                    break;
                }

                XI_ASSERT(size >= nwritten);

                data   += nwritten;
                offset += nwritten;
                size   -= nwritten;
            }
        }
        else {
            // specific offset use pwrite
            //
            xi_u8 *data = (xi_u8 *) src;
            while (size > 0) {
                int nwritten = pwrite(fd, data, size, offset);
                if (nwritten < 0) {
                    result = false;
                    handle->status = XI_FILE_HANDLE_STATUS_FAILED_WRITE;
                    break;
                }

                XI_ASSERT(size >= nwritten);

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
XI_INTERNAL void linux_game_code_init(xiLinuxContext *context) {
    xiGameCode *game = context->game.code;

    if (game->dynamic) {
        xiArena *temp = xi_temp_get();

        const char *cexe_dir = linux_system_path_get(LINUX_PATH_TYPE_EXECUTABLE);
        xi_string exe_dir    = xi_str_wrap_cstr(cexe_dir);

        xiDirectoryList all;
        xi_directory_list_get(temp, &all, exe_dir, false);

        xiDirectoryList so_list;
        xi_directory_list_filter_for_extension(temp, &so_list, &all, xi_str_wrap_const(".so"));

        xi_b32 found = false;
        for (xi_u32 it = 0; it < so_list.count; ++it) {
            xiDirectoryEntry *entry = &so_list.entries[it];

            const char *path = linux_str_to_cstr(temp, entry->path);
            void *lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
            if (lib) {
                xiGameInit     *init     = dlsym(lib, "__xi_game_init");
                xiGameSimulate *simulate = dlsym(lib, "__xi_game_simulate");
                xiGameRender   *render   = dlsym(lib, "__xi_game_render");

                if (init != 0 && simulate != 0 && render != 0) {
                    context->game.path = linux_str_to_cstr(&context->arena, entry->path);


                    struct stat sb;
                    if (stat(context->game.path, &sb) == 0) {
                        context->game.lib = lib;

                        game->init     = init;
                        game->simulate = simulate;
                        game->render   = render;

                        context->game.time = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
                        found = true;
                    }

                }
            }

            if (found) { break; }
        }
    }

    if (!game->init)     { game->init     = __xi_game_init;     }
    if (!game->simulate) { game->simulate = __xi_game_simulate; }
    if (!game->render)   { game->render   = __xi_game_render;   }
}

XI_INTERNAL void linux_game_code_reload(xiLinuxContext *context) {
    struct stat sb;
    if (stat(context->game.path, &sb) == 0) {
        xiGameCode *game = context->game.code;

        xi_u64 time = (1000000000 * sb.st_mtim.tv_sec) + sb.st_mtim.tv_nsec;
        if (time != context->game.time) {
            if (context->game.lib) {
                dlclose(context->game.lib);

                game->init     = __xi_game_init;
                game->simulate = __xi_game_simulate;
                game->render   = __xi_game_render;
            }

            void *new_lib = dlopen(context->game.path, RTLD_LAZY | RTLD_LOCAL);
            if (new_lib) {
                xiGameInit     *init     = dlsym(new_lib, "__xi_game_init");
                xiGameSimulate *simulate = dlsym(new_lib, "__xi_game_simulate");
                xiGameRender   *render   = dlsym(new_lib, "__xi_game_render");

                if (init && simulate && render) {
                    context->game.lib  = new_lib;
                    context->game.time = time;

                    game->init     = init;
                    game->simulate = simulate;
                    game->render   = render;

                    // call the init function of the game to notify it has been reloaded
                    //
                    game->init(&context->xi, XI_GAME_RELOADED);
                }
            }
        }
    }
}

XI_INTERNAL xiSDL2Context *linux_sdl2_load(xiLinuxContext *context) {
    xiSDL2Context *result = 0;

    xiSDL2Context *SDL = &context->SDL;
    SDL->lib = dlopen("libSDL2.so", RTLD_LAZY | RTLD_LOCAL);
    if (!SDL->lib) {
        xi_log(&context->logger, "init", "failed to find libSDL2.so globally... trying locally");

        const char *exe_dir = linux_system_path_get(LINUX_PATH_TYPE_EXECUTABLE);

        xiArena *temp = xi_temp_get();

        xi_string local_sdl = xi_str_format(temp, "%s/libSDL2.so", exe_dir);
        const char *cpath   = linux_str_to_cstr(temp, local_sdl);

        SDL->lib = dlopen(cpath, RTLD_NOW | RTLD_GLOBAL);
    }

    if (SDL->lib) {
        xiSDL2WindowData *window = &context->renderer.window;

#define SDL2_FUNCTION_LOAD(x, name) (x)->name = (xiSDL2_SDL_##name *) dlsym(SDL->lib, XI_STRINGIFY(SDL_##name)); XI_ASSERT((x)->name != 0)
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
#undef  SDL2_FUNCTION_LOAD

        if (SDL->Init(SDL_INIT_VIDEO) == 0) {
            result = SDL;
        }
        else {
            xi_log(&context->logger, "init", "failed to initialise (%s)", SDL->GetError());
        }
    }
    else {
        xi_log(&context->logger, "init", "failed to load libSDL2.so");
    }

    return result;
}

XI_INTERNAL void linux_sdl2_displays_enumerate(xiLinuxContext *context) {
    xiSDL2Context *SDL = &context->SDL;

    int count = SDL->GetNumVideoDisplays();
    if (count > 0) {
        count = XI_MIN(count, XI_MAX_DISPLAYS);

        xiContext *xi = &context->xi;
        for (int it = 0; it < count; ++it) {
            xiDisplay *display = &xi->system.displays[xi->system.display_count];

            SDL_DisplayMode mode;
            if (SDL->GetDesktopDisplayMode(it, &mode) == 0) {
                const char *cname = SDL->GetDisplayName(it);
                if (cname != 0) {
                    xi_string name = xi_str_wrap_cstr(cname);
                    display->name  = xi_str_copy(&context->arena, name);
                }

                // @incomplete: sdl doesn't provide a way to calculate scale without first creating a
                // window and checking the differences between GetDrawableSize and GetWindowSize
                //
                display->width        = mode.w;
                display->height       = mode.h;
                display->refresh_rate = mode.refresh_rate;
                display->scale        = 1.0f;

                xi->system.display_count += 1;
            }
        }
    }
}

XI_INTERNAL void linux_sdl2_audio_init(xiLinuxContext *context) {
    xiSDL2Context *SDL = &context->SDL;

    xiAudioPlayer *player = &context->xi.audio_player;
    xi_audio_player_init(&context->arena, player);

    if (SDL->InitSubSystem(SDL_INIT_AUDIO) == 0) {
        int flags = SDL_AUDIO_ALLOW_SAMPLES_CHANGE;

        SDL_AudioSpec format = { 0 };
        format.freq     = player->sample_rate;
        format.format   = AUDIO_S16LSB;
        format.channels = 2;
        format.samples  = xi_pow2_nearest_u32(player->frame_count);
        format.callback = 0;
        format.userdata = 0;

        SDL_AudioSpec obtained;

        SDL->audio.device = SDL->OpenAudioDevice(0, SDL_FALSE, &format, &obtained, flags);
        if (SDL->audio.device != 0) {
            SDL->audio.format  = obtained;
            SDL->audio.samples = xi_arena_push_array(&context->arena, xi_s16, obtained.samples << 1);

            SDL->PauseAudioDevice(SDL->audio.device, 0);

            SDL->audio.enabled = true;
        }
        else {
            xi_log(&context->logger, "audio", "no device found... disabling audio");
        }
    }
    else {
        xi_log(&context->logger, "audio", "failed to initialise (%s)", SDL->GetError());
    }

}

XI_INTERNAL void linux_sdl2_audio_mix(xiLinuxContext *context) {
    xiSDL2Context *SDL = &context->SDL;
    xiContext *xi = &context->xi;

    if (SDL->audio.enabled) {
        // @hardcode: divide by 4 as a single frame is (channels * sizeof(sample)) which
        // in our case is only supported to be (2 * sizeof(s16)) == 4
        //
        // we will probably want to support more channels, and maybe even different sample
        // formats in the future
        //
        // :frame_count
        //
        xi_u32 max_frames    = SDL->audio.format.samples;
        xi_u32 queued_frames = SDL->GetQueuedAudioSize(SDL->audio.device) >> 2;
        XI_ASSERT(queued_frames <= max_frames);

        xi_u32 frame_count = max_frames - queued_frames;
        if (frame_count > 0) {
            xi_f32 dt = (xi_f32) xi->time.delta.s;
            xi_audio_player_update(&xi->audio_player, &xi->assets, SDL->audio.samples, frame_count, dt);

            SDL->QueueAudio(SDL->audio.device, SDL->audio.samples, frame_count << 2); // :frame_count
        }
    }
}

XI_GLOBAL xi_u8 tbl_sdl2_scancode_to_input_key[] = {
    [SDL_SCANCODE_UNKNOWN] = XI_KEYBOARD_KEY_UNKNOWN,

    // general keys
    //
    [SDL_SCANCODE_RETURN]    = XI_KEYBOARD_KEY_RETURN,
    [SDL_SCANCODE_ESCAPE]    = XI_KEYBOARD_KEY_ESCAPE,
    [SDL_SCANCODE_BACKSPACE] = XI_KEYBOARD_KEY_BACKSPACE,
    [SDL_SCANCODE_TAB]       = XI_KEYBOARD_KEY_TAB,
    [SDL_SCANCODE_HOME]      = XI_KEYBOARD_KEY_HOME,
    [SDL_SCANCODE_PAGEUP]    = XI_KEYBOARD_KEY_PAGEUP,
    [SDL_SCANCODE_DELETE]    = XI_KEYBOARD_KEY_DELETE,
    [SDL_SCANCODE_END]       = XI_KEYBOARD_KEY_END,
    [SDL_SCANCODE_PAGEDOWN]  = XI_KEYBOARD_KEY_PAGEDOWN,

    // arrow keys
    //
    [SDL_SCANCODE_RIGHT] = XI_KEYBOARD_KEY_RIGHT, [SDL_SCANCODE_LEFT] = XI_KEYBOARD_KEY_LEFT,
    [SDL_SCANCODE_DOWN]  = XI_KEYBOARD_KEY_DOWN,  [SDL_SCANCODE_UP]   = XI_KEYBOARD_KEY_UP,

    // f keys
    //
    [SDL_SCANCODE_F1]  = XI_KEYBOARD_KEY_F1,  [SDL_SCANCODE_F2]  = XI_KEYBOARD_KEY_F2,
    [SDL_SCANCODE_F3]  = XI_KEYBOARD_KEY_F3,  [SDL_SCANCODE_F4]  = XI_KEYBOARD_KEY_F4,
    [SDL_SCANCODE_F5]  = XI_KEYBOARD_KEY_F5,  [SDL_SCANCODE_F6]  = XI_KEYBOARD_KEY_F6,
    [SDL_SCANCODE_F7]  = XI_KEYBOARD_KEY_F7,  [SDL_SCANCODE_F8]  = XI_KEYBOARD_KEY_F8,
    [SDL_SCANCODE_F9]  = XI_KEYBOARD_KEY_F9,  [SDL_SCANCODE_F10] = XI_KEYBOARD_KEY_F10,
    [SDL_SCANCODE_F11] = XI_KEYBOARD_KEY_F11, [SDL_SCANCODE_F12] = XI_KEYBOARD_KEY_F12,

    // more general
    //
    [SDL_SCANCODE_INSERT]     = XI_KEYBOARD_KEY_INSERT,
    [SDL_SCANCODE_SPACE]      = XI_KEYBOARD_KEY_SPACE,
    [SDL_SCANCODE_APOSTROPHE] = XI_KEYBOARD_KEY_QUOTE,
    [SDL_SCANCODE_COMMA]      = XI_KEYBOARD_KEY_COMMA,
    [SDL_SCANCODE_MINUS]      = XI_KEYBOARD_KEY_MINUS,
    [SDL_SCANCODE_PERIOD]     = XI_KEYBOARD_KEY_PERIOD,
    [SDL_SCANCODE_SLASH]      = XI_KEYBOARD_KEY_SLASH,

    // numbers
    //
    [SDL_SCANCODE_0] = XI_KEYBOARD_KEY_0, [SDL_SCANCODE_1] = XI_KEYBOARD_KEY_1,
    [SDL_SCANCODE_2] = XI_KEYBOARD_KEY_2, [SDL_SCANCODE_3] = XI_KEYBOARD_KEY_3,
    [SDL_SCANCODE_4] = XI_KEYBOARD_KEY_4, [SDL_SCANCODE_5] = XI_KEYBOARD_KEY_5,
    [SDL_SCANCODE_6] = XI_KEYBOARD_KEY_6, [SDL_SCANCODE_7] = XI_KEYBOARD_KEY_7,
    [SDL_SCANCODE_8] = XI_KEYBOARD_KEY_8, [SDL_SCANCODE_9] = XI_KEYBOARD_KEY_9,

    // more more general
    //
    [SDL_SCANCODE_SEMICOLON] = XI_KEYBOARD_KEY_SEMICOLON,
    [SDL_SCANCODE_EQUALS]    = XI_KEYBOARD_KEY_EQUALS,

    // modifiers
    //
    [SDL_SCANCODE_LSHIFT] = XI_KEYBOARD_KEY_LSHIFT,
    [SDL_SCANCODE_LCTRL]  = XI_KEYBOARD_KEY_LCTRL,
    [SDL_SCANCODE_LALT]   = XI_KEYBOARD_KEY_LALT,

    [SDL_SCANCODE_RSHIFT] = XI_KEYBOARD_KEY_RSHIFT,
    [SDL_SCANCODE_RCTRL]  = XI_KEYBOARD_KEY_RCTRL,
    [SDL_SCANCODE_RALT]   = XI_KEYBOARD_KEY_RALT,

    // more more more general
    //
    [SDL_SCANCODE_LEFTBRACKET]  = XI_KEYBOARD_KEY_LBRACKET,
    [SDL_SCANCODE_BACKSLASH]    = XI_KEYBOARD_KEY_BACKSLASH,
    [SDL_SCANCODE_RIGHTBRACKET] = XI_KEYBOARD_KEY_RBRACKET,
    [SDL_SCANCODE_GRAVE]        = XI_KEYBOARD_KEY_GRAVE,

    // letters
    //
    [SDL_SCANCODE_A] = XI_KEYBOARD_KEY_A, [SDL_SCANCODE_B] = XI_KEYBOARD_KEY_B,
    [SDL_SCANCODE_C] = XI_KEYBOARD_KEY_C, [SDL_SCANCODE_D] = XI_KEYBOARD_KEY_D,
    [SDL_SCANCODE_E] = XI_KEYBOARD_KEY_E, [SDL_SCANCODE_F] = XI_KEYBOARD_KEY_F,
    [SDL_SCANCODE_G] = XI_KEYBOARD_KEY_G, [SDL_SCANCODE_H] = XI_KEYBOARD_KEY_H,
    [SDL_SCANCODE_I] = XI_KEYBOARD_KEY_I, [SDL_SCANCODE_J] = XI_KEYBOARD_KEY_J,
    [SDL_SCANCODE_K] = XI_KEYBOARD_KEY_K, [SDL_SCANCODE_L] = XI_KEYBOARD_KEY_L,
    [SDL_SCANCODE_M] = XI_KEYBOARD_KEY_M, [SDL_SCANCODE_N] = XI_KEYBOARD_KEY_N,
    [SDL_SCANCODE_O] = XI_KEYBOARD_KEY_O, [SDL_SCANCODE_P] = XI_KEYBOARD_KEY_P,
    [SDL_SCANCODE_Q] = XI_KEYBOARD_KEY_Q, [SDL_SCANCODE_R] = XI_KEYBOARD_KEY_R,
    [SDL_SCANCODE_S] = XI_KEYBOARD_KEY_S, [SDL_SCANCODE_T] = XI_KEYBOARD_KEY_T,
    [SDL_SCANCODE_U] = XI_KEYBOARD_KEY_U, [SDL_SCANCODE_V] = XI_KEYBOARD_KEY_V,
    [SDL_SCANCODE_W] = XI_KEYBOARD_KEY_W, [SDL_SCANCODE_X] = XI_KEYBOARD_KEY_X,
    [SDL_SCANCODE_Y] = XI_KEYBOARD_KEY_Y, [SDL_SCANCODE_Z] = XI_KEYBOARD_KEY_Z
};

XI_INTERNAL void linux_xi_context_update(xiLinuxContext *context) {
    xiContext *xi = &context->xi;
    xiSDL2Context *SDL = &context->SDL;

    struct timespec timer_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &timer_end);

    xi_u64 end_ns   = (1000000000 * timer_end.tv_sec) + timer_end.tv_nsec;
    xi_u64 delta_ns = end_ns - context->start_ns;

    delta_ns = XI_MIN(delta_ns, context->clamp_ns);

    context->accum_ns += delta_ns;

    xi->time.ticks = end_ns;
    context->start_ns = end_ns;

    xi->time.delta.ns = (context->fixed_ns);
    xi->time.delta.us = (context->fixed_ns / 1000);
    xi->time.delta.ms = (context->fixed_ns / 1000000);
    xi->time.delta.s  = (context->fixed_ns / 1000000000.0);

    xi->time.total.ns += (delta_ns);
    xi->time.total.us  = (xi_u64) ((xi->time.total.ns / 1000.0) + 0.5);
    xi->time.total.ms  = (xi_u64) ((xi->time.total.ns / 1000000.0) + 0.5);
    xi->time.total.s   = (xi->time.total.ns / 1000000000.0);

    linux_sdl2_audio_mix(context);

    // process windows messages that our application received
    //
    xiInputMouse *mouse = &xi->mouse;
    xiInputKeyboard *kb = &xi->keyboard;

    // :step_input
    //
    if (context->did_update) {
        // reset mouse params for this frame
        //
        mouse->connected    = true;
        mouse->active       = false;
        mouse->delta.screen = xi_v2s_create(0, 0);
        mouse->delta.clip   =  xi_v2_create(0, 0);
        mouse->delta.wheel  =  xi_v2_create(0, 0);

        mouse->left.pressed   = mouse->left.released   = false;
        mouse->middle.pressed = mouse->middle.released = false;
        mouse->right.pressed  = mouse->right.released  = false;
        mouse->x0.pressed     = mouse->x0.released     = false;
        mouse->x1.pressed     = mouse->x1.released     = false;

        // reset keyboard params for this frame
        //
        kb->connected = true;
        kb->active    = false;

        for (xi_u32 it = 0; it < XI_KEYBOARD_KEY_COUNT; ++it) {
            kb->keys[it].pressed  = false;
            kb->keys[it].released = false;
        }
    }

    context->did_update = false;

    // we don't handle programatic resizes if the user has manually resized the window
    //
    xi_b32 user_resize = false;

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
                xi_b32 down     = (event.type == SDL_KEYDOWN);
                xi_u32 scancode = event.key.keysym.scancode;

                if (scancode < XI_ARRAY_SIZE(tbl_sdl2_scancode_to_input_key)) {
                    xi_u32 index = tbl_sdl2_scancode_to_input_key[scancode];

                    xiInputButton *key = &kb->keys[index];
                    xi_input_button_handle(key, down);
                }

                // handle modifiers for easy access booleans
                //
                switch (scancode) {
                    case SDL_SCANCODE_LALT:
                    case SDL_SCANCODE_RALT: {
                        xiInputButton *left  = &kb->keys[XI_KEYBOARD_KEY_LALT];
                        xiInputButton *right = &kb->keys[XI_KEYBOARD_KEY_RALT];

                        kb->alt = (left->down || right->down);
                    }
                    break;
                    case SDL_SCANCODE_LCTRL:
                    case SDL_SCANCODE_RCTRL: {
                        xiInputButton *left  = &kb->keys[XI_KEYBOARD_KEY_LCTRL];
                        xiInputButton *right = &kb->keys[XI_KEYBOARD_KEY_RCTRL];

                        kb->ctrl = (left->down || right->down);
                    }
                    break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT: {
                        xiInputButton *left  = &kb->keys[XI_KEYBOARD_KEY_LSHIFT];
                        xiInputButton *right = &kb->keys[XI_KEYBOARD_KEY_RSHIFT];

                        kb->shift = (left->down || right->down);
                    }
                    break;
                }

                kb->active = true;
            }
            break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN: {
                xiInputButton *button = 0;

                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:   { button = &mouse->left;   } break;
                    case SDL_BUTTON_MIDDLE: { button = &mouse->middle; } break;
                    case SDL_BUTTON_RIGHT:  { button = &mouse->right;  } break;
                    case SDL_BUTTON_X1:     { button = &mouse->x0;     } break;
                    case SDL_BUTTON_X2:     { button = &mouse->x1;     } break;
                    default: {} break;
                }

                if (button) {
                    xi_b32 down = (event.type == SDL_MOUSEBUTTONDOWN);
                    xi_input_button_handle(button, down);

                    mouse->active = true;
                }
            }
            break;
            case SDL_MOUSEMOTION: {
                xi_v2s pt = xi_v2s_create(event.motion.x, event.motion.y);

                xi_v2s old_screen = mouse->position.screen;
                xi_v2  old_clip   = mouse->position.clip;

                // update screen position
                //
                mouse->position.screen = pt;

                // update clip position
                //
                mouse->position.clip.x = -1.0f + (2.0f * (pt.x / (xi_f32) window_width));
                mouse->position.clip.y =  1.0f - (2.0f * (pt.y / (xi_f32) window_height));

                xi_v2s delta_screen = xi_v2s_sub(mouse->position.screen, old_screen);
                xi_v2  delta_clip   =  xi_v2_sub(mouse->position.clip,   old_clip);

                mouse->delta.screen = xi_v2s_add(mouse->delta.screen, delta_screen);
                mouse->delta.clip   =  xi_v2_add(mouse->delta.clip,   delta_clip);

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

    if (!xi_str_equal(xi->window.title, SDL->title.str)) {
        xi_uptr count = XI_MIN(SDL->title.limit, xi->window.title.count);

        xi_memory_copy(SDL->title.data, xi->window.title.data, count);
        SDL->title.used = count;

        xiArena *temp = xi_temp_get();
        const char *title = linux_str_to_cstr(temp, SDL->title.str);
        SDL->SetWindowTitle(SDL->window, title);
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
                case XI_WINDOW_STATE_WINDOWED:
                case XI_WINDOW_STATE_WINDOWED_RESIZABLE:
                case XI_WINDOW_STATE_WINDOWED_BORDERLESS: {
                    SDL->SetWindowFullscreen(SDL->window, SDL_FALSE);

                    if (xi->window.state == XI_WINDOW_STATE_WINDOWED_RESIZABLE) {
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
                case XI_WINDOW_STATE_FULLSCREEN: {
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

    xiGameCode *game = context->game.code;
    if (game->dynamic) {
        linux_game_code_reload(context);
    }
}

extern int __xie_bootstrap_run(xiGameCode *game) {
    int result = 1;

    xiFileHandle cout = { XI_FILE_HANDLE_STATUS_VALID, (void *) STDOUT_FILENO };
    xiFileHandle cerr = { XI_FILE_HANDLE_STATUS_VALID, (void *) STDERR_FILENO };

    xiLinuxContext *context;
    {
        xiArena arena;
        xi_arena_init_virtual(&arena, XI_GB(4));

        context = xi_arena_push_type(&arena, xiLinuxContext);
        context->arena = arena;

        xi_logger_create(&context->arena, &context->logger, cout, XI_KB(512));
    }

    xiSDL2Context *SDL = linux_sdl2_load(context); // also stored context->SDL
    if (SDL) {
        xiContext *xi = &context->xi;

        linux_sdl2_displays_enumerate(context);

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
        SDL->title.limit  = XI_MAX_TITLE_COUNT;
        SDL->title.data   = xi_arena_push_size(&context->arena, SDL->title.limit);

        xi->window.title = SDL->title.str;

        const char *exe_path     = linux_system_path_get(LINUX_PATH_TYPE_EXECUTABLE);
        const char *temp_path    = linux_system_path_get(LINUX_PATH_TYPE_TEMP);
        const char *user_path    = linux_system_path_get(LINUX_PATH_TYPE_USER);
        const char *working_path = linux_system_path_get(LINUX_PATH_TYPE_WORKING);

        xi->system.executable_path = xi_str_copy(&context->arena, xi_str_wrap_cstr(exe_path));
        xi->system.temp_path       = xi_str_copy(&context->arena, xi_str_wrap_cstr(temp_path));
        xi->system.user_path       = xi_str_copy(&context->arena, xi_str_wrap_cstr(user_path));
        xi->system.working_path    = xi_str_copy(&context->arena, xi_str_wrap_cstr(working_path));

        xi->system.processor_count = sysconf(_SC_NPROCESSORS_ONLN); // vs _CONF?

        linux_game_code_init(context);

        game->init(xi, XI_ENGINE_CONFIGURE);

        if (xi->window.width == 0 || xi->window.height == 0) {
            xi->window.width  = 1280;
            xi->window.height = 720;
        }

        int display = XI_MIN(xi->window.display, xi->system.display_count);

        int p = SDL_WINDOWPOS_CENTERED_DISPLAY(display);

        int w = xi->window.width;
        int h = xi->window.height;

        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

        if (xi->window.state == XI_WINDOW_STATE_FULLSCREEN) {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        else if (xi->window.state == XI_WINDOW_STATE_WINDOWED_RESIZABLE) {
            flags |= SDL_WINDOW_RESIZABLE;
        }

        const char *title = "xie";
        if (xi_str_is_valid(xi->window.title)) {
            xi_uptr count = XI_MIN(xi->window.title.count, SDL->title.limit);

            xi->window.title.count = count;

            xiArena *temp = xi_temp_get();
            title = linux_str_to_cstr(temp, xi->window.title);

            xi_memory_copy(SDL->title.data, xi->window.title.data, count);
            SDL->title.used = count;
        }

        SDL->window = SDL->CreateWindow(title, p, p, w, h, flags);
        if (SDL->window) {
            if (xi->window.state != XI_WINDOW_STATE_WINDOWED_RESIZABLE) {
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

            xiRenderer *renderer = &xi->renderer;
            {
                renderer->setup.window_dim.width  = xi->window.w;
                renderer->setup.window_dim.height = xi->window.h;

                xiArena *temp = xi_temp_get();
                xi_string renderer_path = xi_str_format(temp, "%s/xi_opengld.so", exe_path);

                const char *path = linux_str_to_cstr(temp, renderer_path);
                context->renderer.lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
                if (context->renderer.lib) {
                    context->renderer.init = dlsym(context->renderer.lib, "xi_opengl_init");
                    if (context->renderer.init) {
                        xiSDL2WindowData *window_data = &context->renderer.window;
                        window_data->window = SDL->window;

                        context->renderer.valid = context->renderer.init(renderer, window_data);
                    }
                    else {
                        xi_log(&context->logger, "renderer", "xi_opengl_init not found");
                    }
                }
                else {
                    xi_log(&context->logger, "renderer", "failed to load xi_opengld.so");
                }
            }

            if (context->renderer.valid) {
                linux_sdl2_audio_init(context);

                xi_u32 n_processors = xi->system.processor_count;
                xi_thread_pool_init(&context->arena, &xi->thread_pool, n_processors);
                xi_asset_manager_init(&context->arena, &xi->assets, xi);

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

                    xi_u64 fixed_ns   = (1000000000 / xi->time.delta.fixed_hz);
                    context->fixed_ns = fixed_ns;

                    if (xi->time.delta.clamp_s <= 0.0f) {
                        xi->time.delta.clamp_s = 0.2f;
                        context->clamp_ns = 200000000;
                    }
                    else {
                        context->clamp_ns = (xi_u64) (1000000000 * xi->time.delta.clamp_s);
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

                game->init(xi, XI_GAME_INIT);

                xi_logger_flush(&context->logger);

                while (true) {
                    linux_xi_context_update(context);

                    // :temp_usage
                    //
                    xi_temp_reset();

                    while (context->accum_ns >= context->fixed_ns) {
                        game->simulate(xi);
                        context->accum_ns -= context->fixed_ns;

                        context->did_update = true;

                        // :multiple_updates
                        //
                        xi->audio_player.event_count = 0;

                        if (context->quitting || xi->quit) { break; }
                    }

                    if (context->accum_ns < 0) { context->accum_ns = 0; }

                    game->render(xi, renderer);

                    // :single_worker
                    //
                    if (xi->thread_pool.thread_count == 1) { xi_thread_pool_await_complete(&xi->thread_pool); }

                    renderer->submit(renderer);
                    if (context->quitting || xi->quit) {
                        break;
                    }
                }

                xi_thread_pool_await_complete(&xi->thread_pool);

                result = 0;
            }
            else {
                xi_log(&context->logger, "renderer", "failed to initialise");
            }
        }
        else {
            xi_log(&context->logger, "window", "failed to create (%s)", SDL->GetError());
        }
    }

    xi_logger_flush(&context->logger);

    return result;
}

