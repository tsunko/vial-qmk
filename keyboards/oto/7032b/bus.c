#include "hardware/clocks.h"
#include "quantum.h"
#include "gpio.h"
#include "bus.h"

void bus_init(bus_pins bus) {
    for (size_t i = 0; i < 8; i++) {
        setPinOutput(bus.data[i]);
    }

    setPinOutput(bus.wr);
    setPinInputLow(bus.busy);
}

void bus_write8(bus_pins bus, const uint8_t val){
    // store our last written byte value
    // we use this to prevent having to waste cycles setting redundent/same pin states 
    static uint8_t last = 0;
    bus_wait_busy(bus);

    // get changed bits between our last and current value
    uint8_t diff = last ^ val;
    
    // then set pins hi/lo based on the differences
    for (size_t i = 0; i < 8; i++) {        
        if (((diff >> i) & 1) > 0) {
            if (((val >> i) & 1) > 0) {
                writePinHigh(bus.data[i]);
            } else {
                writePinLow(bus.data[i]);
            }
        }
    }

    // now notify the display that a new byte is on the line
    writePinHigh(bus.wr);
    writePinLow(bus.wr);

    // then make sure we set last byte to the current
    last = val;
}

void bus_write8Arr(bus_pins bus, const uint8_t val[], size_t length) {
    for (size_t i = 0; i < length; i++) { 
        bus_write8(bus, val[i]);
    }
}

void bus_write16(bus_pins bus, const uint16_t val) {
    uint8_t lo = (uint8_t)(val & 0xFF); 
    uint8_t hi = (uint8_t)(val >> 8);

    bus_write8(bus, lo);
    bus_write8(bus, hi);
}

void bus_write16Arr(bus_pins bus, const uint16_t val[], size_t length) {
    for (size_t i = 0; i < length; i++) { 
        bus_write16(bus, val[i]);
    }
}

inline void bus_wait_busy(bus_pins bus) {
    while (readPin(bus.busy) > 0) {
        asm("yield");
    }
}