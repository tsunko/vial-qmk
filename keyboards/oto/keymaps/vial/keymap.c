#include <stdio.h>
#include QMK_KEYBOARD_H
#include "timer.h"
#include "quantum.h"
#include "oled_driver.h"
#include "anim/bongo_tap.h"
#include "oto.h"

#define IDLE_FRAMES    5
#define IDLE_SPEED     20
#define TAP_FRAMES     2
#define TAP_SPEED      40
#define FRAME_DURATION 200
#define FRAME_SIZE     (256 * (64 / 8))

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

uint32_t frame_timer      = 0;
uint32_t sleep_timer      = 0;
uint8_t  bongo_idle_frame = 0;
bool     bongo_right_tap  = false;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  uprintf("KL: kc: 0x%04X, col: %2u, row: %2u, pressed: %u, time: %5u, int: %u, count: %u\n", keycode, record->event.key.col, record->event.key.row, record->event.pressed, record->event.time, record->tap.interrupted, record->tap.count);
  return true;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (clockwise) {
        tap_code_delay(KC_VOLU, 10);
    } else {
        tap_code_delay(KC_VOLD, 10);
    }
    return false;
}

void render_bongo_cat(void) {
    void write_bongo_frame(void) {
        if (get_current_wpm() <= IDLE_SPEED) {
            // current_idle_frame = (current_idle_frame + 1) % IDLE_FRAME_COUNT;
            // oled_write_raw(idle[abs((IDLE_FRAMES - 1) - current_idle_frame)], FRAME_SIZE);
        } else {
            oled_write_raw((const char*)bongo_tap[bongo_right_tap], FRAME_SIZE);
            bongo_right_tap = !bongo_right_tap;
        }
    }

    if (get_current_wpm()) {
        oled_on();

        if (timer_elapsed32(frame_timer) > FRAME_DURATION) {
            frame_timer = timer_read32();
            write_bongo_frame();
        }

        sleep_timer = timer_read32();
    } else {
        if (timer_elapsed32(sleep_timer) > OLED_TIMEOUT) {
            oled_off();
        } else {
            if (timer_elapsed32(frame_timer) > FRAME_DURATION) {
                frame_timer = timer_read32();
                write_bongo_frame();
            }
        }
    }
}

bool oled_task_user(void) {
    static char wpm_buffer[8];
    static char scan_rate_buffer[16];

    render_bongo_cat();

    oled_set_cursor(0, 0);
    sprintf(wpm_buffer, "WPM:%03d", get_current_wpm());
    oled_write(wpm_buffer, false);

    oled_set_cursor(0, 1);
    sprintf(scan_rate_buffer, "SCAN:%04lu", get_matrix_scan_rate());
    oled_write(scan_rate_buffer, false);

    oled_set_cursor(0, 2);
    led_t led_state = host_keyboard_led_state();
    oled_write(led_state.caps_lock ? "CAPS" : "    ", false);

    return false;
}