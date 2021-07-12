using HIDCtrl;
using MadWizard.WinUSBNet;
using System;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using static System.Console;

namespace Konamiman.RookieDriveDeviceServer
{
    class Program
    {
        const string WINUSB_GUID = "{A5DCBF10-6530-11D2-901F-00C04FB951ED}";
        const int DEFAULT_VID = 0x1209;
        const int DEFAULT_PID = 0x0007;
        const int ROWS_COUNT = 12;
        const byte HID_DRIVER_CMD_RELEASE_ALL = 1;
        const byte HID_DRIVER_CMD_SEND_KEYS = 2;
        const byte HID_DRIVER_CMD_PING = 3;
        const int INTERRUPT_ENDPOINT_ADDRESS = 0x81;
        int vid, pid;
        USBPipe inPipe;
        HIDController hidController;
        SetFeatureKeyboard report;
        int[] keyMappings = new int[ROWS_COUNT * 8];
        int[] modifiedKeyMappings = new int[ROWS_COUNT * 8];
        int msxModifierKeyRow = -1;
        int msxModifierKeyMask;
        bool msxModifierIsPressed;
        byte[] buffer;
        bool debugMode = false;
        string keyMappingsFilePath;

        static int Main(string[] args)
        {
            return new Program().Run(args);
        }

        int Run(string[] args)
        {
            WriteLine(
@"Keyboard server for MSX running Rookie Drive in device mode
By Konamiman, 7/2021
");

            if(args.Length == 0)
            {
                WriteLine(
@"Run with ? for help
");
            }
            if (args.Length > 0 && args[0].Contains("?"))
            {
                WriteLine(
$@"Supported parameters (all optional):

-i <vid>:<pid> - Use this pair of vendor and product ids (hexadecimal numbers) to find the device, default is {DEFAULT_VID:X4}:{DEFAULT_PID:X4}
-k <filename> - Use this file for the key mappings, default is KeyMappings.txt in same directory
-d - Debug mode, print all data received
");
                return 0;
            }

            try
            {
                ProcessParameters(args);
            }
            catch (Exception ex)
            {
                WriteLine($"*** Invalid parameters: {ex.Message}");
                return 1;
            }

            hidController = new HIDController();
            hidController.VendorID = (ushort)DriversConst.TTC_VENDORID;
            hidController.ProductID = (ushort)DriversConst.TTC_PRODUCTID_KEYBOARD;
            hidController.Connect();
            if (!hidController.Connected)
            {
                WriteLine("*** Can't connect to the virtual HID driver, is it installed? (https://tetherscript.com/hid-driver-kit-download/)");
                return 1;
            }

            try
            {
                FillKeyMappings();
            }
            catch (Exception ex)
            {
                WriteLine("*** Bad key mappings file: " + ex.Message);
                return 1;
            }

            WriteLine($"Device VID: {vid:X4}, PID: {pid:X4}\r\n");

            report = new SetFeatureKeyboard();
            report.keys = new byte[6];
            report.ReportID = 1;
            report.Timeout = 2500; //About 0.5s, theorically
            report.Modifier = 0;
            report.Padding = 0;
            for (int i = 0; i < report.keys.Length; i++)
                report.keys[i] = 0;

            inPipe = GetInPipe(false);
            if (inPipe == null)
            {
                WriteLine("*** Device not connected, waiting for it...");
                inPipe = GetInPipe();
            }
            WriteLine("Device connected!");

            while (true)
            {
                try
                {
                    ReadAndProcessIncomingData();
                }
                catch (USBException)
                {
                    WriteLine("*** Device disconnected! Waiting for reconnection...");
                    SendDataToHidDriver(HID_DRIVER_CMD_RELEASE_ALL);
                    inPipe = GetInPipe();
                    WriteLine("Device reconnected!");
                }

                Thread.Sleep(10);
            }
        }

        void ProcessParameters(string[] args)
        {
            vid = DEFAULT_VID;
            pid = DEFAULT_PID;
            keyMappingsFilePath = null;
            debugMode = false;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i].ToLower() == "-d")
                {
                    debugMode = true;
                }
                else if (args[i].ToLower() == "-k")
                {
                    i++;
                    if (i >= args.Length)
                    {
                        throw new Exception("Missing filename after -k");
                    }
                    keyMappingsFilePath = args[i];
                }
                else if (args[i].ToLower() == "-i")
                {
                    i++;
                    if (i >= args.Length)
                    {
                        throw new Exception("Missing vid:pid after -i");
                    }
                    try
                    {
                        var parts = args[i].Split(':');
                        vid = int.Parse(parts[0], NumberStyles.HexNumber);
                        pid = int.Parse(parts[1], NumberStyles.HexNumber);
                    }
                    catch
                    {
                        throw new Exception("Invalid vid:pid, must be a pair of hex numbers, e.g. 12AB:34CD");
                    }
                }
                else
                {
                    throw new Exception($"Unknown parameter: {args[i]}");
                }
            }

            if (keyMappingsFilePath == null)
                keyMappingsFilePath = Path.Combine(AppContext.BaseDirectory, "KeyMappings.txt");
        }

