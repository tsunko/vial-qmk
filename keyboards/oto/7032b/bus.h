#include <stdint.h>

typedef struct bus {
    pin_t data[8];
    pin_t wr;
    pin_t busy;
} bus_pins;

void bus_init(bus_pins bus);
void bus_write8(bus_pins bus, const uint8_t val);
void bus_write8Arr(bus_pins bus, const uint8_t val[], size_t length);
void bus_write16(bus_pins bus, const uint16_t val);
void bus_write16Arr(bus_pins bus, const uint16_t val[], size_t length);
void bus_wait_busy(bus_pins bus);