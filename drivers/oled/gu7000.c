#include <stdint.h>
#include <string.h>
#include "timer.h"
#include "glcdfont_gu7000compat.c"
#include "gpio.h"

// define everything _before_ including oled_driver.h
#define OLED_DISPLAY_CUSTOM
#define OLED_DISPLAY_WIDTH 256
#define OLED_DISPLAY_HEIGHT 64
#define OLED_MATRIX_SIZE (OLED_DISPLAY_HEIGHT / 8 * OLED_DISPLAY_WIDTH) // 2048
// we use a 16x64 block chunking size, to simplify updates
#define OLED_BLOCK_TYPE uint16_t
#define OLED_BLOCK_COUNT (OLED_DISPLAY_WIDTH / 16)
#define OLED_BLOCK_WIDTH 16
#define OLED_BLOCK_HEIGHT 64
#define OLED_BLOCK_SIZE (OLED_BLOCK_HEIGHT / 8 * OLED_BLOCK_WIDTH)

#define OLED_SOURCE_MAP { 32, 40, 48, 56 }
#define OLED_TARGET_MAP { 24, 16, 8, 0 }
#define OLED_ALL_BLOCKS_MASK (~((OLED_BLOCK_TYPE)1 & 0))
#include "oled_driver.h"

#define BIT_SET(value, i) (((value) >> i) & 1)

#if !defined(GU7000_DATA_PINS)
#   error Missing data pins definition
#endif
#if !defined(GU7000_BUSY_PIN)
#   error Missing busy pin definition
#endif
#if !defined(GU7000_WR_PIN)
#   error Missing wr pin definition
#endif

pin_t comm_data_pins[] = GU7000_DATA_PINS;
pin_t comm_busy_pin = GU7000_BUSY_PIN;
pin_t comm_wr_pin = GU7000_WR_PIN;

uint8_t         oled_buffer[OLED_MATRIX_SIZE];
uint8_t        *oled_cursor;
OLED_BLOCK_TYPE oled_dirty        = 0;
bool            oled_initialized  = false;
bool            oled_active       = false;
bool            oled_inverted     = false;
bool            oled_scrolling    = false;
uint8_t         oled_brightness   = 0;
uint8_t         oled_scroll_speed = 0;
uint8_t         oled_scroll_start = 0;
uint8_t         oled_scroll_end   = 0;

#if OLED_TIMEOUT > 0
uint32_t        oled_timeout;
#endif
#if OLED_SCROLL_TIMEOUT > 0
uint32_t        oled_scroll_timeout;
#endif
#if OLED_UPDATE_INTERVAL > 0
uint16_t        oled_update_timeout;
#endif

void _gu7000_init_pins(void);
void _gu7000_write8(uint8_t value);
void _gu7000_write16(uint16_t value);
void _gu7000_writeUS(void){ _gu7000_write8(0x1F); _gu7000_write8(0x28); }

__attribute__((weak)) oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    return rotation;
}
__attribute__((weak)) oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return rotation;
}

bool oled_init(oled_rotation_t rotation) {
    // the 7933B supports OLED_ROTATION_180, but no test board or documentation available
    oled_init_user(oled_init_kb(rotation));

    _gu7000_init_pins();

    _gu7000_write8(0x1B);
    _gu7000_write8(0x40);
    
    oled_clear();

    oled_initialized = true;
    oled_active = true;

    return rotation;
}

void oled_clear(void) {
    memset(oled_buffer, 0, OLED_MATRIX_SIZE);
    oled_set_cursor(0, 0);
}

