#include "oto.h"
#include "7032b/driver.h"

void keyboard_post_init_kb(void) {
    disp_initialize(DISPLAY_WIDTH, DISPLAY_HEIGHT, (pin_t[])DISPLAY_DATA_PINS, DISPLAY_WRITE_PIN, DISPLAY_BUSY_PIN);
    disp_clear();
}