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
//     - get working directory
//     - directory iteration
//     - directory create/delete
//     - file open/close
//     - file read/write
//     - file create/delete

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
//   - math utilities
//   - thread work queue
//   - asset management
//     - images
//     - shaders
//     - audio
//     - fonts
//     - packing to .xia files
//     - streaming/threaded loading
//   - text file tokenizer
//

#include "xi_types.h"
#include "xi_maths.h"

#include "xi_utility.h"
#include "xi_arena.h"
#include "xi_string.h"

#include "xi_thread_pool.h"

#include "xi_renderer.h"

#include "xi_fileio.h"

#define XI_MAX_DISPLAYS 8

typedef struct xiDisplay {
    xi_u32 width;
    xi_u32 height;

    xi_f32 refresh_rate;
    xi_f32 scale; // windows are dpi aware, for informational purposes

    xi_string name;

    // @todo: maybe we want to have supported modes here
    //
} xiDisplay;

enum xiWindowState {
    XI_WINDOW_STATE_WINDOWED = 0,
    XI_WINDOW_STATE_WINDOWED_RESIZABLE,
    XI_WINDOW_STATE_BORDERLESS,
    XI_WINDOW_STATE_FULLSCREEN
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

    struct {
        xi_u32 width;
        xi_u32 height;

        xi_u32 display; // index from zero < system.display_count
        xi_u32 state;

        xi_string title;
    } window;

    xiThreadPool thread_pool;

    // :note any members of the system struct can be considered valid _at all times_ this includes
    // in the XI_GAME_PRE_INIT call even though other engine services have not yet been initialised
    //
    struct {
        xi_u32 display_count;
        xiDisplay displays[XI_MAX_DISPLAYS];

        xi_u32 processor_count;

        // :note encoded in utf-8
        // these paths do not end with a terminating forward slash and are not guaranteed to be
        // null-terminated
        //
        xi_string working_path;
        xi_string executable_path;
        xi_string user_path;
        xi_string temp_path;
    } system;
} xiContext;

// these function prototypes below are the functions your game dll should export
//
enum xiGameInitType {
    // the first call to init will be provided with this as 'init_type', occurs before any engine services
    // have been initialised and thus can be used to configure any of the user-configurable engine services,
    // such as window, asset system, renderer etc.
    //
    // this means engine services _should not be used_ during pre-init, only setup for configuration
    //
    // will happen exactly once on first startup
    //
    XI_GAME_PRE_INIT = 0,

    // the second call to init will be provided with this as 'init_type', occurs after engine services have
    // been successfully initialised and any services can be assumed to be valid and can now be used
    //
    // will happen exactly once on first startup, guaranteed to come after XI_GAME_PRE_INIT
    //
    XI_GAME_POST_INIT,

    // any time the game code is dynamically reloaded at runtime, the init function will be called again,
    // in such a case the 'init_type' paramter will contain this enumeration value
    //
    XI_GAME_RELOADED
};

#define XI_GAME_INIT(name) void name(xiContext *xi, xi_u32 init_type)
typedef XI_GAME_INIT(xiGameInit);

#define XI_GAME_SIMULATE(name) void name(xiContext *xi)
typedef XI_GAME_SIMULATE(xiGameSimulate);

#define XI_GAME_RENDER(name) void name(xiContext *xi, xiRenderer *renderer)
typedef XI_GAME_RENDER(xiGameRender);

enum xiGameCodeType {
    XI_GAME_CODE_TYPE_DYNAMIC = 0,
    XI_GAME_CODE_TYPE_STATIC
};

typedef struct xiGameCode {
    xi_u32 type;
    union {
        struct {
            xi_string init;     // name of the init function to load
            xi_string simulate; // name of the simulate function to load
            xi_string render;   // name of the render function to load
        } names;

        struct {
            xiGameInit     *init;     // direct pointer to the init function to call
            xiGameSimulate *simulate; // direct pointer to the simulate function to call
            xiGameRender   *render;   // direct pointer to the render function to call
        } functions;
    };
} xiGameCode;

// this will execute the main loop of the engine. game code can either be supplied directly via
// function pointers or via names to the functions and executable to load dynamically.
//
extern XI_API int xie_run(xiGameCode *code);

#if defined(__cplusplus)
}
#endif

#endif  // XI_H_
