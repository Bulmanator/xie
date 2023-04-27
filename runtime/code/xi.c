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

XI_INTERNAL XI_GAME_INIT(xiContext *xi, xi_u32 type) {
    // do nothing...
    //
    (void) xi;
    (void) type;
}

XI_INTERNAL XI_GAME_SIMULATE(xiContext *xi) {
    // do nothing...
    //
    (void) xi;
}

XI_INTERNAL XI_GAME_RENDER(xiContext *xi, xiRenderer *renderer) {
    // do nothing...
    //
    (void) xi;
    (void) renderer;
}

#if XI_OS_WIN32
    #include "os/win32.c"
#endif

