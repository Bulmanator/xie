#define XI_RUNTIME_BUILD 1
#include <xi/xi.h>

#if XI_OS_WIN32
    #include "os/win32.c"
#endif

#include "xi_utility.c"
#include "xi_arena.c"
#include "xi_string.c"

#include "xi_renderer.c"
