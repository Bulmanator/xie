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
//     - timing information
//     - keyboard input
//     - mouse input
//   - string utilities
//     - logging system
//   - file system support
//     - get user directory
//     - get temp directory
//     - get working directory
//     - directory iteration
//     - directory create/delete
//     - file open/close
//     - file read/write
//     - file create/delete
//   - thread work queue
//   - graphics rendering
//     - basic sprite rendering
//     - 2d animation rendering
//   - math utilities
//   - asset management
//     - images
//     - animations
//     - packing to .xia files
//     - streaming/threaded loading

// xi runtime will supply
//     - controller input
//     - audio output
//       - mixer
//   - graphics rendering
//     - custom shaders
//     - custom render targets
//   - asset management
//     - shaders
//     - audio
//     - fonts
//     - packing to .xia files
//   - text file tokenizer
//

#include "xi_types.h"
#include "xi_maths.h"

#include "xi_arena.h"

#include "xi_fileio.h"

#include "xi_string.h"
#include "xi_utility.h"

#include "xi_thread_pool.h"

#include "xi_renderer.h"

#include "xi_xia.h"
#include "xi_assets.h"

#include "xi_draw.h"

#include "xi_input.h"

#define XI_VERSION_MAJOR 0
#define XI_VERSION_MINOR 3
#define XI_VERSION_PATCH 2

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
    struct {
        // @important: do not move this from the beginning of the structure
        //
        // this will be filled out by the engine when the context is passed to user functions, you
        // can use this to check if the version of the header included matches/supports the version
        // if the engine that is running
        //
        // we guarantee that any changes to the 'patch' number will never change this structure, however,
        // if the 'minor' or 'major' version numbers do not match this structure could've changed and
        // updated headers should be used. this also extends to the .inl implementation file
        //
        xi_u32 major;
        xi_u32 minor;
        xi_u32 patch;
    } version;

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

    xiAssetManager assets;
    xiThreadPool thread_pool;

    xiRenderer renderer;

    xiInputKeyboard keyboard;
    xiInputMouse mouse;

    struct {
        xi_u64 ticks;

        struct {
            xi_u64 ns;
            xi_u64 us;
            xi_u64 ms;
            xi_f64 s;
        } total;

        struct {
            xi_u64 ns;
            xi_u64 us;
            xi_u64 ms;
            xi_f64 s;

            xi_u32 fixed_hz; // fixed step rate in hz
            xi_f32 clamp_s;  // max delta time in seconds
        } delta;
    } time;

    // :note any members of the system struct can be considered valid _at all times_ this includes
    // in the XI_ENGINE_CONFIGURE call even though other engine services have not yet been initialised
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

        // setting this to true will open a console window to log stdout/stderr to, only really valid on
        // windows
        //
        xi_b32 console_open;

        // these can be used to create loggers that output to the console, if the console is not open when
        // XI_ENGINE_CONFIGURE init is called, these may change after if the above 'console_open' was
        // set to true
        //
        xiFileHandle out; // console out
        xiFileHandle err; // console err
    } system;
} xiContext;

// these function prototypes below are the functions your game dll should export
//
enum xiGameInitType {
    // the first call to the game code 'init' function will be provided with this value. this occurs
    // before any of the engine systems have been initialised and thus can be used to configure them
    // to the games preferences.
    //
    // engine systems are not valid to use on this call
    //
    XI_ENGINE_CONFIGURE = 0,

    // the second call to the game code 'init' function will be provided with this value. this occurs
    // after engine systems have been initialise and thus they are valid to use.
    //
    // this can be used to initialise any game state that may require interaction with engine systems,
    // such as asset system etc.
    //
    XI_GAME_INIT,

    // any time the game code is dynamically reloaded at runtime, the init function will be called again,
    // in such a case the 'init_type' paramter will contain this enumeration value
    //
    XI_GAME_RELOADED
};

// the game initialisation function that is used to setup the engine and game. called once for engine and
// once for game, with the exception of dynamically loaded gmae code that is hot-reloaded
//
#define XI_GAME_INIT(name) void name(xiContext *xi, xi_u32 init_type)
typedef XI_GAME_INIT(xiGameInit);

// the game simulate function that is used to perform a single update step, will be called once
// per update step
//
#define XI_GAME_SIMULATE(name) void name(xiContext *xi)
typedef XI_GAME_SIMULATE(xiGameSimulate);

// the game render function used to produce draw commands for the backend renderer,
// will be called once per frame
//
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

#include "xi.inl"

#if defined(__cplusplus)
}
#endif

#endif  // XI_H_
