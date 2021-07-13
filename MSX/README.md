# MSX keyboard data sender

This is a small program that will configure the CH376 of the Rookie Drive as "device in internal firmware" mode and then will enter in a loop in which it's going to read all the keyboard rows (using the BIOS routine `SNSMAT`). Whenever it detects a change (any key is pressed or released) it'll send the new keyboard row status to the interrupt endpoint that the CH376 provides. Two bytes are sent for each change detected: the row number first, then the row status. The maximum row number scanned is 11.

The program can be assembled using [Sjasm](https://github.com/Konamiman/Sjasm), or directly from a MSX using Compass.

The ROM version of the program uses RAM above C000h, thus it should work on any MSX having at least 16K of RAM.

There are a few interesting constants to consider when assembling the program:

* `ROM`: set to 1 to generate a ROM file, set to 0 to generate a BIN file.
* `DEBUG_LOG`: set to 1 to print all the data sent to the endpoint, as well as "wakeup" and "suspend" events as detected by the CH376.
* `VID_*` and `PID_*`: these control the VID and the PID that will be send to the USB host in the configuration descriptor.

Once the program is running it can be exited with the following keys combination: `ESC` + `ENTER` + `STOP`.

See [the guide to use Rookie Drive as a keyboard](https://github.com/Konamiman/NestorDevice/blob/main/docs/Keyboard.md) for more details.
