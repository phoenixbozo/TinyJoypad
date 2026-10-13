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

#include "ssd1306.h"
#include <string.h>

void ssd1306_init(ssd1306_t *d)
{
    ssd1306_reset(d);
}

void ssd1306_reset(ssd1306_t *d)
{
    memset(d->gddram, 0, sizeof(d->gddram));
    memset(d->shadow, 0, sizeof(d->shadow));

    d->column = 0;
    d->page = 0;
    d->col_start = 0;
    d->col_end = 127;
    d->page_start = 0;
    d->page_end = 7;
    d->addressing_mode = 0;

    d->display_on = true;
    d->invert = false;
    d->contrast = 0xCF;

    d->start_line = 0;
    d->display_offset = 0;

    d->segment_remap = true;    // 0xA1
    d->com_scan_dir  = true;    // 0xC8

    d->frame_ready = false;
}

static void advance_pointer(ssd1306_t *d)
{
    /* Mode Horizontal (celui de FastTinyDriver) */
    if (d->addressing_mode == 0) {
        d->column++;
        if (d->column > d->col_end) {
            d->column = d->col_start;
            d->page++;
            if (d->page > d->page_end)
                d->page = d->page_start;
        }
    }
    /* Vertical */
    else if (d->addressing_mode == 1) {
        d->page++;
        if (d->page > d->page_end) {
            d->page = d->page_start;
            d->column++;
            if (d->column > d->col_end)
                d->column = d->col_start;
        }
    }
    /* Page Addressing Mode */
    else {
        d->column++;
        if (d->column > d->col_end)
            d->column = d->col_start;
        /* page ne bouge jamais */
    }

    /* Sécurité finale */
    if (d->column > 127) d->column = 0;
    if (d->page   > 7)   d->page   = 0;
}

void ssd1306_write_command(ssd1306_t *d, uint8_t cmd)
{
    static uint8_t pending = 0;
    static int state = 0;

    if (state > 0) {
        if (pending == 0x20) {               // Memory Addressing Mode
            d->addressing_mode = cmd & 0x03;
            state = 0;
        }
        else if (pending == 0x21) {          // Column Address
            if (state == 1) {
                d->col_start = cmd & 0x7F;
                state = 2;
            } else {
                d->col_end = cmd & 0x7F;
                d->column = d->col_start;    // ← important
                state = 0;
            }
        }
        else if (pending == 0x22) {          // Page Address
            if (state == 1) {
                d->page_start = cmd & 0x07;
                state = 2;
            } else {
                d->page_end = cmd & 0x07;
                d->page = d->page_start;     // ← important
                state = 0;
            }
        }
        else if (pending == 0x81) {
            d->contrast = cmd;
            state = 0;
        }
        else if (pending == 0xD3) {
            d->display_offset = cmd & 0x3F;
            state = 0;
        }
        else {
            state = 0; // ignore les autres paramètres
        }
        return;
    }

    // Commandes à 1 octet
  else if (cmd >= 0xB0 && cmd <= 0xB7) {
    d->page = cmd & 0x07;
    /* Important : on ne touche PAS à column ici (le 0x00/0x10 qui suit le fera) */
}
else if (cmd <= 0x0F) {
    d->column = (d->column & 0xF0) | (cmd & 0x0F);
}
else if (cmd >= 0x10 && cmd <= 0x1F) {
    d->column = (d->column & 0x0F) | ((cmd & 0x0F) << 4);
}
    else if (cmd == 0xA0) d->segment_remap = false;
    else if (cmd == 0xA1) d->segment_remap = true;
    else if (cmd == 0xC0) d->com_scan_dir = false;
    else if (cmd == 0xC8) d->com_scan_dir = true;
    else if (cmd == 0xA6) d->invert = false;
    else if (cmd == 0xA7) d->invert = true;
    else if (cmd == 0xAE) d->display_on = false;
    else if (cmd == 0xAF) d->display_on = true;
    else if (cmd == 0x20 || cmd == 0x21 || cmd == 0x22 ||
             cmd == 0x81 || cmd == 0xD3 || cmd == 0xA8 ||
             cmd == 0xD5 || cmd == 0xD9 || cmd == 0xDA ||
             cmd == 0xDB || cmd == 0x8D) {
        pending = cmd;
        state = 1;
    }



}

void ssd1306_write_data(ssd1306_t *d, uint8_t data)
{
    /* Clamp ABSOLU – le hardware ne sort jamais de la fenêtre */
    if (d->column < d->col_start || d->column > d->col_end)
        d->column = d->col_start;
    if (d->page   < d->page_start || d->page   > d->page_end)
        d->page   = d->page_start;

    if (d->page < 8 && d->column < 128)
        d->gddram[d->page][d->column] = data;

    advance_pointer(d);
}

void ssd1306_get_framebuffer(ssd1306_t *d, uint8_t *out)
{
    for (int y = 0; y < 64; y++) {
        int src_y = (y + d->display_offset + d->start_line) & 63;

        if (!d->com_scan_dir)
            src_y = 63 - src_y;

        int page = src_y >> 3;
        int bit  = src_y & 7;

        for (int x = 0; x < 128; x++) {
            int src_x = x;

            if (!d->segment_remap)
                src_x = 127 - x;

            uint8_t byte = d->gddram[page][src_x];
            int on = (byte >> bit) & 1;

            if (d->invert) on = !on;
            if (!d->display_on) on = 0;

            out[y * 128 + x] = on ? 1 : 0;
        }
    }
}
