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
 
#include "i2c_bb.h"
#include <string.h>

void i2c_bb_init(i2c_bb_t *i, ssd1306_t *oled)
{
    memset(i, 0, sizeof(*i));
    i->oled = oled;
    i->last_scl = true;
    i->last_sda = true;
    i->expect_control = false;
    i->data_written = false;
}

void i2c_bb_reset(i2c_bb_t *i)
{
    ssd1306_t *o = i->oled;
    memset(i, 0, sizeof(*i));
    i->oled = o;
    i->last_scl = true;
    i->last_sda = true;
    i->expect_control = false;
    i->data_written = false;
}

bool i2c_bb_sda_forced_low(i2c_bb_t *i)
{
    return i->expect_ack;
}

void i2c_bb_process(i2c_bb_t *i, bool sda, bool scl)
{
    // START
    if (i->last_scl && scl && i->last_sda && !sda) {
        i->active = true;
        i->bit_count = 0;
        i->shift = 0;
        i->byte_count = 0;
        i->data_mode = false;
        i->expect_ack = false;
        i->address_matched = false;
        i->continuation = false;
        i->expect_control = true;
        i->data_written = false;
    }
    // STOP
    else if (i->last_scl && scl && !i->last_sda && sda) {
        i->active = false;
        i->expect_ack = false;

        if (i->data_written) {
            memcpy(i->oled->shadow, i->oled->gddram, sizeof(i->oled->gddram));
            i->oled->frame_ready = true;
            i->data_written = false;
        }
    }
    // Front montant SCL
    else if (i->active && !i->last_scl && scl) {
        if (!i->expect_ack) {
            i->shift = (i->shift << 1) | (sda ? 1 : 0);
            i->bit_count++;
        }
    }
    // Front descendant SCL
    else if (i->active && i->last_scl && !scl) {
        if (i->bit_count == 8 && !i->expect_ack) {
            uint8_t val = i->shift;

            if (i->byte_count == 0) {
                uint8_t addr = val >> 1;
                i->address_matched = (addr == 0x3C || addr == 0x3D);
                i->expect_control = true;
            }
            else if (i->expect_control) {
                i->continuation = (val & 0x80) != 0;
                i->data_mode    = (val & 0x40) != 0;
                i->expect_control = false;
            }
            else if (i->address_matched) {
                if (i->data_mode) {
                    ssd1306_write_data(i->oled, val);
                    i->data_written = true;
                } else {
                    ssd1306_write_command(i->oled, val);
                }

                if (i->continuation)
                    i->expect_control = true;
            }

            i->byte_count++;
            i->expect_ack = true;
            i->bit_count = 0;
            i->shift = 0;
        }
        else if (i->expect_ack) {
            i->expect_ack = false;
        }
    }

    i->last_scl = scl;
    i->last_sda = sda;
}
