# Rookie Drive in device mode

[Rookie Drive](http://rookiedrive.com/en) is a USB host cartridge for MSX computers, powered by a CH376 USB host controller. It's designed and produced by Xavirompe.

The CH376 chip that powers the Rookie Drive is in principle a USB host controller, and both the Rookie Drive hardware and the official ROM are designed to use the cartridge as such. However the CH376 chip is compatible with the CH372, which is a device controller; thus, it is possible to use a Rookie Drive to turn your MSX into a USB device to be used by a PC. This repository contains some (mostly experimental) code that does exactly that.


## What's implemented?

I tried to use the CH376 in "device with external firmware" mode, in which I take full control on the default control endpoint; this would allow me to e.g. implement a fully HID compliantt device. However [weird technical problems](https://stackoverflow.com/questions/68066989/usb-device-prototype-timeouts-with-winusb-or-hid-drivers-works-fine-with-libusb) prevented me from doing so. Therfore I had to resort to the "device with internal firmware" mode of the chip, in which I have control of the interrupt/bulk endpoints only; the MSX is seen by the host as a USB device having the "vendor-specific" class and dedicated software is needed to communicate with it.

For now there's just one functionality implemented: USB keyboard. Details here: **[USB keyboard functionality](https://github.com/Konamiman/NestorDevice/blob/main/docs/Keyboard.md)**


## An important note about the cable

You need an USB A-A (or A-C if your computer has USB C ports) cable to connect the PC to the Rookie Drive. However you should not use a standard cable as is: first, you should cut or somehow block the power (5V) connector from the cable.

That's because Rookie Drive, being designed as an USB host controller, supplies power to the USB bus (needed by USB devices). But the PC, on the other hand, will also supply power to the bus. USB is not designed for both ends suppliying power to the bus at the same time, which may have undesirable results.

The easiest way to block the power from a cable is to simply put a small piece of duct tape on top of the 5V connector of the cable. Please look at the following images for reference (taken from [a blog post by Arjen Stens explaining the process](https://arjenstens.com/prevent-usb-powering-3d-printer-display/)):

![where to put the tape](https://github.com/Konamiman/NestorDevice/blob/main/docs/BlockingUSBPower1.jpg?raw=true)
![the cable with the tape in place](https://github.com/Konamiman/NestorDevice/blob/main/docs/BlockingUSBPower2.jpg?raw=true)

It's enough to block the connector in one of the sides of the cable, and it doesn't matter in which end (the MSX or the PC) you plug the blocked side of the cable.

There are also adapters for sale that will block the power of your cable in a more convenient way, just try "USB power blocker" in your favorite search engine if you want to explore this option.


## Device VID and PID

Each USB device must have an unique combination of vendor ID (VID) and product ID (PID), being each a 16 bit number. The software in this repository uses, provisionally, VID `0x1209` and PID `0x0007`. The VID is the one for [pid.codes](https://pid.codes/), and the PID is one reserved for testing. [A pull request is on its way](https://github.com/pidcodes/pidcodes.github.com/pull/661) for a dedicated PID.


## Last but not least...

...if you like this project **[please consider donating!](http://www.konamiman.com/msx/msx-e.html#donate)** My kids need moar shoes!
