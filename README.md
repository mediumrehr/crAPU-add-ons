# crAPU-add-ons
a crappy Audio Processing Unit SAO for DEF CON 26 badges; specifically, the zinthesizer was designed for the [DCZia DC26 badge](https://github.com/dczia/Defcon26-Badge) and the scaremin was designed for [Chris Gammell's](https://twitter.com/Chris_Gammell) Goodfear badge.

[video demo](https://www.youtube.com/watch?v=xHMZxM6m_c0)

crAPUs were designed to add audio sythesis/playback capabilities to badges that didn't have any. Since i2c would not be fast enough to drive an external digital to analog converter (DAC), the crAPUs feature their own processor (SAM21G18) to generate tones, and the note and volume can be selected over i2c.

## Installing/Updating Firmware

### Setup

Firmware can be loaded with [Atmel Studio 7.0](https://www.microchip.com/mplab/avr-support/atmel-studio-7), or install the packages below to use with your preferred text editor and compile with arm gcc.

#### arm gcc and gdb

MacOS

```
brew install armmbed/formulae/arm-none-eabi-gcc
```

#### jlink gdb server

Download install file from [segger.com](https://www.segger.com/products/debug-probes/j-link/tools/j-link-gdb-server/about-j-link-gdb-server/)

#### SWD connection

Software debug (SWD) pins are labelled on the back-right of the boards. Connect SWDIO, SWCLK, 3V3, RESET, and GND as labelled. SWD requires the board to be powered external from the SWD port, so it's easiest to power the board from the 3V3 and GND pins on the SAO header.

### Build

```
cd firmware/crAPU/
make -C Build/
```

### Load

#### jlink

Start jlink gdb server in one tab

```
JLinkGDBServer -if SWD -device ATSAMD21G18
```

Start gdb in another tba

```
arm-none-eabi-gdb Build/crAPU.elf
(gdb) target extended-remote :2331
```

Load new firmware, reset microcontroller, and run new code

```
(gdb) load
(gdb) monitor reset
(gdb) c
```

## Usage

more coming soon...

#### DCZia Badge

Load my [DCZia fork](https://github.com/mediumrehr/Defcon26-Badge) onto the DCZia badge. Attach the zinthesizer add-on, turn on the badge, and within menu mode, select button 7 (left column, third down). The display should now say it's in zinthesizer mode and each key will play a unique note.
