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
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <sys/stat.h>

#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_gdb.h"
#include "avr_ioport.h"
#include "avr_adc.h"

#include "board.h"

#define SCALE           4
#define WIN_W           (128 * SCALE)
#define WIN_H           (64  * SCALE)

#define SAMPLE_RATE     44100
#define AUDIO_SAMPLES   512

static board_t          board;
static SDL_Window      *window   = NULL;
static SDL_Renderer    *renderer = NULL;
static SDL_Texture     *texture  = NULL;
static bool             running  = true;
static bool             paused   = false;
static bool             fullscreen = false;
static char             current_rom[512] = {0};

static Uint32 last_click_time = 0;
static int    last_click_x = -1;
static int    last_click_y = -1;

static time_t rom_mtime = 0;
static Uint32 last_rom_check = 0;

static volatile bool force_quit = false;

static SDL_Joystick *joystick = NULL;

static bool key_left = false;
static bool key_right = false;
static bool key_up = false;
static bool key_down = false;
static bool key_action = false;

static bool joy_left = false;
static bool joy_right = false;
static bool joy_up = false;
static bool joy_down = false;
static bool joy_action = false;

static const int joystick_deadzone = 8000;


static int load_rom(const char *path);

static void request_quit(void)
{
    force_quit = true;
    running = false;
    paused = true;
    SDL_PauseAudio(1);
}

static time_t get_file_mtime(const char *path)
{
    struct stat st;

    if (!path || !path[0] || stat(path, &st) != 0) {
        return 0;
    }

    return st.st_mtime;
}

static void check_rom_update(void)
{
    Uint32 now = SDL_GetTicks();

    /* Vérifie le fichier au maximum une fois par seconde. */
    if (now - last_rom_check < 1000) {
        return;
    }
    last_rom_check = now;

    if (!current_rom[0]) {
        return;
    }

    time_t new_mtime = get_file_mtime(current_rom);

    if (new_mtime == 0 || rom_mtime == 0 || new_mtime == rom_mtime) {
        return;
    }

    printf("ROM modifiée, rechargement : %s\n", current_rom);

    if (load_rom(current_rom)) {
        rom_mtime = new_mtime;
    }
}


static int load_rom(const char *path)
{
    if (!path || !path[0]) {
        return 0;
    }

    SDL_PauseAudio(1);

    if (board.avr) {
        avr_terminate(board.avr);
        board.avr = NULL;
    }

    avr_t *avr = avr_make_mcu_by_name("attiny85");
    if (!avr) {
        fprintf(stderr, "Impossible de créer le MCU attiny85\n");
        SDL_PauseAudio(0);
        return 0;
    }

    avr_init(avr);
    avr->frequency = 16000000;

    elf_firmware_t f;
    memset(&f, 0, sizeof(f));

    if (strstr(path, ".elf") || strstr(path, ".axf")) {
        if (elf_read_firmware(path, &f) != 0) {
            fprintf(stderr, "Erreur lecture ELF : %s\n", path);
            avr_terminate(avr);
            SDL_PauseAudio(0);
            return 0;
        }
    } else {
        ihex_chunk_p chunks = NULL;
        int cnt = read_ihex_chunks(path, &chunks);

        if (cnt <= 0) {
            fprintf(stderr, "Erreur lecture HEX : %s\n", path);
            avr_terminate(avr);
            SDL_PauseAudio(0);
            return 0;
        }

        f.flashsize = 8192;
        f.flash = malloc(f.flashsize);

        if (!f.flash) {
            free_ihex_chunks(chunks);
            avr_terminate(avr);
            SDL_PauseAudio(0);
            return 0;
        }

        memset(f.flash, 0xFF, f.flashsize);

        for (int i = 0; i < cnt; i++) {
            if (chunks[i].baseaddr + chunks[i].size <= f.flashsize) {
                memcpy(
                    f.flash + chunks[i].baseaddr,
                    chunks[i].data,
                    chunks[i].size
                );
            }
        }

        free_ihex_chunks(chunks);

        strcpy(f.mmcu, "attiny85");
        f.frequency = 16000000;
    }

    avr_load_firmware(avr, &f);
    board_init(&board, avr);

    strncpy(current_rom, path, sizeof(current_rom) - 1);
    current_rom[sizeof(current_rom) - 1] = '\0';
    rom_mtime = get_file_mtime(current_rom);

    printf("ROM chargée : %s\n", current_rom);

    SDL_PauseAudio(0);
    return 1;
}

