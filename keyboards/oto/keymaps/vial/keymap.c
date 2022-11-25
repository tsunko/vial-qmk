#include QMK_KEYBOARD_H
#include "7032b/driver.h"
#include "anim/bongo.h"
#include "oto.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

	[0] = LAYOUT(
		KC_ESC ,  KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  , KC_F6  , KC_F7  , KC_F8  ,                                               KC_PRINT_SCREEN,
		KC_GRV ,  KC_1   , KC_2   , KC_3   , KC_4   , KC_5   , KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_MINS, KC_EQL , KC_BSPC,  KC_PGUP,
		KC_TAB ,  KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   , KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_LBRC, KC_RBRC, KC_BSLS,  KC_DELETE,
		KC_CAPS,  KC_A   , KC_S   , KC_D   , KC_F   , KC_G   , KC_H   , KC_J   , KC_K   , KC_L   , KC_SCLN, KC_QUOT, KC_ENT ,           KC_PGDN,
		KC_LSFT,  KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   , KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_LSFT, KC_UP  ,
		KC_LCTL,  KC_LGUI, KC_LALT,                   KC_SPC ,                   KC_RALT, KC_RCTL,          KC_LEFT, KC_DOWN, KC_RIGHT
    ),

};

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (clockwise) {
        tap_code_delay(KC_VOLU, 10);
    } else {
        tap_code_delay(KC_VOLD, 10);
    }
    return false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static bool right = false;

    if (record->event.pressed){
        disp_cursorHome();
        disp_drawImageXY(0, 0, 256, 64, right ? bongo_tap_right : bongo_tap_left);
        right = !right;
    }
    
    disp_cursorHome();
    disp_printf("WPM: %d", get_current_wpm());
    

    return true;
}

