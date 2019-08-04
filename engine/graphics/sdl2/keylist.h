#ifndef IRE_IKEYS_HPP
#define IRE_IKEYS_HPP
#include <SDL2/SDL_keycode.h>

#define IREKEY_A                 SDLK_a
#define IREKEY_B                 SDLK_b
#define IREKEY_C                 SDLK_c
#define IREKEY_D                 SDLK_d
#define IREKEY_E                 SDLK_e
#define IREKEY_F                 SDLK_f
#define IREKEY_G                 SDLK_g
#define IREKEY_H                 SDLK_h
#define IREKEY_I                 SDLK_i
#define IREKEY_J                 SDLK_j
#define IREKEY_K                 SDLK_k
#define IREKEY_L                 SDLK_l
#define IREKEY_M                 SDLK_m
#define IREKEY_N                 SDLK_n
#define IREKEY_O                 SDLK_o
#define IREKEY_P                 SDLK_p
#define IREKEY_Q                 SDLK_q
#define IREKEY_R                 SDLK_r
#define IREKEY_S                 SDLK_s
#define IREKEY_T                 SDLK_t
#define IREKEY_U                 SDLK_u
#define IREKEY_V                 SDLK_v
#define IREKEY_W                 SDLK_w
#define IREKEY_X                 SDLK_x
#define IREKEY_Y                 SDLK_y
#define IREKEY_Z                 SDLK_z
#define IREKEY_0                 SDLK_0
#define IREKEY_1                 SDLK_1
#define IREKEY_2                 SDLK_2
#define IREKEY_3                 SDLK_3
#define IREKEY_4                 SDLK_4
#define IREKEY_5                 SDLK_5
#define IREKEY_6                 SDLK_6
#define IREKEY_7                 SDLK_7
#define IREKEY_8                 SDLK_8
#define IREKEY_9                 SDLK_9
#define IREKEY_0_PAD             SDLK_KP_0
#define IREKEY_1_PAD             SDLK_KP_1
#define IREKEY_2_PAD             SDLK_KP_2
#define IREKEY_3_PAD             SDLK_KP_3
#define IREKEY_4_PAD             SDLK_KP_4
#define IREKEY_5_PAD             SDLK_KP_5
#define IREKEY_6_PAD             SDLK_KP_6
#define IREKEY_7_PAD             SDLK_KP_7
#define IREKEY_8_PAD             SDLK_KP_8
#define IREKEY_9_PAD             SDLK_KP_9
#define IREKEY_F1                SDLK_F1
#define IREKEY_F2                SDLK_F2
#define IREKEY_F3                SDLK_F3
#define IREKEY_F4                SDLK_F4
#define IREKEY_F5                SDLK_F5
#define IREKEY_F6                SDLK_F6
#define IREKEY_F7                SDLK_F7
#define IREKEY_F8                SDLK_F8
#define IREKEY_F9                SDLK_F9
#define IREKEY_F10               SDLK_F10
#define IREKEY_F11               SDLK_F11
#define IREKEY_F12               SDLK_F12
#define IREKEY_ESC               SDLK_ESCAPE
#define IREKEY_TILDE             SDLK_BACKQUOTE	// SDLK_GRAVE, apparently not
#define IREKEY_MINUS             SDLK_MINUS
#define IREKEY_EQUALS            SDLK_EQUALS
#define IREKEY_BACKSPACE         SDLK_BACKSPACE
#define IREKEY_TAB               SDLK_TAB
#define IREKEY_OPENBRACE         SDLK_LEFTPAREN
#define IREKEY_CLOSEBRACE        SDLK_RIGHTPAREN
#define IREKEY_ENTER             SDLK_RETURN
#define IREKEY_COLON             SDLK_COLON
#define IREKEY_QUOTE             SDLK_QUOTE
#define IREKEY_BACKSLASH         SDLK_BACKSLASH
#define IREKEY_BACKSLASH2        SDLK_BACKSLASH	// Doubtful
#define IREKEY_COMMA             SDLK_COMMA
#define IREKEY_STOP              SDLK_PERIOD
#define IREKEY_DOT               SDLK_PERIOD	// Not sure that's right
#define IREKEY_SLASH             SDLK_SLASH
#define IREKEY_SPACE             SDLK_SPACE
#define IREKEY_INSERT            SDLK_INSERT
#define IREKEY_DEL               SDLK_DELETE
#define IREKEY_HOME              SDLK_HOME
#define IREKEY_END               SDLK_END
#define IREKEY_PGUP              SDLK_PAGEUP
#define IREKEY_PGDN              SDLK_PAGEDOWN
#define IREKEY_LEFT              SDLK_LEFT
#define IREKEY_RIGHT             SDLK_RIGHT
#define IREKEY_UP                SDLK_UP
#define IREKEY_DOWN              SDLK_DOWN
#define IREKEY_SLASH_PAD         SDLK_KP_DIVIDE
#define IREKEY_ASTERISK          SDLK_KP_MULTIPLY
#define IREKEY_MINUS_PAD         SDLK_KP_MINUS
#define IREKEY_PLUS_PAD          SDLK_KP_PLUS
#define IREKEY_DEL_PAD           SDLK_KP_PERIOD
#define IREKEY_ENTER_PAD         SDLK_KP_ENTER
#define IREKEY_PRTSCR            SDLK_PRINTSCREEN
#define IREKEY_PAUSE             SDLK_PAUSE
#define IREKEY_YEN               -1
#define IREKEY_YEN2              -1

#define IREKEY_MODIFIERS         -1	// ??

#define IREKEY_LSHIFT            SDLK_LSHIFT
#define IREKEY_RSHIFT            SDLK_RSHIFT
#define IREKEY_LCONTROL          SDLK_LCTRL
#define IREKEY_RCONTROL          SDLK_RCTRL
#define IREKEY_ALT               SDLK_LALT
#define IREKEY_ALTGR             SDLK_RALT
#define IREKEY_LWIN              SDLK_LGUI
#define IREKEY_RWIN              SDLK_RGUI
#define IREKEY_MENU              -100
#define IREKEY_SCRLOCK           SDLK_SCROLLLOCK
#define IREKEY_NUMLOCK           SDLK_NUMLOCKCLEAR
#define IREKEY_CAPSLOCK          SDLK_CAPSLOCK

//#define IREKEY_MAX               SDLK_LAST
#define IREKEY_MAX               4096


#define IRESHIFT_SHIFT            1
#define IRESHIFT_CONTROL          2
#define IRESHIFT_ALT              4

#endif
