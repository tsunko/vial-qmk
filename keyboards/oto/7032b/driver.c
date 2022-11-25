#include <stdarg.h>
#include "quantum.h"
#include "driver.h"


uint16_t viewportWidth;
uint16_t viewportHeight;
bus_pins bus;

void disp_initialize(uint16_t width, uint16_t height, pin_t data[8], pin_t wr, pin_t busy) {
    viewportWidth = width;
    viewportHeight = height;

    memcpy(bus.data, data, sizeof(data[0]) * 8);
    bus.wr = wr;
    bus.busy = busy;

    bus_init(bus);

    // technically supposed to only wait 200 microseconds, 
    wait_ms(1);
    bus_write8Arr(bus, (uint8_t[]){0x1B, '@'}, 2);
}

void disp_wait(uint8_t cycles) {
    disp_writeUSPrefix();
    bus_write8Arr(bus, (uint8_t[]){ 0x1F, 0x28 }, 2);
}

void disp_backspace(void) {
    bus_write8(bus, 0x08);
}

void disp_clear(void) {
    bus_write8(bus, 0x0C);
}

// Cursor functions
void disp_cursorHome(void) {
    bus_write8(bus, 0x0B);
}

void disp_cursorForward(void) {
    bus_write8(bus, 0x09);
}

void disp_cursorMove(uint16_t x, uint16_t y) {
    bus_write8Arr(bus, (uint8_t[]){ 0x1F, '$' }, 2);
    bus_write16Arr(bus, (uint16_t[]){ x, (y / PIXELS_PER_BYTE) }, 2); 
}

void disp_cursorSetVisible(bool visible) {
    bus_write8Arr(bus, (uint8_t[]){ 0x1F, 'C', visible ? 1 : 0 }, 3);
}

// Custom character set functions
void disp_customCharEnable(bool use) {

}

void disp_customCharCreate(uint8_t codepoint, enum FontFormat format, uint8_t data[], size_t length) {

}

void disp_customCharDelete(uint8_t codepoint) {

}

// Control ASCII character set
void disp_asciiSetVariant(enum AsciiVariant variant) {

}

void disp_asciiSetCharset(enum Charset charset) {

}

// Display properties manipulation functions
// No prefix since this pertains to the display itself
void disp_setScrollMode(enum ScrollMode mode) {

}

void disp_setHorizontalScrollSpeed(uint8_t speed) {

}

void disp_setInversion(bool invert) {

}

void disp_setCompositionMode(enum CompositionMode mode) {
    bus_write8Arr(bus, (uint8_t[]){ 0x1F, 'w', (uint8_t)mode }, 3);
}

void disp_setBrightness(uint8_t brightness) {

}

void disp_scrollScreen(uint16_t x, uint16_t y, uint16_t times, uint8_t speed) {

}

void disp_configureBlinking(bool enable, bool reverse, uint8_t durationOn, uint8_t durationOff, uint8_t times) {

}

void disp_setScreenStatus(enum ScreenStatus status) {

}

void disp_setFontStyle(bool proportional, bool evenSpacing) {

}

void disp_setFontSize(uint8_t width, uint8_t height, bool tall) {

}

// Window functions
void disp_windowDefine(uint8_t id, bool defining, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {

}

void disp_windowSelect(uint8_t id) {

}

void disp_windowSetArticulation(bool jointed) {

}

// Actual printing/drawing functions
void disp_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[128];
    size_t written = vsprintf(buffer, fmt, args);

    if (written >= 0) {
        for (size_t i = 0; i < written; i++) {
            bus_write8(bus, buffer[i]);
        }
    }

    // TODO: handle written < 0
    va_end(args);
}

void disp_drawImage(uint16_t w, uint16_t h, const uint8_t data[]) {
    disp_writeUSPrefix();

    bus_write8(bus, 0x66);
    bus_write8(bus, 0x11);

    bus_write16(bus, w);
    bus_write16(bus, h / PIXELS_PER_BYTE);
    bus_write8(bus, 0x01);

    for (size_t i = 0; i < w * (h / PIXELS_PER_BYTE); i++) {
        bus_write8(bus, data[i]);
    }
}

void disp_drawImageXY(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t data[]) {
    disp_writeUSPrefix();

    bus_write8(bus, 0x64);
    bus_write8(bus, 0x21);

    bus_write16(bus, x);
    bus_write16(bus, y);

    bus_write16(bus, w);
    bus_write16(bus, h);
    bus_write8(bus, 0x01);

    for (size_t i = 0; i < w * (h / PIXELS_PER_BYTE); i++) {
        bus_write8(bus, data[i]);
    }
}

void disp_writeUSPrefix(void) {
    // bus_write8Arr(bus, (uint8_t[]){ 0x1F, 0x28 }, 2);
    bus_write8(bus, 0x1F);
    bus_write8(bus, 0x28);
}