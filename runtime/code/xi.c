#define XI_RUNTIME_BUILD 1
#include <xi/xi.h>

#define XI_OS_WIN32 0

#if defined(_WIN32)
    #undef  XI_OS_WIN32
    #define XI_OS_WIN32 1
#else
    #error "unsupported operating system."
#endif

#if XI_OS_WIN32
    #include "os/win32.c"
#endif

#include "xi_utility.c"

#include "xi_arena.c"
