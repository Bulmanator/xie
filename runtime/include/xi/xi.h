#if !defined(XI_H_)
#define XI_H_

#if defined(__cplusplus)
extern "C" {
#endif

// xi runtime supplies
//   - core types
//   - math types
//   - utility macros
//   - arena memory allocation
//     - temporary arena
//   - interactive application support
//     - window
//   - string utilities
//   - file system support
//     - get user directory
//     - get temp directory

// xi runtime will supply
//     - keyboard input
//     - mouse input
//     - controller input
//     - timing information
//     - audio output
//       - mixer
//   - graphics rendering
//     - basic sprite rendering
//     - 2d animation rendering
//     - custom shaders
//     - custom render targets
//   - string utilities
//   - math utilities
//   - file system support
//     - directory iteration
//     - directory create/delete
//     - file open/close
//     - file read/write
//     - file create/delete
//     - get/set working directory
//   - thread work queue
//   - asset management
//     - images
//     - shaders
//     - audio
//     - fonts
//     - packing to .xi files
//     - streaming/threaded loading
//   - text file tokenizer
//

#include "xi_types.h"
#include "xi_utility.h"
#include "xi_arena.h"
#include "xi_string.h"

#define XI_MAX_DISPLAYS 8

typedef struct xiDisplay {
    u32 width;
    u32 height;

    f32 refresh_rate;
    f32 scale; // windows are dpi aware, for informational purposes

    // @todo: add a name here so we can identify displays to the user
    // at runtime
    //

    // @todo: maybe we want to have supported modes here
    //
} xiDisplay;

enum xiWindowState {
    XI_WINDOW_STATE_WINDOWED = 0,
    XI_WINDOW_STATE_WINDOWED_RESIZABLE,
    XI_WINDOW_STATE_BORDERLESS,
    XI_WINDOW_STATE_FULLSCREEN
};

enum xiContextFlags {
    // will be set on the first call to 'init' if the game code has been reloaded at runtime
    //
    XI_CONTEXT_FLAG_RELOADED = (1 << 0)
};

typedef struct xiContext {
    // this user pointer can be set to anything, thus can be used to carry game
    // state between calls to update, render etc.
    //
    // it is recommended that you contain all game state in some global structure and place a pointer
    // to it in here. using global variables within reloadable code is unlikely to work reliably as they
    // get reset after reloading
    //
    void *user;

    u64 flags;

    struct {
        u32 width;
        u32 height;

        u32 display_index; // index from zero < system.display_count
        u32 state;

        string title;
    } window;

    struct {
        u32 display_count;
        xiDisplay displays[XI_MAX_DISPLAYS];

        uptr processor_count;

        // :note encoded in utf-8
        // these paths do not end with a terminating forward slash
        //
        string working_path;
        string executable_path;
        string user_path;
        string temp_path;
    } system;
} xiContext;

// these function prototypes below are the functions your game dll should export
//
#define XI_GAME_INIT(name) void name(xiContext *xi)
typedef XI_GAME_INIT(xiGameInit);

#if defined(__cplusplus)
}
#endif

#endif  // XI_H_
