#if !defined(XI_INPUT_H_)
#define XI_INPUT_H_

// :note any key enum that has been assigned a character literal can be looked up into the array via said
// character literal over the use of the enum constant itself
//
// excluding ordering produced by using the ascii literals, these are roughly laid out in the order they
// are defined by the hid keyboard scancode specification
//
typedef U32 KeyboardKey;
enum KeyboardKey {
    KEYBOARD_KEY_UNKNOWN = 0,

    KEYBOARD_KEY_RETURN,
    KEYBOARD_KEY_ESCAPE,
    KEYBOARD_KEY_BACKSPACE,
    KEYBOARD_KEY_TAB,

    KEYBOARD_KEY_HOME,
    KEYBOARD_KEY_PAGEUP,
    KEYBOARD_KEY_DELETE,
    KEYBOARD_KEY_END,
    KEYBOARD_KEY_PAGEDOWN,

    KEYBOARD_KEY_RIGHT,
    KEYBOARD_KEY_LEFT,
    KEYBOARD_KEY_DOWN,
    KEYBOARD_KEY_UP,

    KEYBOARD_KEY_F1,
    KEYBOARD_KEY_F2,
    KEYBOARD_KEY_F3,
    KEYBOARD_KEY_F4,
    KEYBOARD_KEY_F5,
    KEYBOARD_KEY_F6,
    KEYBOARD_KEY_F7,
    KEYBOARD_KEY_F8,
    KEYBOARD_KEY_F9,
    KEYBOARD_KEY_F10,
    KEYBOARD_KEY_F11,
    KEYBOARD_KEY_F12,

    KEYBOARD_KEY_INSERT,

    KEYBOARD_KEY_SPACE  = ' ',

    KEYBOARD_KEY_QUOTE  = '\'',
    KEYBOARD_KEY_COMMA  = ',',
    KEYBOARD_KEY_MINUS  = '-',
    KEYBOARD_KEY_PERIOD = '.',
    KEYBOARD_KEY_SLASH  = '/',

    KEYBOARD_KEY_0 = '0',
    KEYBOARD_KEY_1 = '1',
    KEYBOARD_KEY_2 = '2',
    KEYBOARD_KEY_3 = '3',
    KEYBOARD_KEY_4 = '4',
    KEYBOARD_KEY_5 = '5',
    KEYBOARD_KEY_6 = '6',
    KEYBOARD_KEY_7 = '7',
    KEYBOARD_KEY_8 = '8',
    KEYBOARD_KEY_9 = '9', // 57

    KEYBOARD_KEY_SEMICOLON = ';',
    KEYBOARD_KEY_EQUALS    = '=', // 61

    // these use the capital letter ascii range as lookups for letters should be done with lowercase character
    // literals or the enum constant itself
    //
    KEYBOARD_KEY_LSHIFT,
    KEYBOARD_KEY_LCTRL,
    KEYBOARD_KEY_LALT,

    KEYBOARD_KEY_RSHIFT,
    KEYBOARD_KEY_RCTRL,
    KEYBOARD_KEY_RALT,

    KEYBOARD_KEY_LBRACKET  = '[',
    KEYBOARD_KEY_BACKSLASH = '\\',
    KEYBOARD_KEY_RBRACKET  = ']',
    KEYBOARD_KEY_GRAVE     = '`',

    KEYBOARD_KEY_A = 'a', // 97
    KEYBOARD_KEY_B = 'b',
    KEYBOARD_KEY_C = 'c',
    KEYBOARD_KEY_D = 'd',
    KEYBOARD_KEY_E = 'e',
    KEYBOARD_KEY_F = 'f',
    KEYBOARD_KEY_G = 'g',
    KEYBOARD_KEY_H = 'h',
    KEYBOARD_KEY_I = 'i',
    KEYBOARD_KEY_J = 'j',
    KEYBOARD_KEY_K = 'k',
    KEYBOARD_KEY_L = 'l',
    KEYBOARD_KEY_M = 'm',
    KEYBOARD_KEY_N = 'n',
    KEYBOARD_KEY_O = 'o',
    KEYBOARD_KEY_P = 'p',
    KEYBOARD_KEY_Q = 'q',
    KEYBOARD_KEY_R = 'r',
    KEYBOARD_KEY_S = 's',
    KEYBOARD_KEY_T = 't',
    KEYBOARD_KEY_U = 'u',
    KEYBOARD_KEY_V = 'v',
    KEYBOARD_KEY_W = 'w',
    KEYBOARD_KEY_X = 'x',
    KEYBOARD_KEY_Y = 'y',
    KEYBOARD_KEY_Z = 'z',

    KEYBOARD_KEY_COUNT
};

// make sure we haven't encroached on the ascii ranges
//
StaticAssert(KEYBOARD_KEY_INSERT < KEYBOARD_KEY_SPACE);
StaticAssert(KEYBOARD_KEY_RALT   < KEYBOARD_KEY_RBRACKET);

typedef struct InputButton InputButton;
struct InputButton {
    B32 down;   // currently down
    U32 repeat; // number of repeat events, mouse buttons don't get repeats

    B32 pressed;  // was pressed this frame
    B32 released; // was released this frame
};

typedef struct InputKeyboard InputKeyboard;
struct InputKeyboard {
    B32 connected; // true if there is a keyboard connected :input_connected
    B32 active;    // true if any key was pressed within the last frame :input_active

    // these are here for convenience, will be set to 'true' if either left/right modifiers
    // are down on the current frame
    //
    B32 alt;
    B32 ctrl;
    B32 shift;

    InputButton keys[KEYBOARD_KEY_COUNT];
};

typedef struct InputMouse InputMouse;
struct InputMouse {
    B32 connected; // :input_connected
    B32 active;    // :input_active

    InputButton left;
    InputButton middle;
    InputButton right;
    InputButton x0;
    InputButton x1;

    struct {
        Vec2S screen; // signed just incase. we don't need the range anyway
        Vec2F  clip;
        Vec2F  wheel;
    } position;

    struct {
        Vec2S screen;
        Vec2F clip;
        Vec2F wheel; // .x is horizontal scroll, .y is vertical scroll
    } delta;
};

#endif  // XI_INPUT_H_