/* Réglages audio */
#define AUDIO_VOLUME       8000.0f  // Volume de sortie

#define HP_ALPHA           0.840f   // Passe-haut : plus proche de 1 = moins de graves coupés
#define LP_ALPHA           0.500f   // Premier passe-bas : plus petit = aigus davantage atténués
#define LP2_ALPHA          0.250f   // Second passe-bas : plus petit = harmoniques davantage atténuées

#define DC_ALPHA           0.0015f  // Vitesse de correction du décalage DC

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    board_t *b = (board_t *)userdata;
    Sint16 *out = (Sint16 *)stream;
    int samples = len / sizeof(Sint16);

    /* Si on veut quitter, on sort immédiatement sans toucher au MCU */
    if (force_quit || !running) {
        memset(stream, 0, len);   /* silence */
        return;
    }

    static uint32_t cycle_remainder = 0;
    static float dc = 0.0f;
    static float lp = 0.0f;
    static float lp2 = 0.0f;
    static float hp_prev_x = 0.0f;
    static float hp_prev_y = 0.0f;

    for (int i = 0; i < samples; i++) {
        cycle_remainder += 16000000;
        uint32_t cycles = cycle_remainder / SAMPLE_RATE;
        cycle_remainder %= SAMPLE_RATE;

        if (!paused) {
            board_run_cycles(b, cycles);
        }

        float sample = b->spk_level ? 0.18f : -0.18f;

        /* Retire la composante continue */
        dc += DC_ALPHA * (sample - dc);
        sample -= dc;

        /* Passe-haut */
        float hp = HP_ALPHA * (hp_prev_y + sample - hp_prev_x);
        hp_prev_x = sample;
        hp_prev_y = hp;

        /* Deux passe-bas */
        lp += LP_ALPHA * (hp - lp);
        lp2 += LP2_ALPHA * (lp - lp2);

        out[i] = (Sint16)(lp2 * AUDIO_VOLUME);
    }
}

static void draw_oled(void)
{
    static Uint32 last_time = 0;
    static int frame_count = 0;
    static char title[128];
    static bool has_valid_frame = false;

    uint8_t fb[128 * 64];
    bool do_update = false;

    if (board.oled.frame_ready) {
        // Utilise le shadow (déjà stable)
        ssd1306_t tmp = board.oled;
        memcpy(tmp.gddram, board.oled.shadow, sizeof(tmp.gddram));
        ssd1306_get_framebuffer(&tmp, fb);

        board.oled.frame_ready = false;
        do_update = true;
        has_valid_frame = true;
    }

    if (do_update) {
        void *pixels;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &pixels, &pitch) == 0) {
            uint32_t *dst = (uint32_t *)pixels;
            for (int y = 0; y < 64; y++) {
                for (int x = 0; x < 128; x++) {
                    dst[y * (pitch / 4) + x] = fb[y * 128 + x] ? 0xFFE8E8E8 : 0xFF000000;
                }
            }
            SDL_UnlockTexture(texture);
        }
    }

    // === Toujours afficher quelque chose ===
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);   // noir
    SDL_RenderClear(renderer);

    if (has_valid_frame) {
        SDL_RenderCopy(renderer, texture, NULL, NULL);
    }

    SDL_RenderPresent(renderer);

    // Compteur FPS
    frame_count++;
    Uint32 now = SDL_GetTicks();
    if (now - last_time >= 1000) {
        snprintf(title, sizeof(title), "TinyJoypad Emulator - ATtiny85  |  %d FPS", frame_count);
        SDL_SetWindowTitle(window, title);
        frame_count = 0;
        last_time = now;
    }
}

static void toggle_fullscreen(void)
{
    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    printf(fullscreen ? "→ Fullscreen\n" : "→ Fenêtré\n");
}

static bool is_hex_file(const char *path)
{
    if (!path) {
        return false;
    }

    const char *ext = strrchr(path, '.');
    return ext && SDL_strcasecmp(ext, ".hex") == 0;
}


