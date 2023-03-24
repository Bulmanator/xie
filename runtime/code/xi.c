#define XI_RUNTIME_BUILD 1
#include <xi/xi.h>

// os memory
//
static uptr xi_os_allocation_granularity_get();
static uptr xi_os_page_size_get();

static void *xi_os_virtual_memory_reserve(uptr size);
static b32   xi_os_virtual_memory_commit(void *base, uptr size);
static void  xi_os_virtual_memory_decommit(void *base, uptr size);
static void  xi_os_virtual_memory_release(void *base, uptr size);

// os fileio
//
typedef struct xiDirectoryListBuilder xiDirectoryListBuilder;

static void xi_os_directory_list_build(xiArena *arena,
        xiDirectoryListBuilder *builder, string path, b32 recursive);

#include "xi_utility.c"
#include "xi_arena.c"
#include "xi_string.c"

#include "xi_renderer.c"

#include "xi_fileio.c"

#if XI_OS_WIN32
    #include "os/win32.c"
#endif

