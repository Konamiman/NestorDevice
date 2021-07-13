# USB keyboard functionality

The software in this repository allows you to use your MSX+Rookie Drive combo as an USB keyboard for a PC. This document is a complete guide for how to do that.


## How does it work?

A small program on the MSX will configure the CH376 on the Rookie Drive as a device and then will enter in a loop in which it will scan the entire keyboard and send any changes detected (keys presses and releases) to the interrupt endpoint provided by the CH376. The raw keyboard port status is sent verbatim, thus the program does not depend on the MSX keyboard layout.

Meanwhile, a server program on the PC will listen to this endpoint and as soon as it receives data it will translate the key presses and releases into HID key codes, which will then be sent to a virtual HID driver. A key mappings file is used to translate MSX keys to HID keys.


## What do I need?

* An MSX computer (any model will do) and a Rookie Drive cartridge.
* An USB cable **with the power connector cut or blocked**. Please read [the main README file](https://github.com/Konamiman/NestorDevice/blob/main/README.md) for details on this.
* A PC running a 64-bit edition of Windows 7, 8 or 10. Please read [the main README file](https://github.com/Konamiman/NestorDevice/blob/main/README.md) for details on why this is the case and why I haven't implemented a HID compliant device on the MSX side.


## The initial setup

Follow these steps to setup your computers for the fun, all the required software except the virtual HID driver is available in [the releases page of this repository](https://github.com/Konamiman/NestorDevice/releases):

1. Prepare the hardware: plug your Rookie Drive on your MSX and the cable on both the Rookie Drive and the PC.

2. Download the software for the MSX. You have two options: the `RDDEV.ROM` file and the `RDDEV.BIN` file. 

   * The ROM is to be flashed in the Rookie Drive and is the most convenient option if you plan to use your MSX computer exclusively as an USB keyboard, so once you have it flashed you can just turn on your MSX and you don't even need to plug a monitor to it. On the other hand the BIN is better if you don't want to replace the original ROM of your Rookie Drive, you can load it from BASIC with just `BLOAD"RDDEV.BIN",R`.

3. Switch on your MSX, if you are using the BIN file load it. The PC should detect the Rookie Drive as an unknown device (you will see it in Device Manager under "Other devices" as "Unknown device")

4. Install the WinUSB driver for Rookie Driver. Options for doing that:

   1. Download `RDDevDriver.zip`, unzip it somewhere, right click `Unknown_Device_#1.inf` (yes, the files must have these names; don't ask me, ask Microsoft) and select "Install".
   2. Use a dedicated tool like [Zadig](https://zadig.akeo.ie/). This is the tool used to generate the .inf and .cat files of step 1, by the way.

   Now the device should appear as "Unknown device #1" in device manager, but this time under the "Universal Serial Bus Devices" section and without an exclamation mark.

5. Install [The virtual HID driver from Tetherscript](https://tetherscript.com/hid-driver-kit-download/) in your PC.

   _Note_: The driver license allows free usage for open source projects, but forbids hosting the driver installer itself outside the official driver site.

   The virtual keyboard device installed will appear as "Tetherscript Virtual Keyboard" under "Human Interface Devices" in device manager.

6. Download the server program, `RDServer.zip`. Unzip it somewhere.

   _Note_: There are two variants of the server program: "Standalone" and "Framework Dependant". The later is smaller in size but requires [.NET Core 3.1](https://dotnet.microsoft.com/download/dotnet/3.1) to be installed separately. The program will be referred as `RDServer.exe` in this guide.

7. The previous step will have generated the program files and two text files with keyboard mappings. The server program needs a keyboard mappings files to translate MSX keys to HID keys.

   Two mappings keys files are provided, [one for the Canon V-20 with UK keyboard](https://github.com/Konamiman/NestorDevice/blob/main/dotNet/RookieDriveDeviceServer/Server/KeyMappings_Canon_V20_UK.txt), and [one for the Panasonic FS-A1ST](https://github.com/Konamiman/NestorDevice/blob/main/dotNet/RookieDriveDeviceServer/Server/KeyMappings_TurboR.txt) (the two MSX computers I had at hand). You can use these as is, tweak them to your taste, or use them as the basis to create a completely different file. See [the guide on how to create a mappings file](https://github.com/Konamiman/NestorDevice/blob/main/docs/MappingFiles.md) for details.


## Putting it to work

Now, with your MSX turned on and either the ROM flahsed on the Rookie Drive or `RDDEV.BIN` loaded, you can run `RDServer.exe`. It admits these arguments (all optional):

* `-i <vid>:<pid>`: Use this pair of vendor and product ids (hexadecimal numbers) to identify the device if you are using a modified version of the MSX program that doesn't use the default pair (which is 1209:0007).
* `-k <file>`: Use this to specify the path of the file to use for the key mappings, the default if none is supplied is `KeyMappings.txt` in same directory of the application executable.
* `-d`: Debug mode, print out all the data received from the device.
* `?`: Show help (the arguments list above).

If everything goes well the server should display the message "Device connected!" (if you forgot to turn or your MSX or to load the BIN file, do it now; there's no need to restart the server program) and... voila, your MSX is now an MSX keyboard!

To exit the server program, press CTRL+C or close the console window where it's running. To exit the MSX program, press `ESC` + `ENTER` + `STOP` on the MSX keyboard.


## Caveats

The virtual HID driver implements an English (US) keyboard. Thus, you must have that same keyboard layout selected in your PC for the MSX keyboard to work as configured in the mappings file; if you have a different keyboard layout selected you'll see that some keys (especially the ones for special characters) will have a different actual mapping.

Additionally, the MSX keyboard doesn't have an exact correspondence with a PC keyboard, and therefore some creativity is needed to map the MSX-only keys (`GRAPH`, `SELECT`, `CODE`...) and the PC-only keys (`End`, `PgUp/Down`, `Alt`...). See [the provided mapping files themselves](https://github.com/Konamiman/NestorDevice/tree/main/dotNet/RookieDriveDeviceServer/Server) for the specific mappings used in each case.


