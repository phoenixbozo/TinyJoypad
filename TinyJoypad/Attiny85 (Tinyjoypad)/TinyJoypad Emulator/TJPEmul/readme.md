# TinyJoypad Emulator - ATtiny85 + SSD1306

> A software emulator designed to run games originally created for the Tinyjoypad gaming console (ATtiny85 + SSD1306 OLED).

* **GitHub Repository:** [phoenixbozo/TinyJoypad](https://github.com/phoenixbozo/TinyJoypad)
* **Website:** [www.tinyjoypad.com](https://www.tinyjoypad.com)
* **Contact Email:** [electro_l.i.b@tinyjoypad.com](mailto:electro_l.i.b@tinyjoypad.com)

---

## Description

TinyJoypad Emulator is a software emulator designed to run games originally created for the Tinyjoypad gaming console (ATtiny85 + SSD1306 OLED).

It accurately simulates the ATtiny85 microcontroller and the SSD1306 display, allowing you to play and test Tinyjoypad games directly on your computer without the physical hardware.

The emulator is built with **SDL2** and **simavr**.

---

## Features

* Full ATtiny85 simulation (16 MHz)
* SSD1306 OLED screen emulation (128x64)
* Sound emulation
* Support for `.hex` files
* Drag & drop ROM loading
**Automatic ROM reload when the file is modified**
* Keyboard controls
* Fullscreen mode
* Pause / Reset

---

## Controls

| Key | Action |
| :--- | :--- |
| **Arrow keys / WASD** | Direction |
| **Space / Z / X / Enter** | Action button |
| **P** | Pause / Resume |
| **R** | Reset |
| **F or Double-click** | Toggle Fullscreen |
| **Escape** | Quit / Exit fullscreen |

---

## Dependencies

* [SDL2](https://www.libsdl.org/)
* [simavr](https://github.com/buserror/simavr)

---

## Compilation (Linux Debian/Ubuntu)

**BUILD INSTRUCTIONS (Linux)**

**Option 1:- Install dependencies automatically)**

Run the installation script:

./install-deps.sh

Then, from the directory containing the Makefile, run:

make


**Option 2:- (Install dependencies manually)**

Install SDL2:

sudo apt install libsdl2-dev

Make sure simavr is also installed and accessible.

Then, from the directory containing the Makefile, run:

make

---

## Running the Emulator

```bash
./tinyjoypad_emu monjeu.hex
```

> *Tip: You can also drag and drop a `.hex` file directly onto the window.*

---

## License

This code is distributed under the **GNU General Public License version 3 (GPLv3)**.  
See the [GNU GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.html) for more details.

---

## Credits

- **Électro L.I.B / TinyJoypad** – Concept, direction, and testing
- **Électro L.I.B / TinyJoypad** – Original TinyJoypad hardware and games
- **Grok (xAI)** – Code, written under the direction and with the assistance of Électro L.I.B
- **Michel Pollet (buserror) and the simavr contributors** – For creating and maintaining simavr, a core foundation of this emulator
- **SDL2 contributors** – For the SDL2 library used by this emulator
- **Willems Davy ([@joyrider3774](https://joyrider3774.github.io/))** – For identifying and helping fix critical race conditions in the audio thread under Windows