void oled_render(void) {
    if (!oled_initialized) {
        return;
    }

    // check if we even have anything to render
    // oled_dirty &= OLED_ALL_BLOCKS_MASK;
    if (!oled_dirty || oled_scrolling) {
        return;
    }
    
    uint8_t update_start = __builtin_ffs(oled_dirty) - 1;

    // found a directy block - begin writing the command and relevant data
    _gu7000_writeUS();
    _gu7000_write8(0x64);
    _gu7000_write8(0x21);
    _gu7000_write16(update_start * 16);
    _gu7000_write16(0x0000);
    _gu7000_write16(OLED_BLOCK_WIDTH);
    _gu7000_write16(OLED_BLOCK_HEIGHT);
    _gu7000_write8(0x01);
    
    for (size_t i = 0; i < OLED_BLOCK_SIZE; i++) {
        _gu7000_write8(oled_buffer[update_start * OLED_BLOCK_SIZE + i]);
    }

    oled_on();

    // clear dirty flag for the given chunk
    oled_dirty &= ~((OLED_BLOCK_TYPE)1 << update_start);
}

void oled_set_cursor(uint8_t col, uint8_t line) {
    uint16_t index = (col * (OLED_DISPLAY_HEIGHT / 8)) + line;
    if (index >= OLED_MATRIX_SIZE) {
        index = 0;
    }
    oled_cursor = &oled_buffer[index];
}

void oled_advance_page(bool clearPageRemainder) {
    uint16_t index = oled_cursor - &oled_buffer[0];
    uint16_t remaining = OLED_DISPLAY_WIDTH - (index % OLED_DISPLAY_WIDTH);

    if (clearPageRemainder) {
        remaining /= OLED_FONT_WIDTH;

        while (remaining--) {
            oled_write_char(' ', false);
        }
    } else {
        uint16_t newIndex = index + remaining;
        if (newIndex >= OLED_MATRIX_SIZE) {
            newIndex = 0;
        }
        oled_cursor = &oled_buffer[newIndex];
    }
}

void oled_advance_char(void) {
    uint16_t current = oled_cursor - &oled_buffer[0]; // current index into oled_buffer
    current += (OLED_DISPLAY_HEIGHT / 8) * OLED_FONT_WIDTH; // add 1 character's worth of space

    if (current > OLED_MATRIX_SIZE) {
        current %= OLED_MATRIX_SIZE;
    }

    oled_cursor = &oled_buffer[current];
}

void oled_write_char(const char data, bool invert) {
    if (data == '\n') {
        oled_advance_page(true);
        return;
    }

    if (data == '\r') {
        oled_advance_page(false);
        return;
    }

    uint16_t current = oled_cursor - &oled_buffer[0];

    const uint16_t glyphPosition = (uint8_t)data * OLED_FONT_WIDTH;
    const uint8_t *glyph = &font[glyphPosition];

    uint16_t index;
    for (uint16_t i = 0; i < OLED_FONT_WIDTH; i++) {        
        index = current + (i * (OLED_DISPLAY_HEIGHT / 8));
        
        uint8_t gCol = glyph[i];
        if(invert){
            // invert bits
            gCol = ~gCol;
        }

        if(oled_buffer[index] != gCol){
            oled_buffer[index] = gCol;
            oled_dirty |= ((OLED_BLOCK_TYPE)1 << (index / OLED_BLOCK_SIZE));
        }
    }

    oled_advance_char();
}

void oled_write(const char *data, bool invert) {    
    while(*data){
        oled_write_char(*data++, invert);
    }
}

void oled_write_ln(const char *data, bool invert) {
    oled_write(data, invert);
    oled_advance_page(true);
}

// CAVEAT: left panning is unsupported - no documented way
void oled_pan(bool left) {
    uint16_t i = 0;
    for (uint16_t y = 0; y < OLED_DISPLAY_HEIGHT / 8; y++) {
        if (left) {
            for (uint16_t x = 0; x < OLED_DISPLAY_WIDTH - 1; x++) {
                i              = y * OLED_DISPLAY_WIDTH + x;
                oled_buffer[i] = oled_buffer[i + 1];
            }
        } else {
            for (uint16_t x = OLED_DISPLAY_WIDTH - 1; x > 0; x--) {
                i              = y * OLED_DISPLAY_WIDTH + x;
                oled_buffer[i] = oled_buffer[i - 1];
            }
        }
    }
    oled_dirty = OLED_ALL_BLOCKS_MASK;
}

