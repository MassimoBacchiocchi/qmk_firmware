/*
Copyright 2012-2018 Jun Wako, Jack Humbert, Yiancar
Copyright 2019 Evy Dekkers

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "wait.h"
#include "util.h"
#include "matrix.h"
#include "debounce.h"

#ifdef DIRECT_PINS
static pin_t direct_pins[MATRIX_ROWS][MATRIX_COLS] = DIRECT_PINS;
#elif (DIODE_DIRECTION == ROW2COL) || (DIODE_DIRECTION == COL2ROW)
static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
//static const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;
#endif

// matrix code

#ifdef DIRECT_PINS

static void init_pins(void) {
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            pin_t pin = direct_pins[row][col];
            if (pin != NO_PIN) {
                gpio_set_pin_input_high(pin);
            }
        }
    }
}

static bool read_cols_on_row(matrix_row_t current_matrix[], uint8_t current_row) {
    matrix_row_t last_row_value = current_matrix[current_row];
    current_matrix[current_row] = 0;

    for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
        pin_t pin = direct_pins[current_row][col_index];
        if (pin != NO_PIN) {
            current_matrix[current_row] |= gpio_read_pin(pin) ? 0 : (MATRIX_ROW_SHIFTER << col_index);
        }
    }

    return (last_row_value != current_matrix[current_row]);
}

#elif (DIODE_DIRECTION == ROW2COL)

/* Cols 0 - 16
 * These columns use two 74HC138 3 to 8 bit demultiplexer. B0, F1 is the enable pin, must be set high (1) to use it.
 *
 * col / pin:     PB5  PB7  PF0  PB0  PF1  PE6
 * 0:              0 ── 0 ── 0    1 ── 0    0 
 * ────────────────────────────────────────────
 * 1:              0 ── 0 ── 1    1 ── 0    0
 * ────────────────────────────────────────────    
 * 2:              0 ── 1 ── 0    1 ── 0    0 
 * ────────────────────────────────────────────    
 * 3:              0 ── 1 ── 1    1 ── 0    0 
 * ────────────────────────────────────────────    
 * 4:              1 ── 0 ── 0    1 ── 0    0 
 * ────────────────────────────────────────────    
 * 5:              1 ── 0 ── 1    1 ── 0    0 
 * ────────────────────────────────────────────    
 * 6:              1 ── 1 ── 0    1 ── 0    0 
 * ────────────────────────────────────────────    
 * 7:              1 ── 1 ── 1    1 ── 0    0 
 * ────────────────────────────────────────────    
 * 8:              0 ── 0 ── 0    0 ── 1    0 
 * ────────────────────────────────────────────    
 * 9:              0 ── 0 ── 1    0 ── 1    0 
 * ────────────────────────────────────────────    
 *10:              0 ── 1 ── 0    0 ── 1    0 
 * ────────────────────────────────────────────    
 *11:              0 ── 1 ── 1    0 ── 1    0 
 * ────────────────────────────────────────────    
 *12:              1 ── 0 ── 0    0 ── 1    0 
 * ────────────────────────────────────────────    
 *13:              1 ── 0 ── 1    0 ── 1    0 
 * ────────────────────────────────────────────    
 *14:              1 ── 1 ── 1    0 ── 1    0 
 * ────────────────────────────────────────────    
 *15:              1 ── 1 ── 0    0 ── 1    0 
 * ────────────────────────────────────────────    
 *16:              0 ── 0 ── 0    0 ── 0    1  
 *
 */
static void select_col(uint8_t col) {
    switch (col) {
        case 0:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(B0);
          break;
        case 1:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(B0);
          break;
        case 2:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(B0);
          break;
        case 3:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(B0);
          break;
        case 4:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(B0);
          break;
        case 5:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(B0);
          break;
        case 6:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(B0);
          break;
        case 7:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(B0);
          break;
        case 8:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(F1);
          break;
        case 9:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(F1);
          break;
        case 10:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(F1);
          break;
        case 11:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(F1);
          break;
        case 12:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(F1);
          break;
        case 13:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(F1);
          break;
        case 14:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_high(F1);
          break;
        case 15:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_high(F1);
          break;
        case 16:
          gpio_write_pin_low(E6);
          break;
    }
}

