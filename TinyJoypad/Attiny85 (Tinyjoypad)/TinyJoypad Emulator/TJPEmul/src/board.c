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

#include "board.h"
#include "avr_ioport.h"
#include "avr_adc.h"
#include <stdio.h>
#include <string.h>


void board_init(board_t *b, avr_t *avr)
{
    memset(b, 0, sizeof(*b));
    b->avr = avr;

    ssd1306_init(&b->oled);
    i2c_bb_init(&b->i2c, &b->oled);

    b->adc_lr = 1023;
    b->adc_ud = 1023;

    b->spk_level = 0;
    b->spk_last_cycle = 0;
    b->spk_acc = 0.0;
}

void board_reset(board_t *b)
{
    ssd1306_reset(&b->oled);
    i2c_bb_reset(&b->i2c);
    b->adc_lr = 1023;
    b->adc_ud = 1023;
}

void board_update_inputs(board_t *b)
{
    /* ADC Left/Right (canal 0) */
    if (b->btn_left)       b->adc_lr = 800;
    else if (b->btn_right) b->adc_lr = 600;
    else                   b->adc_lr = 1023;

    /* ADC Up/Down (canal 3) */
    if (b->btn_down)       b->adc_ud = 800;
    else if (b->btn_up)    b->adc_ud = 600;
    else                   b->adc_ud = 1023;

    /* On pousse les valeurs ADC dans simavr */
    avr_irq_t *adc_base = avr_io_getirq(b->avr, AVR_IOCTL_ADC_GETIRQ, 0);
    if (adc_base) {
        /* simavr attend des millivolts (0-5000 typiquement) */
        avr_raise_irq(adc_base + ADC_CH_LEFTRIGHT, (b->adc_lr * 3300) / 1023);
        avr_raise_irq(adc_base + ADC_CH_UPDOWN,    (b->adc_ud * 3300) / 1023);
    }
}

static bool read_pin_level(board_t *b, int pin)
{
    uint8_t ddr  = b->avr->data[0x37]; // DDRB
    uint8_t port = b->avr->data[0x38]; // PORTB

    // Pin configuré en sortie → on renvoie PORT
    if (ddr & (1 << pin)) {
        return (port >> pin) & 1;
    }

    // === Pin en entrée ===

    // Bouton Action (PB1) - pull-up, actif bas
    if (pin == PIN_ACTION) {
        return !b->btn_action;
    }

    // SDA (PB0) - open drain
    if (pin == PIN_SDA) {
        if (i2c_bb_sda_forced_low(&b->i2c))
            return false;   // le slave tire vers le bas (ACK)
        return true;        // pull-up
    }

    // Tous les autres pins en entrée = pull-up
    return true;
}

void board_run_cycles(board_t *b, uint32_t cycles)
{
    for (uint32_t i = 0; i < cycles; i++) {
        avr_run(b->avr);

        /* I2C : OBLIGATOIRE à chaque cycle (détection des fronts) */
        bool sda = read_pin_level(b, PIN_SDA);
        bool scl = read_pin_level(b, PIN_SCL);
        i2c_bb_process(&b->i2c, sda, scl);

        /* Speaker + PINB : maintenant à chaque cycle */
        uint8_t ddr  = b->avr->data[0x37]; // DDRB
        uint8_t port = b->avr->data[0x38]; // PORTB

        b->spk_level = ((ddr & (1 << PIN_SOUND)) && (port & (1 << PIN_SOUND))) ? 1 : 0;
		b->spk_acc += b->spk_level;
        uint8_t pinb = 0;
        for (int p = 0; p < 6; p++) {
            if (read_pin_level(b, p))
                pinb |= (1 << p);
        }
        b->avr->data[0x36] = pinb;   // PINB
    }
}   
