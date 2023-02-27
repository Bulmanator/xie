#if !defined(XI_H_)
#define XI_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "xi_types.h"
#include "xi_utility.h"

#include "xi_arena.h"

#define XI_MAX_DISPLAYS 8

typedef struct xiDisplay {
    u32 width;
    u32 height;

    f32 refresh_rate;
    f32 scale; // windows are dpi aware, for informational purposes

    // @todo: maybe we want to have supported modes here
    //
} xiDisplay;

enum xiWindowState {
    XI_WINDOW_STATE_WINDOWED = 0,
    XI_WINDOW_STATE_WINDOWED_RESIZABLE,
    XI_WINDOW_STATE_BORDERLESS,
    XI_WINDOW_FULLSCREEN
};

typedef struct xiContext {
    // this user pointer can be set to anything, thus can be used to carry game
    // state between calls to update, render etc.
    //
    void *user;

    struct {
        s32 x;
        s32 y;

        u32 width;
        u32 height;

        u32 display_index; // index from zero < system.monitor_count
        u32 state;

        str8 title;
    } window;

    struct {
        u32 display_count;
        xiDisplay displays[XI_MAX_DISPLAYS];

        uptr processor_count;
    } system;
} xiContext;

#if defined(__cplusplus)
}
#endif

#endif  // XI_H_
