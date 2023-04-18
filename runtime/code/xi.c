#define XI_RUNTIME_BUILD 1
#include <xi/xi.h>

#include "xi_maths.c"
#include "xi_utility.c"
#include "xi_arena.c"
#include "xi_string.c"

#include "xi_thread_pool.c"

#include "xi_renderer.c"

#include "xi_fileio.c"
#include "xi_assets.c"

#include "xi_draw.c"

#if XI_OS_WIN32
    #include "os/win32.c"
#endif