oled_buffer_reader_t oled_read_raw(uint16_t start_index) {
    if (start_index > OLED_MATRIX_SIZE) start_index = OLED_MATRIX_SIZE;
    oled_buffer_reader_t ret_reader;
    ret_reader.current_element         = &oled_buffer[start_index];
    ret_reader.remaining_element_count = OLED_MATRIX_SIZE - start_index;
    return ret_reader;
}

void oled_write_raw_byte(const char data, uint16_t index) {
    if (index > OLED_MATRIX_SIZE) index = OLED_MATRIX_SIZE;
    if (oled_buffer[index] == data) return;
    oled_buffer[index] = data;
    oled_dirty |= ((OLED_BLOCK_TYPE)1 << (index / OLED_BLOCK_SIZE));
}

void oled_write_raw(const char *data, uint16_t size) {
    uint16_t cursor_start_index = oled_cursor - &oled_buffer[0];
    if ((size + cursor_start_index) > OLED_MATRIX_SIZE) size = OLED_MATRIX_SIZE - cursor_start_index;
    for (uint16_t i = cursor_start_index; i < cursor_start_index + size; i++) {
        uint8_t c = *data++;
        if (oled_buffer[i] == c) continue;
        oled_buffer[i] = c;
        oled_dirty |= ((OLED_BLOCK_TYPE)1 << (i / OLED_BLOCK_SIZE));
    }
}

void oled_write_pixel(uint8_t x, uint8_t y, bool on) {
    if (x >= OLED_DISPLAY_WIDTH) {
        return;
    }
    uint16_t index = x + (y / 8) * OLED_DISPLAY_WIDTH;
    if (index >= OLED_MATRIX_SIZE) {
        return;
    }
    uint8_t data = oled_buffer[index];
    if (on) {
        data |= (1 << (y % 8));
    } else {
        data &= ~(1 << (y % 8));
    }
    if (oled_buffer[index] != data) {
        oled_buffer[index] = data;
        oled_dirty |= ((OLED_BLOCK_TYPE)1 << (index / OLED_BLOCK_SIZE));
    }
}

bool oled_on(void) {
    if (!oled_initialized) {
        return !oled_active;
    }

#if OLED_TIMEOUT > 0
    oled_timeout = timer_read32() + OLED_TIMEOUT;
#endif

    if (!oled_active) {
        _gu7000_writeUS();
        _gu7000_write8(0x61);
        _gu7000_write8(0x40);
        _gu7000_write8(0x01);
        oled_active = true;
    }

    return oled_active;
}

bool oled_off(void) {
    if (!oled_initialized) {
        return !oled_active;
    }

    if (oled_active) {
        _gu7000_writeUS();
        _gu7000_write8(0x61);
        _gu7000_write8(0x40);
        _gu7000_write8(0x00);
        oled_active = false;
    }

    return !oled_active;
}

bool is_oled_on(void) {
    return oled_active;
}

uint8_t oled_set_brightness(uint8_t level) {
    if (level < 0x01 || level > 0x08 || oled_brightness == level) {
        return oled_brightness;
    }

    _gu7000_write8(0x1F);
    _gu7000_write8(0x58);
    _gu7000_write8(level);
    oled_brightness = level;
    return oled_brightness;
}

uint8_t oled_get_brightness(void) {
    return oled_brightness;
}

void oled_scroll_set_area(uint8_t start_line, uint8_t end_line) {
    oled_scroll_start = start_line;
    oled_scroll_end = end_line;
}

void oled_scroll_set_speed(uint8_t speed) {
    oled_scroll_speed = speed;
}

bool oled_scroll_right(void) {
    if (!oled_initialized) {
        return oled_scrolling;
    }

    if (!oled_dirty && !oled_scrolling) {
        _gu7000_writeUS();
        _gu7000_write8(0x61);
        _gu7000_write8(0x10);

        // TODO: validate what scrolling even looks like
        _gu7000_write16(oled_scroll_end - oled_scroll_start);
        _gu7000_write16(1);
        _gu7000_write8(oled_scroll_speed);
        oled_scrolling = true;
    }

    return oled_scrolling;
}

