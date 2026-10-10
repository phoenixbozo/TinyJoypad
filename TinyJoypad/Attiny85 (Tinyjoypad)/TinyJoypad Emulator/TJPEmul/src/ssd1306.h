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
 
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES  8

typedef struct {
    uint8_t gddram[SSD1306_PAGES][SSD1306_WIDTH];

    /* pointeurs d'écriture */
    uint8_t column;
    uint8_t page;
    uint8_t col_start, col_end;
    uint8_t page_start, page_end;
    uint8_t addressing_mode;   /* 0=horizontal, 1=vertical, 2=page */

    /* état d'affichage */
    bool    display_on;
    bool    invert;
    uint8_t contrast;

    /* ← LES 4 REGISTRES MANQUANTS QUI CAUSENT LE ROLLING */
    uint8_t start_line;        /* 0x40-0x7F → 0..63 */
    uint8_t display_offset;    /* commande 0xD3 */
    bool    segment_remap;     /* 0xA0 = false, 0xA1 = true */
    bool    com_scan_dir;      /* 0xC0 = false (normal), 0xC8 = true (remapped) */

bool     frame_ready;
uint8_t  shadow[8][128];   // copie stable

} ssd1306_t;

void ssd1306_init(ssd1306_t *d);
void ssd1306_reset(ssd1306_t *d);
void ssd1306_write_command(ssd1306_t *d, uint8_t cmd);
void ssd1306_write_data(ssd1306_t *d, uint8_t data);

/* Génère le framebuffer final 128×64 en tenant compte de start_line + offset + remap */
void ssd1306_get_framebuffer(ssd1306_t *d, uint8_t *out); /* out[y*128 + x] = 0 ou 1 */

#endif