static void open_joystick(void)
{
    if (SDL_NumJoysticks() <= 0) {
        printf("Aucun joystick détecté.\n");
        return;
    }

    joystick = SDL_JoystickOpen(0);

    if (!joystick) {
        fprintf(stderr, "Impossible d'ouvrir le joystick : %s\n",
                SDL_GetError());
        return;
    }

    printf("Joystick connecté : %s\n", SDL_JoystickName(joystick));
}

static void update_combined_inputs(void)
{
    SDL_LockAudio();

    board.btn_left   = key_left   || joy_left;
    board.btn_right  = key_right  || joy_right;
    board.btn_up     = key_up     || joy_up;
    board.btn_down   = key_down   || joy_down;
    board.btn_action = key_action || joy_action;

    board_update_inputs(&board);

    SDL_UnlockAudio();
}

static void handle_joystick_event(const SDL_Event *e)
{
    if (!joystick) {
        return;
    }

    if (e->type == SDL_JOYAXISMOTION) {
        if (e->jaxis.axis == 0) {
            joy_left  = e->jaxis.value < -joystick_deadzone;
            joy_right = e->jaxis.value >  joystick_deadzone;
        } else if (e->jaxis.axis == 1) {
            joy_up   = e->jaxis.value < -joystick_deadzone;
            joy_down = e->jaxis.value >  joystick_deadzone;
        }
    } else if (e->type == SDL_JOYHATMOTION) {
        joy_left  = (e->jhat.value & SDL_HAT_LEFT)  != 0;
        joy_right = (e->jhat.value & SDL_HAT_RIGHT) != 0;
        joy_up    = (e->jhat.value & SDL_HAT_UP)    != 0;
        joy_down  = (e->jhat.value & SDL_HAT_DOWN)  != 0;
    } else if (e->type == SDL_JOYBUTTONDOWN ||
               e->type == SDL_JOYBUTTONUP) {
        /* Bouton 0 du joystick = bouton A */
        if (e->jbutton.button == 0) {
            joy_action = (e->type == SDL_JOYBUTTONDOWN);
        }
    }

    update_combined_inputs();
}


static void handle_input(void)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            request_quit();
        }
        else if (e.type == SDL_DROPFILE) {
            if (is_hex_file(e.drop.file)) {
                if (!load_rom(e.drop.file)) {
                    fprintf(stderr, "Impossible de charger le fichier : %s\n",
                            e.drop.file);
                }
            } else {
                fprintf(stderr,
                        "Fichier ignoré (seuls les .hex sont acceptés) : %s\n",
                        e.drop.file);
            }

            SDL_free(e.drop.file);
        }
        else if (e.type == SDL_JOYAXISMOTION ||
                 e.type == SDL_JOYHATMOTION ||
                 e.type == SDL_JOYBUTTONDOWN ||
                 e.type == SDL_JOYBUTTONUP) {
            handle_joystick_event(&e);
        }
        else if (e.type == SDL_JOYDEVICEREMOVED) {
            if (joystick &&
                SDL_JoystickInstanceID(joystick) == e.jdevice.which) {
                SDL_JoystickClose(joystick);
                joystick = NULL;

                joy_left = false;
                joy_right = false;
                joy_up = false;
                joy_down = false;
                joy_action = false;

                update_combined_inputs();
                printf("Joystick déconnecté.\n");
            }
        }
        else if (e.type == SDL_JOYDEVICEADDED) {
            if (!joystick) {
                open_joystick();
            }
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN &&
                 e.button.button == SDL_BUTTON_LEFT) {
            Uint32 now = SDL_GetTicks();
            int dx = e.button.x - last_click_x;
            int dy = e.button.y - last_click_y;

            if (now - last_click_time < 350 &&
                abs(dx) < 10 && abs(dy) < 10) {
                toggle_fullscreen();
                last_click_time = 0;
            } else {
                last_click_time = now;
                last_click_x = e.button.x;
                last_click_y = e.button.y;
            }
        }
        else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool pressed = (e.type == SDL_KEYDOWN);

            if (e.type == SDL_KEYDOWN && e.key.repeat) {
                continue;
            }

            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (pressed) {
                        if (fullscreen) {
                            toggle_fullscreen();
                        } else {
                            request_quit();
                        }
                    }
                    break;

                case SDLK_p:
                    if (pressed) {
                        paused = !paused;
                        printf(paused ? "Pause\n" : "Reprise\n");
                    }
                    break;

                case SDLK_r:
                    if (pressed && current_rom[0]) {
                        if (load_rom(current_rom)) {
                            printf("Reset\n");
                        }
                    }
                    break;

                case SDLK_f:
                    if (pressed) {
                        toggle_fullscreen();
                    }
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    key_left = pressed;
                    update_combined_inputs();
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    key_right = pressed;
                    update_combined_inputs();
                    break;

                case SDLK_UP:
                case SDLK_w:
                    key_up = pressed;
                    update_combined_inputs();
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    key_down = pressed;
                    update_combined_inputs();
                    break;

                case SDLK_SPACE:
                case SDLK_z:
                case SDLK_x:
                case SDLK_RETURN:
                    key_action = pressed;
                    update_combined_inputs();
                    break;

                default:
                    break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    const char *rom_path = (argc >= 2) ? argv[1] : NULL;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL_Init failed : %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_JoystickEventState(SDL_ENABLE);
	open_joystick();

    window = SDL_CreateWindow(
        "TinyJoypad Emulator - ATtiny85",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed : %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
}

SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

// Force OpenGL + VSync
SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

renderer = SDL_CreateRenderer(window, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed : %s\n", SDL_GetError());
    return 1;
}

if (SDL_GL_SetSwapInterval(1) == 0) {
    printf("→ VSync forcé avec succès (OpenGL)\n");
} else {
    printf("→ Échec VSync : %s\n", SDL_GetError());
}

    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        128, 64);

    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture failed : %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    SDL_AudioSpec desired, obtained;
    memset(&desired, 0, sizeof(desired));
    desired.freq     = SAMPLE_RATE;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples  = AUDIO_SAMPLES;
    desired.callback = audio_callback;
    desired.userdata = &board;



    if (SDL_OpenAudio(&desired, &obtained) < 0) {
        fprintf(stderr, "Impossible d'ouvrir l'audio : %s\n", SDL_GetError());
    } else {
        SDL_PauseAudio(0);
        printf("Audio initialisé : %d Hz\n", obtained.freq);
    }

    if (rom_path) {
        if (!load_rom(rom_path)) {
            avr_t *avr = avr_make_mcu_by_name("attiny85");
            if (avr) {
                avr_init(avr);
                avr->frequency = 16000000;
                board_init(&board, avr);
            }
        }
    } else {
        avr_t *avr = avr_make_mcu_by_name("attiny85");
        if (avr) {
            avr_init(avr);
            avr->frequency = 16000000;
            board_init(&board, avr);
        }
        printf("Aucun ROM chargé.\n");
        printf("→ Lancez avec : ./tinyjoypad_emu monjeu.hex\n");
    }

    printf("\n=== TinyJoypad Emulator (SDL2 + VSync) ===\n");
    printf("Contrôles :\n");
    printf("  Flèches / WASD     = Direction\n");
    printf("  Espace / Z / X     = Bouton A\n");
    printf("  P                  = Pause\n");
    printf("  R                  = Reset\n");
    printf("  F ou Double-clic   = Fullscreen\n");
    printf("  Esc                = Quitter / Sortir du fullscreen\n\n");

 while (running) {
    handle_input();
    check_rom_update();
    draw_oled();
}

       /* ========== NETTOYAGE ULTRA ROBUSTE ========== */
    force_quit = true;
    running = false;
    paused = true;

    SDL_PauseAudio(1);
    SDL_Delay(20);               /* laisse le callback s’arrêter */

    SDL_CloseAudio();
    SDL_Delay(30);

    /* On force la destruction de l’audio subsystem */
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    if (board.avr) {
        avr_terminate(board.avr);
        board.avr = NULL;
    }

    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

if (joystick) {
    SDL_JoystickClose(joystick);
    joystick = NULL;
}

    SDL_Quit();

    /* Délai final pour que l’OS libère vraiment le device audio + GL */
    SDL_Delay(120);

    return 0;
}