bool oled_scroll_left(void) {
    oled_scroll_right(); // lol i have no idea what scrolling looks like
    return oled_scrolling;
}

bool oled_scroll_off(void) {
    if (!oled_initialized) {
        return !oled_scrolling;
    }

    // the GU7000 doesn't have infinite scrolling...
    oled_scrolling = false;
    return oled_scrolling;
}

bool is_oled_scrolling(void) {
    return oled_scrolling;
}

bool oled_invert(bool invert) {
    if (!oled_initialized) {
        return oled_inverted;
    }
    
    if (invert && !oled_inverted) {
        _gu7000_write8(0x1F);
        _gu7000_write8(0x72);
        _gu7000_write8(0x01);
        oled_inverted = true;
    } else if (!invert && oled_inverted) {
        _gu7000_write8(0x1F);
        _gu7000_write8(0x72);
        _gu7000_write8(0x00);
        oled_inverted = false;
    }

    return oled_inverted;
}

uint8_t oled_max_chars(void) {
    return OLED_DISPLAY_WIDTH / OLED_FONT_WIDTH;
}

uint8_t oled_max_lines(void) {
    return OLED_DISPLAY_HEIGHT / OLED_FONT_HEIGHT;
}

__attribute__((weak)) bool oled_task_kb(void) {
    return oled_task_user();
}

__attribute__((weak)) bool oled_task_user(void) {
    return true;
}

void oled_task(void) {
    if (!oled_initialized) {
        return;
    }

#if OLED_UPDATE_INTERVAL > 0
    if (timer_elapsed(oled_update_timeout) >= OLED_UPDATE_INTERVAL) {
        oled_update_timeout = timer_read();
        oled_set_cursor(0, 0);
        oled_task_kb();
    }
#else
    oled_set_cursor(0, 0);
    oled_task_kb();
#endif

#if OLED_SCROLL_TIMEOUT > 0
    if (oled_dirty && oled_scrolling) {
        oled_scroll_timeout = timer_read32() + OLED_SCROLL_TIMEOUT;
        oled_scroll_off();
    }
#endif

    oled_render();

    // Display timeout check
#if OLED_TIMEOUT > 0
    if (oled_active && timer_expired32(timer_read32(), oled_timeout)) {
        oled_off();
    }
#endif

#if OLED_SCROLL_TIMEOUT > 0
    if (!oled_scrolling && timer_expired32(timer_read32(), oled_scroll_timeout)) {
#    ifdef OLED_SCROLL_TIMEOUT_RIGHT
        oled_scroll_right();
#    else
        oled_scroll_left();
#    endif
    }
#endif
}

void _gu7000_init_pins(void) {
    for (size_t i = 0; i < 8; i++) {
        setPinOutput(comm_data_pins[i]);
    }

    setPinInputLow(comm_busy_pin);
    setPinOutput(comm_wr_pin);
}

void _gu7000_write8(uint8_t value) {
    // last written value to save cycles on setting already-set pins
    static uint8_t lastValue = 0;

    // wait for busy to go low
    while (readPin(comm_busy_pin) > 0) {
        asm("yield");
    }
    
    const uint8_t diff = lastValue ^ value;
    for (uint8_t i = 0; i < 8; i++) {
        if (BIT_SET(diff, i)) {
            if(BIT_SET(value, i)) {
                writePinHigh(comm_data_pins[i]);
            } else {
                writePinLow(comm_data_pins[i]);
            }
        }
    }

    // pulse wr line
    writePinHigh(comm_wr_pin);
    // TODO: insert 110ns delay here
    writePinLow(comm_wr_pin);

    lastValue = value;
}

void _gu7000_write16(uint16_t value) {
    const uint8_t lo = (uint8_t)(value & 0xFF);
    const uint8_t hi = (uint8_t)(value >> 8);

    _gu7000_write8(lo);
    _gu7000_write8(hi);
}