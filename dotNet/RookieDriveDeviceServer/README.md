# Rookie Drive in device mode keyboard server

This program will:

1. Connect to [the virtual HID driver](https://tetherscript.com/hid-driver-kit-download/)
2. Check if a Rookie Drive in device mode is connected (by VID and PID)
3. Enter in a loop in which any MSX keyboard status change data received from the interrupt endpoint of the Rookie Drive is translated into HID key codes and sent to the HID driver

## Requirements

1. A computer running 64-bit Windows 7, 8 or 10
2. .NET Core 3.1 must be installed when running the "framework dependant" variant of the program
2. [The virtual HID driver](https://tetherscript.com/hid-driver-kit-download/) must be installed and the virtual keyboard device must be enabled
3. Rookie Drive running in device mode must be connected to the computer via a slightly modified USB cable (see warning below)
4. A proper keyboard mappings file must be supplied

**WARNING!** The USB cable used to connect the Rookie Drive to the PC must have the power (5V) connector cut or blocked. Otherwise both the MSX and the PC will be suppliying power to each other at the same time, and that's not a good idea. By having the power connector blocked the Rookie Drive will be MSX-powered as it's designed to be, and no power will go from one computer to the other.

## Command line arguments

* `-i <vid>:<pid>`: Use this pair of vendor and product ids (hexadecimal numbers) to identify the device, default is 1209:0007
* `-k <file>`: Use this file for the key mappings, default is `KeyMappings.txt` in same directory of the application executable
* `-d`: Debug mode, print out all the data received from the device
* `?`: Show help (the arguments list above)

All are optional.

## The mappings file

The keyboard mappings file is used by the program to translate the MSX keys to standard HID keys. It's a text file following these rules:

1. Empty lines and lines starting with `;` or `#` are considered comments and are ignored.
2. The file must contain at least one and at most 12 non-comment lines.
3. Each non-comment line must consist of eaxctly 8 words separated by spaces.
4. Each word must be either one HID key definition as defined in the `HidKeyCodes.cs` file, or two such definitions separated by a comma, `,`, or a point `.`.

Each line corresponds to one row of the MSX keyboard: the first non-comment line found is for row 0, the next one is for row 1, etc. Each word in the line corresponds to one bit in the keyboard status bytes, the first word is for the MSX and the last word is for the LSB. Use a point, `.`, to ignore a key (pressing it will have no effect).

There's one special HID key code named `MSX_MODIFIER`. This is a modifier key on the MSX side, and allows to assign two different HID keys to one single MSX key: if the mapping in the file is defined as `key1,key2` then the key is mapped to `key2` if the MSX modifier key is pressed at the same time, or to `key1` otherwise.

Example: if you map the MSX `Select` key to `MSX_MODIFIER` and the MSX `Home` key to `HOME,END`, then the `Home` key alone in the MSX is mapped to the `HOME` key in the PC; but the `Select+Home` combination in the MSX is mapped to the `END` key in the PC.

Please note that the virtual HID driver has some limitations:

1. It implements an English keyboard, if you use a different layout in Windows you'll get different keys than the ones mapped.
2. It doesn't support all the HID codes defined in `HidKeyCodes.cs`.

Two key mappings files are supplied: one for the Canon V-20 with UK keyboard, and another one for the Panasonic FS-A1 ST (the two computers I had at hand for testing). You can use these verbatim or you can modify them to suit the keyboard of your MSX model. (Note that the mappings are not perfect, especially the ones for the FS-A1 ST which has a Japanese keyboard).
