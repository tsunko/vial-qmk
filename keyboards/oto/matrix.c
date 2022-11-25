#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// have to define it here since we need a definition of matrix_row_t; better
// play it safe and just use the existing matrix.h checks instead of
// typedefing it
#define MATRIX_COLS 15
#define MATRIX_IO_DELAY 30
#include "matrix.h"
#include "debounce.h"
#include "quantum.h"
#include "util.h"
#include "print.h"

// 595 shift-register related pins
#define SR_CLK GP18
#define SR_SER GP19
#define SR_LATCH GP14
#define SR_CLR GP11

// column definitions
static const uint16_t col_values[MATRIX_COLS] = { 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };
static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;

// raw, from pins, state values
static matrix_row_t raw_matrix[MATRIX_ROWS];
// debounced matrix values
static matrix_row_t matrix[MATRIX_ROWS];

static inline void pinOutWriteLow(pin_t pin) {
	ATOMIC_BLOCK_FORCEON {
		setPinOutput(pin);
		writePinLow(pin);
	}
}

static inline void pinOutWriteHigh(pin_t pin) {
	ATOMIC_BLOCK_FORCEON {
		setPinOutput(pin);
		writePinHigh(pin);
	}
}

static inline void pinInLow(pin_t pin) {
	ATOMIC_BLOCK_FORCEON {
		setPinInputLow(pin);
	}
}

__attribute__((weak)) void matrix_io_delay(void) { wait_us(MATRIX_IO_DELAY); }
__attribute__((weak)) void matrix_output_select_delay(void) { waitInputPinDelay(); }
__attribute__((weak)) void matrix_output_unselect_delay(uint8_t line, bool key_pressed) { matrix_io_delay(); }
__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }
__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }
__attribute__((weak)) void matrix_init_user(void) {}
__attribute__((weak)) void matrix_scan_user(void) {}

void print_matrix(matrix_row_t m[]) {
	print("0123456789ABCDEF\n");
	for (int r=0; r < MATRIX_ROWS; r++) {
		matrix_row_t row = m[r];
		print_bin_reverse16(row);
		print("\n");
	}
}

matrix_row_t matrix_get_row(uint8_t row) {
	return matrix[row];
}

void matrix_print(void) {
	print_matrix(matrix);
}

void matrix_init(void) {
	// prevent shift register from clearing itself
	pinOutWriteHigh(SR_CLR);

	// initialize rows as input pins
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		pinInLow(row_pins[r]);
	}

	debounce_init(MATRIX_ROWS);
	matrix_init_quantum();
}

static inline void shift_pulse(void) {
	pinOutWriteHigh(SR_CLK);
	pinOutWriteLow(SR_CLK);
}

static void shift_out_single(uint8_t value) {
	for (uint8_t i = 0; i < 8; i++) {
		if (value & 0b10000000) {
			pinOutWriteHigh(SR_SER);
		} else {
			pinOutWriteLow(SR_SER);
		}

		shift_pulse();
		value <<= 1;
	}
}

static void shift_out(uint16_t value) {
	pinOutWriteLow(SR_LATCH);

	uint8_t hi = (value >> 8) & 0xFF;
	uint8_t lo = (uint8_t)(value & 0xFF);

	shift_out_single(hi);
	shift_out_single(lo);

	pinOutWriteHigh(SR_LATCH);
}

static void select_col(uint8_t col) {
	shift_out(col_values[col]);
}

void matrix_read_rows_on_col(matrix_row_t immediate_matrix[], uint8_t current_col, matrix_row_t row_shifter) { 
	bool key_pressed = false;

	select_col(current_col);
	matrix_output_select_delay();

	for (uint8_t current_row = 0; current_row < MATRIX_ROWS; current_row++) {
		uint8_t pin_state = readPin(row_pins[current_row]);
		if (pin_state == 0) {
			// pin off, unset bit
			immediate_matrix[current_row] &= ~row_shifter;
		} else {
			immediate_matrix[current_row] |= row_shifter;
			key_pressed = true;
		}
	}

	matrix_output_unselect_delay(current_col, key_pressed);
}

uint8_t matrix_scan(void) {
	matrix_row_t immediate_matrix[MATRIX_ROWS] = { 0 };

	matrix_row_t row_shifter = MATRIX_ROW_SHIFTER;
	for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++) {
		matrix_read_rows_on_col(immediate_matrix, current_col, row_shifter);
		row_shifter <<= 1;
	}

	bool dirty = memcmp(raw_matrix, immediate_matrix, sizeof(immediate_matrix)) != 0;
	if (dirty) memcpy(raw_matrix, immediate_matrix, sizeof(immediate_matrix));
	dirty = debounce(raw_matrix, matrix, MATRIX_ROWS, dirty);

	matrix_scan_quantum();
	return dirty;
}
