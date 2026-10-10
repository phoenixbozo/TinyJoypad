/*
 * TinyJoypad Emulator - ATtiny85 + SSD1306 (GPLv3)
 *
 * Copyright (C) 2026 Électro L.I.B / ()Daniel C.)
 *
 * Code entirely written by Grok (xAI)
 * under the direction and with the assistance of Électro L.I.B (Daniel C.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Official website: https://www.tinyjoypad.com
 * 
 * ==========================================================================
 * SPECIAL ACKNOWLEDGEMENTS & GRATITUDE
 * --------------------------------------------------------------------------
 * I want to express my deepest gratitude to Michel Pollet (buserror) and 
 * everyone who contributed to the 'simavr' project (https://github.com/buserror/simavr).
 * 
 * To be completely honest, this emulator would simply not exist without simavr. 
 * It is the absolute core and the true foundation of this project. The incredible 
 * work, precision, and dedication put into simavr made bringing the TinyJoypad 
 * experience to the desktop a reality. 
 * 
 * Thank you for creating such an exceptional piece of software!
 * =================================================================        
 */
 
#ifndef I2C_BB_H
#define I2C_BB_H

#include <stdint.h>
#include <stdbool.h>
#include "ssd1306.h"

typedef struct {
    ssd1306_t *oled;
    bool last_scl;
    bool last_sda;
    bool active;
    bool expect_ack;
    bool address_matched;
    bool data_mode;
    bool continuation;
    bool expect_control;
    uint8_t bit_count;
    uint8_t shift;
    uint8_t byte_count;
    
    bool data_written;
} i2c_bb_t;

bool i2c_bb_sda_forced_low(i2c_bb_t *i);
void i2c_bb_init(i2c_bb_t *i2c, ssd1306_t *oled);
void i2c_bb_reset(i2c_bb_t *i2c);
void i2c_bb_process(i2c_bb_t *i2c, bool sda, bool scl);

#endif
