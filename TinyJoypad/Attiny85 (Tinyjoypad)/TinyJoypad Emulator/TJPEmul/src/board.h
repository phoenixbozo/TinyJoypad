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

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "sim_avr.h"
#include "ssd1306.h"
#include "i2c_bb.h"

#define PIN_SDA        0
#define PIN_ACTION     1
#define PIN_SCL        2
#define PIN_UPDOWN     3
#define PIN_SOUND      4
#define PIN_LEFTRIGHT  5

#define ADC_CH_LEFTRIGHT  0
#define ADC_CH_UPDOWN     3

typedef struct {
    avr_t *avr;
    ssd1306_t oled;
    i2c_bb_t i2c;

    bool btn_left;
    bool btn_right;
    bool btn_up;
    bool btn_down;
    bool btn_action;

    uint16_t adc_lr;
    uint16_t adc_ud;

    /* === SON === */
    int      spk_level;        // état actuel de PB4 (0 ou 1)
    uint64_t spk_last_cycle;
    double   spk_acc;          // accumulateur de cycles à l'état haut
} board_t;

void board_init(board_t *b, avr_t *avr);
void board_reset(board_t *b);
void board_update_inputs(board_t *b);
void board_run_cycles(board_t *b, uint32_t cycles);

#endif
