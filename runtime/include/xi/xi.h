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
//     - audio output
//       - mixer
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
//     - audio
//     - packing to .xia files
//     - streaming/threaded loading


// xi runtime will supply
//     - controller input

//   - graphics rendering
//     - custom shaders
//     - custom render targets
//   - asset management
//     - shaders
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

#include "xi_audio.h"
#include "xi_draw.h"

#include "xi_input.h"

#define XI_VERSION_MAJOR 2
#define XI_VERSION_MINOR 1
#define XI_VERSION_PATCH 2

typedef struct DisplayInfo DisplayInfo;
struct DisplayInfo {
    U32 width;
    U32 height;

    Str8 name;

    F32 scale; // windows are dpi aware, for informational purposes
    F32 refresh_rate;

    B32 primary;

    // @todo: maybe we want to have supported modes here
    //
};

typedef U32 WindowState;
enum {
    WINDOW_STATE_WINDOWED = 0,
    WINDOW_STATE_WINDOWED_RESIZABLE,
    WINDOW_STATE_WINDOWED_BORDERLESS,
    WINDOW_STATE_FULLSCREEN
};

typedef struct Xi_Engine Xi_Engine;
struct Xi_Engine {
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
        U32 major;
        U32 minor;
        U32 patch;
    } version;

    // this user pointer can be set to anything, thus can be used to carry game
    // state between calls to update, render etc.
    //
    // it is recommended that you contain all game state in some global structure and place a pointer
    // to it in here. using global variables within reloadable code is unlikely to work reliably as they
    // get reset after reloading
    //
    void *user;

    // signals to the engine that the game would like to quit and shutdown, can be set to true at any
    // point
    //
    // if this is passed to the game code as 'true' the user has requested that the game close
    // (i.e. closing the window etc.) and should be handled appropriatley (saving game state if desired etc.)
    //
    B32 quit;

    struct {
        WindowState state;

        U32 width;
        U32 height;

        U32 display; // index into display array

        Str8 title;  // it is valid to use temporary memory to change the title
    } window;

    AssetManager assets;
    ThreadPool   thread_pool;

    // drawing
    //
    RendererContext renderer;

    // input
    //
    InputKeyboard keyboard;
    InputMouse    mouse;

    // time
    //
    struct {
        U64 ticks;

        struct {
            U64 ns;
            U64 us;
            U64 ms;
            F64 s;
        } total;

        struct {
            U64 ns;
            U64 us;
            U64 ms;
            F64 s;

            U32 fixed_hz; // fixed step rate in hz
            F32 clamp_s;  // max delta time in seconds
        } delta;
    } time;

    // audio
    //
    AudioPlayer audio_player;

    // :note any members of the system struct can be considered valid _at all times_ this includes
    // in the XI_ENGINE_CONFIGURE call even though other engine services have not yet been initialised
    //
    struct {
        U32 num_displays;
        DisplayInfo *displays; // array of display info

        U32 num_cpus;

        // :note encoded in utf-8
        // these paths do not end with a terminating forward slash and are not guaranteed to be
        // null-terminated
        //
        Str8 working_path;
        Str8 executable_path;
        Str8 user_path;
        Str8 temp_path;

        // setting this to true will open a console window to log stdout/stderr to, only really valid on
        // windows
        //
        B32 use_console;

        // these can be used to create loggers that output to the console, if the console is not open when
        // XI_ENGINE_CONFIGURE init is called, these may change after if the above 'console_open' was
        // set to true
        //
        FileHandle out; // console stdout
        FileHandle err; // console stderr
    } system;
};

// these function prototypes below are the functions your game dll should export
//
typedef U32 Xi_InitType;
enum {
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

// these function definition macros should be used by the game code for your initialisation, simulate
// and render functions. they can either be marked as export with 'extern XI_EXPORT' and dynamically
// loaded at runtime, or can be passed as direct pointers via the xiGameCode structure below
//
// parameters can be declared in these macros as if it were a normal function declaration, this
// is purely for documentational/redability purposes
//
#define GAME_INIT(...)     void __xi_game_init(Xi_Engine *xi, Xi_InitType type)
#define GAME_SIMULATE(...) void __xi_game_simulate(Xi_Engine *xi)
#define GAME_RENDER(...)   void __xi_game_render(Xi_Engine *xi, RendererContext *renderer)

typedef void (Xi_GameInitProc)    (Xi_Engine *xi, Xi_InitType type);
typedef void (Xi_GameSimulateProc)(Xi_Engine *xi);
typedef void (Xi_GameRenderProc)  (Xi_Engine *xi, RendererContext *renderer);

typedef struct Xi_GameCode Xi_GameCode;
struct Xi_GameCode {
    B32 reloadable;

    // these can be left null if 'reloadable' is set to true
    //
    // @todo: collapse 'simulate' and 'render' to a single call
    //
    Xi_GameInitProc     *Init;     // direct pointer to the init function to call
    Xi_GameSimulateProc *Simulate; // direct pointer to the simulate function to call
    Xi_GameRenderProc   *Render;   // direct pointer to the render function to call
};

// this will execute the main loop of the engine. game code can either be supplied directly via
// function pointers or via names to the functions and executable to load dynamically.
//
Func S32 Xi_EngineRun(Xi_GameCode *code);

#include "xi.inl"

#if defined(__cplusplus)
}
#endif

#endif  // XI_H_
