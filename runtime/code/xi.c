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

#include "xi_audio.c"
#include "xi_draw.c"

FileScope GAME_INIT(Xi_Engine *xi, U32 type) {
    // do nothing...
    //
    (void) xi;
    (void) type;
}

FileScope GAME_SIMULATE(Xi_Engine *xi) {
    // do nothing...
    //
    (void) xi;
}

FileScope GAME_RENDER(Xi_Engine *xi, RendererContext *renderer) {
    // do nothing...
    //
    (void) xi;
    (void) renderer;
}

#if OS_WIN32
    #include "os/win32.c"
#elif OS_LINUX
    #include "os/linux.c"
#endif

