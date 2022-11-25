#include <stdint.h>
#include "bus.h"

#define PIXELS_PER_BYTE 8

enum FontFormat {
    Thin, // 5x7 pixels
    Wide, // 7x8 pixels
    CUU, 
    LCD, 
};

enum AsciiVariant {
    America,
    France,
    Germany,
    England,
    Denmark_1,
    Sweden,
    Italy,
    Spain_1,
    Japan,
    Norway,
    Denmark_2,
    Spain_2,
    Latin_America,
    Korea,
};

enum Charset {
    USA_Euro_Standard = 0x00,
    Katakana          = 0x01,
    Multilingual      = 0x02,
    Portuguese        = 0x03,
    Canadian_French   = 0x04,
    Noridc            = 0x05,
    WPC1252           = 0x10,
    Cyrillic          = 0x11,
    Latin             = 0x12,
    PC858             = 0x13,
};

enum ScrollMode {
    Wrapping,
    Vertical,
    Horizontal,
};

enum CompositionMode {
    Normal,
    OR,
    AND,
    XOR,
};

enum ScreenStatus {
    PowerOff,
    PowerOn,
    AllDotsOff,
    AllDotsOn, 
    Blink, // blinks the display; most likely similar to configureBlinking().
};

// Helper functions and whatnot
void disp_initialize(uint16_t width, uint16_t height, pin_t data[8], pin_t wr, pin_t busy);
void disp_wait(uint8_t cycles);
void disp_backspace(void);
void disp_clear(void);

// Cursor functions
void disp_cursorHome(void);
void disp_cursorForward(void);
void disp_cursorMove(uint16_t x, uint16_t y);
void disp_cursorSetVisible(bool visible);

// Custom character set functions
void disp_customCharEnable(bool use);
void disp_customCharCreate(uint8_t codepoint, enum FontFormat format, uint8_t data[], size_t length);
void disp_customCharDelete(uint8_t codepoint);

// Control ASCII character set
void disp_asciiSetVariant(enum AsciiVariant variant);
void disp_asciiSetCharset(enum Charset charset);

// Display properties manipulation functions
// No prefix since this pertains to the display itself
void disp_setScrollMode(enum ScrollMode mode);
void disp_setHorizontalScrollSpeed(uint8_t speed);
void disp_setInversion(bool invert);
void disp_setCompositionMode(enum CompositionMode mode);
void disp_setBrightness(uint8_t brightness);
void disp_scrollScreen(uint16_t x, uint16_t y, uint16_t times, uint8_t speed);
void disp_configureBlinking(bool enable, bool reverse, uint8_t durationOn, uint8_t durationOff, uint8_t times);
void disp_setScreenStatus(enum ScreenStatus status);
void disp_setFontStyle(bool proportional, bool evenSpacing);
void disp_setFontSize(uint8_t width, uint8_t height, bool tall);

// Window functions
void disp_windowDefine(uint8_t id, bool defining, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void disp_windowSelect(uint8_t id);
void disp_windowSetArticulation(bool jointed);

// Actual printing/drawing functions
void disp_printf(const char* format, ...);
void disp_drawImage(uint16_t w, uint16_t h, const uint8_t data[]);
void disp_drawImageXY(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t data[]);

// some functions require a special prefix to access
void disp_writeUSPrefix(void);