        void FillKeyMappings()
        {
            var lines = File.ReadAllLines(keyMappingsFilePath);
            var linesCount = lines.Length;
            var mappingIndex = 0;
            var row = 0;
            for (var l = 0; l < linesCount; l++)
            {
                var line = lines[l].Trim();
                if (line == "" || line[0] == '#' || line[0] == ';')
                {
                    continue;
                }

                var tokens = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                if (tokens.Length != 8)
                {
                    throw new Exception($"Line {l + 1}: line must contain exactly 8 words");
                }
                tokens = tokens.Reverse().ToArray();

                string normalKeyName, modifiedKeyName;
                var bit = 7;
                foreach (var token in tokens)
                {
                    if (token.Contains(','))
                    {
                        var subTokens = token.Split(',');
                        normalKeyName = subTokens[0];
                        modifiedKeyName = subTokens[1];
                    }
                    else
                    {
                        normalKeyName = token;
                        modifiedKeyName = token;
                    }

                    if (normalKeyName == ".")
                    {
                        keyMappings[mappingIndex] = 0;
                        modifiedKeyMappings[mappingIndex] = 0;
                    }
                    else
                    {
                        if (!Enum.IsDefined(typeof(HidKeyCodes), normalKeyName.ToUpper()))
                        {
                            throw new Exception($"Line {l + 1}: unknown key code {normalKeyName}");
                        }
                        if (!Enum.IsDefined(typeof(HidKeyCodes), modifiedKeyName.ToUpper()))
                        {
                            throw new Exception($"Line {l + 1}: unknown key code {modifiedKeyName}");
                        }

                        var key = (HidKeyCodes)Enum.Parse(typeof(HidKeyCodes), normalKeyName, true);
                        if (key == HidKeyCodes.MSX_MODIFIER)
                        {
                            msxModifierKeyRow = row;
                            msxModifierKeyMask = 0x80 >> bit;
                            keyMappings[mappingIndex] = 0;
                            modifiedKeyMappings[mappingIndex] = 0;
                        }
                        else
                        {
                            keyMappings[mappingIndex] = (int)key;
                            modifiedKeyMappings[mappingIndex] = (int)Enum.Parse(typeof(HidKeyCodes), modifiedKeyName, true);
                        }
                    }

                    mappingIndex++;
                    bit--;
                }
                row++;

                if (mappingIndex >= keyMappings.Length)
                {
                    break;
                }
            }

            if (mappingIndex == 0)
            {
                throw new Exception("The mappings file doesn't contain any mappings information");
            }

            for (int i = mappingIndex; i < keyMappings.Length; i++)
            {
                keyMappings[i] = 0;
                modifiedKeyMappings[i] = 0;
            }
        }

        void ReadAndProcessIncomingData()
        {
            //This will block until data is available in the endpoint
            var length = inPipe.Read(buffer);

            if (length == 0 || (length & 1) != 0)
            {
                SendDataToHidDriver(HID_DRIVER_CMD_PING);
                return;
            }

            var keyIndex = 0;
            var dataIndex = 0;
            int bit;
            while (dataIndex < length && keyIndex < report.keys.Length)
            {
                var row = buffer[dataIndex];
                var keyStates = buffer[dataIndex + 1];
                if (debugMode)
                {
                    Write($"{row:X2},{keyStates:X2} ");
                }

                if (row == msxModifierKeyRow)
                {
                    msxModifierIsPressed = (keyStates & msxModifierKeyMask) == 0;
                }

                var effectiveKeyMappings = msxModifierIsPressed ? modifiedKeyMappings : keyMappings;

                dataIndex += 2;
                bit = 7;
                while (bit >= 0 && keyIndex < 6)
                {
                    var hidKeyCode = effectiveKeyMappings[row * 8 + bit];
                    var keyIsPressed = (keyStates & (1 << bit)) == 0;
                    if (hidKeyCode > 256)
                    {
                        hidKeyCode &= 0xFF;
                        if (keyIsPressed)
                        {
                            report.Modifier = (byte)(report.Modifier | hidKeyCode);
                        }
                        else
                        {
                            report.Modifier = (byte)(report.Modifier & ~hidKeyCode);
                        }

                    }
                    else if (keyIsPressed && hidKeyCode != 0)
                    {
                        report.keys[keyIndex] = (byte)hidKeyCode;
                        keyIndex++;
                    }
                    bit--;
                }
            }

            for (int i = keyIndex; i < report.keys.Length; i++)
            {
                report.keys[i] = 0;
            }

            SendDataToHidDriver();
        }

        USBPipe GetInPipe(bool waitForIt = true)
        {
            while (true)
            {
                var devices = USBDevice.GetDevices(WINUSB_GUID);
                var deviceInfo = devices.FirstOrDefault(d => d.VID == vid && d.PID == pid);
                if (deviceInfo != null)
                {
                    var device = new USBDevice(deviceInfo);
                    var pipe = device.Pipes.Where(p => p.Address == INTERRUPT_ENDPOINT_ADDRESS).Single();
                    buffer = new byte[pipe.MaximumPacketSize];
                    return pipe;
                }
                else if (!waitForIt)
                {
                    return null;
                }

                Thread.Sleep(100);
            }
        }

        void SendDataToHidDriver(byte commandCode = HID_DRIVER_CMD_SEND_KEYS)
        {
            report.CommandCode = commandCode;
            byte[] buf = ConvertStructToByteArray(report, Marshal.SizeOf(report));
            hidController.SendData(buf, (uint)Marshal.SizeOf(report));
        }

        static byte[] ConvertStructToByteArray(SetFeatureKeyboard data, int size)
        {
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(data, ptr, false);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }
    }
}