static void unselect_col(uint8_t col) {
    switch (col) {
        case 0:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(B0);
          break;
        case 1:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(B0);
          break;
        case 2:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(B0);
          break;
        case 3:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(B0);
          break;
        case 4:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(B0);
          break;
        case 5:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(B0);
          break;
        case 6:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(B0);
          break;
        case 7:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(B0);
          break;
        case 8:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(F1);
          break;
        case 9:
          gpio_write_pin_high(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(F1);
          break;
        case 10:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(F1);
          break;
        case 11:
          gpio_write_pin_high(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(F1);
          break;
        case 12:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(F1);
          break;
        case 13:
          gpio_write_pin_low(B5);
          gpio_write_pin_high(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(F1);
          break;
        case 14:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_low(F0);
          gpio_write_pin_low(F1);
          break;
        case 15:
          gpio_write_pin_low(B5);
          gpio_write_pin_low(B7);
          gpio_write_pin_high(F0);
          gpio_write_pin_low(F1);
          break;
        case 16:
          gpio_write_pin_high(E6);
          break;
    }
}

static void unselect_cols(void) {
    //Native
    gpio_write_pin_high(E6);

    //Demultiplexer
    gpio_write_pin_low(B0);
    gpio_write_pin_low(F1);
    gpio_write_pin_high(B5);
    gpio_write_pin_high(B7);
    gpio_write_pin_high(F0);
}

static void init_pins(void) {
    unselect_cols();
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        gpio_set_pin_input_high(row_pins[x]);
    }
    gpio_set_pin_output(B5);
    gpio_set_pin_output(B7);
    gpio_set_pin_output(F0);
    gpio_set_pin_output(B0);
    gpio_set_pin_output(F1);
    gpio_set_pin_output(E6);
}

static bool read_rows_on_col(matrix_row_t current_matrix[], uint8_t current_col) {
    bool matrix_changed = false;

    // Select col and wait for col selecton to stabilize
    select_col(current_col);
    wait_us(30);

    // For each row...
    for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
        // Store last value of row prior to reading
        matrix_row_t last_row_value = current_matrix[row_index];

        // Check row pin state
        if (gpio_read_pin(row_pins[row_index]) == 0) {
            // Pin LO, set col bit
            current_matrix[row_index] |= (MATRIX_ROW_SHIFTER << current_col);
        } else {
            // Pin HI, clear col bit
            current_matrix[row_index] &= ~(MATRIX_ROW_SHIFTER << current_col);
        }

        // Determine if the matrix changed state
        if ((last_row_value != current_matrix[row_index]) && !(matrix_changed)) {
            matrix_changed = true;
        }
    }

    // Unselect col
    unselect_col(current_col);

    return matrix_changed;
}

#endif

void matrix_init_custom(void) {
    // initialize key pins
    init_pins();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;

#if defined(DIRECT_PINS) || (DIODE_DIRECTION == ROW2COL)
    // Set col, read rows
    for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++) {
        changed |= read_rows_on_col(current_matrix, current_col);
    }
#endif

    return changed;
}

bool led_update_kb(led_t led_state) {
    bool res = led_update_user(led_state);
    if(res) {
        // gpio_write_pin sets the pin high for 1 and low for 0.
        // In this example the pins are inverted, setting
        // it low/0 turns it on, and high/1 turns the LED off.
        // This behavior depends on whether the LED is between the pin
        // and VCC or the pin and GND.
        gpio_write_pin(B0, !led_state.num_lock);
        gpio_write_pin(B1, !led_state.caps_lock);
        gpio_write_pin(B2, !led_state.scroll_lock);
        gpio_write_pin(B3, !led_state.compose);
        gpio_write_pin(B4, !led_state.kana);
    }
    return res;
}