# The keyboard mapping files

When [using your MSX as an USB keyboard](https://github.com/Konamiman/NestorDevice/blob/main/docs/Keyboard.md) the program running in the MSX will send raw keyboard status data to the endpoint, but the server program running in the PC needs to translate this data to HID codes to be sent to the virtual HID driver. A _mappings file_ is used for this.

The MSX keyboard is organized in a matrix of up to twelve rows of eight keys each, with [the exact key to row and column mapping being different depending on the keyboard layout](https://www.msx.org/wiki/Keyboard_Matrices). The MSX program just reads all the rows in a loop and sends the raw data to the endpoint, wihout interpreting it (except for the exit sequence, `ESC`+`ENTER`+`STOP`, which works because these keys have the same mapping in all layouts); thus, one single program is used for all the MSX models.

A mappings file is a text file that contains one HID key code (sometimes two) for each position on each row of the MSX keyboard matrix, thus when the server program detects a key press sent by the MSX it searches the corresponding HID code and sends it to the virtual HID driver.

If you want to create a new mappings file or tweak one of the existing ones, these are the exact rules for the file:

1. A blank line or a line starting with `;` or `#` is a comment line and will be ignored.
2. The file must contain at least one and at most twelve non-coment lines.
3. Each line must contain exactly eight words, separated with spaces (extra spaces at each end of the line, or between words, are accepted).
4. Each word must be either:
   1. A HID key code as defined in [the HidKeyCodes.cs file](https://github.com/Konamiman/NestorDevice/blob/main/dotNet/RookieDriveDeviceServer/Server/HidKeyCodes.cs); case doesn't matter.
   2. Two such codes separated with a comma, `,`.
   3. A dot, `.`; this will leave the MSX key unmapped (the server program will ignore it).

There's a special value in `HidKeyCodes.cs` named `MSX_MODIFIER`. This is not a HID code, but a value that indicates that the corresponding key will act as a modifier on the MSX side. If a MSX key has two comma-separated values assigned in the mappings file, the second value will be used if the modifier key is pressed simultaneously, the first value will be used otherwise.

For example, the provided mappings files assigns the `MSX_MODIFIER` code to the MSX `SELECT` key, and the values `Up,PageUp` to the cursor up key. This means that the cursor up key alone will act as such, but when pressed simultaneously with `SELECT` it will act as the `PageUp` key.

Indeed, there's a limitation here: you can't press simultaneously a modified key and a non-modified one, since the MSX modifier key will modify both. So you will have to design you mappings file accordingly.

With this information you should be able to create a key mappings file or to adapt one to your needs. Please take a look at the "Caveats" section in [the main document](https://github.com/Konamiman/NestorDevice/blob/main/docs/Keyboard.md), also note that not all the codes listed in `HidKeyCodes.cs` are supported by the virtual HID driver.

