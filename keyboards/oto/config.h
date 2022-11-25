// Copyright 2022 tsunko (@tsunko)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define DEBOUNCE 5

#define MATRIX_ROWS 6
#define MATRIX_COLS 15
#define MATRIX_ROW_PINS { GP0, GP1, GP2, GP3, GP4, GP5 }

#define ENCODERS_PAD_A { GP6 }
#define ENCODERS_PAD_B { GP7 }
#define ENCODER_RESOLUTION 12

#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 200U

#define DEBUG_MATRIX_SCAN_RATE

#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 64
#define DISPLAY_DATA_PINS { GP21, GP23, GP24, GP25, GP26, GP27, GP28, GP29 }
//#define DISPLAY_DATA_PINS { GP29, GP28, GP27, GP26, GP25, GP24, GP23, GP21 }
#define DISPLAY_WRITE_PIN GP22
#define DISPLAY_BUSY_PIN GP20

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
