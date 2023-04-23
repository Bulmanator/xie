#if !defined(XI_INPUT_H_)
#define XI_INPUT_H_

// :note any key enum that has been assigned a character literal can be looked up into the array via said
// character literal over the use of the enum constant itself
//
// excluding ordering produced by using the ascii literals, these are roughly laid out in the order they
// are defined by the hid keyboard scancode specification
//
enum xiKeyboardKey {
    XI_KEYBOARD_KEY_UNKNOWN = 0,

    XI_KEYBOARD_KEY_RETURN,
    XI_KEYBOARD_KEY_ESCAPE,
    XI_KEYBOARD_KEY_BACKSPACE,
    XI_KEYBOARD_KEY_TAB,

    XI_KEYBOARD_KEY_HOME,
    XI_KEYBOARD_KEY_PAGEUP,
    XI_KEYBOARD_KEY_DELETE,
    XI_KEYBOARD_KEY_END,
    XI_KEYBOARD_KEY_PAGEDOWN,

    XI_KEYBOARD_KEY_RIGHT,
    XI_KEYBOARD_KEY_LEFT,
    XI_KEYBOARD_KEY_DOWN,
    XI_KEYBOARD_KEY_UP,

    XI_KEYBOARD_KEY_F1,
    XI_KEYBOARD_KEY_F2,
    XI_KEYBOARD_KEY_F3,
    XI_KEYBOARD_KEY_F4,
    XI_KEYBOARD_KEY_F5,
    XI_KEYBOARD_KEY_F6,
    XI_KEYBOARD_KEY_F7,
    XI_KEYBOARD_KEY_F8,
    XI_KEYBOARD_KEY_F9,
    XI_KEYBOARD_KEY_F10,
    XI_KEYBOARD_KEY_F11,
    XI_KEYBOARD_KEY_F12,

    XI_KEYBOARD_KEY_INSERT,

    XI_KEYBOARD_KEY_SPACE  = ' ',

    XI_KEYBOARD_KEY_QUOTE  = '\'',
    XI_KEYBOARD_KEY_COMMA  = ',',
    XI_KEYBOARD_KEY_MINUS  = '-',
    XI_KEYBOARD_KEY_PERIOD = '.',
    XI_KEYBOARD_KEY_SLASH  = '/',

    XI_KEYBOARD_KEY_0 = '0',
    XI_KEYBOARD_KEY_1 = '1',
    XI_KEYBOARD_KEY_2 = '2',
    XI_KEYBOARD_KEY_3 = '3',
    XI_KEYBOARD_KEY_4 = '4',
    XI_KEYBOARD_KEY_5 = '5',
    XI_KEYBOARD_KEY_6 = '6',
    XI_KEYBOARD_KEY_7 = '7',
    XI_KEYBOARD_KEY_8 = '8',
    XI_KEYBOARD_KEY_9 = '9', // 57

    XI_KEYBOARD_KEY_SEMICOLON = ';',
    XI_KEYBOARD_KEY_EQUALS    = '=', // 61

    // these use the capital letter ascii range as lookups for letters should be done with lowercase character
    // literals or the enum constant itself
    //
    XI_KEYBOARD_KEY_LSHIFT,
    XI_KEYBOARD_KEY_LCTRL,
    XI_KEYBOARD_KEY_LALT,

    XI_KEYBOARD_KEY_RSHIFT,
    XI_KEYBOARD_KEY_RCTRL,
    XI_KEYBOARD_KEY_RALT,

    XI_KEYBOARD_KEY_LBRACKET  = '[',
    XI_KEYBOARD_KEY_BACKSLASH = '\\',
    XI_KEYBOARD_KEY_RBRACKET  = ']',
    XI_KEYBOARD_KEY_GRAVE     = '`',

    XI_KEYBOARD_KEY_A = 'a', // 97
    XI_KEYBOARD_KEY_B = 'b',
    XI_KEYBOARD_KEY_C = 'c',
    XI_KEYBOARD_KEY_D = 'd',
    XI_KEYBOARD_KEY_E = 'e',
    XI_KEYBOARD_KEY_F = 'f',
    XI_KEYBOARD_KEY_G = 'g',
    XI_KEYBOARD_KEY_H = 'h',
    XI_KEYBOARD_KEY_I = 'i',
    XI_KEYBOARD_KEY_J = 'j',
    XI_KEYBOARD_KEY_K = 'k',
    XI_KEYBOARD_KEY_L = 'l',
    XI_KEYBOARD_KEY_M = 'm',
    XI_KEYBOARD_KEY_N = 'n',
    XI_KEYBOARD_KEY_O = 'o',
    XI_KEYBOARD_KEY_P = 'p',
    XI_KEYBOARD_KEY_Q = 'q',
    XI_KEYBOARD_KEY_R = 'r',
    XI_KEYBOARD_KEY_S = 's',
    XI_KEYBOARD_KEY_T = 't',
    XI_KEYBOARD_KEY_U = 'u',
    XI_KEYBOARD_KEY_V = 'v',
    XI_KEYBOARD_KEY_W = 'w',
    XI_KEYBOARD_KEY_X = 'x',
    XI_KEYBOARD_KEY_Y = 'y',
    XI_KEYBOARD_KEY_Z = 'z',

    XI_KEYBOARD_KEY_COUNT
} xiKeyboardKey;

// make sure we haven't encroached on the ascii ranges
//
XI_STATIC_ASSERT(XI_KEYBOARD_KEY_INSERT < XI_KEYBOARD_KEY_SPACE);
XI_STATIC_ASSERT(XI_KEYBOARD_KEY_RALT   < XI_KEYBOARD_KEY_RBRACKET);

typedef struct xiInputButton {
    xi_b32 down;   // currently down
    xi_u32 repeat; // number of repeat events, mouse buttons don't get repeats

    xi_b32 pressed;  // was pressed this frame
    xi_b32 released; // was released this frame
} xiInputButton;

typedef struct xiInputKeyboard {
    xi_b32 connected; // true if there is a keyboard connected :input_connected
    xi_b32 active;    // true if any key was pressed within the last frame :input_active

    // these are here for convenience, will be set to 'true' if either left/right modifiers
    // are down on the current frame
    //
    xi_b32 alt;
    xi_b32 ctrl;
    xi_b32 shift;

    xiInputButton keys[XI_KEYBOARD_KEY_COUNT];
} xiInputKeyboard;

typedef struct xiInputMouse {
    xi_b32 connected; // :input_connected
    xi_b32 active;    // :input_active

    xiInputButton left;
    xiInputButton middle;
    xiInputButton right;
    xiInputButton x0;
    xiInputButton x1;

    struct {
        xi_v2s screen; // signed just incase. we don't need the range anyway
        xi_v2  clip;
        xi_v2  wheel;
    } position;

    struct {
        xi_v2s screen;
        xi_v2  clip;
        xi_v2  wheel; // .x is horizontal scroll, .y is vertical scroll
    } delta;
} xiInputMouse;

#endif  // XI_INPUT_H_
