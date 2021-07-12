/**
 * Skeleton for a USB device implementation with a CH372/375/376.
 * 
 * No real device functionality is implemented,
 * only the standard USB requests over the control endpoint.
 * 
 * This is experimental code that attempts tu implemet the
 * CH372 "external firmware" mode. It doesn't really work,
 * for some reason Windows throws timeout errors
 * when trying to configure the device.
 * It looks like a limitation of the CH376.
 * 
 * So this is pretty much experimental code
 * and not fully completed/tested.
 */

#include "constants.h"


/*************
 * Variables *
 *************/

bool unlocked;
bool handlingSetupRequest;
byte* dataToTransfer;
int dataToTransferIndex;
int dataLeftToTransfer;
bool sendingData;
byte addressToSet;
bool configured;
byte hidReportData[3];
bool intEndpointHasData;
bool getConfigReceived;
long initialMillis;
bool wait = false;

void ResetState()
{
  handlingSetupRequest = false;
  dataLeftToTransfer = 0;
  addressToSet = 0;
  configured = false;
  intEndpointHasData = false;
  getConfigReceived = false;

  hidReportData[0] = 0x7F;
  hidReportData[1] = 0x7F;
  hidReportData[2] = 0;
}


/*******************
 * Descriptor data *
 *******************/

/*
byte deviceDescriptor[] = {
  0x12, //Length
  USB_DESC_DEVICE,
  0x10, 0x01, //USB version,
  0xFF, 128, 55, //Class, subclass, protocol
  EP0_PIPE_SIZE, //Max packet size for EP0
  0x09, 0x12, //VID (https://pid.codes)
  0x03, 0x00, //PID (testing)
  0x20, 0x02, //Device release number
  0, //Manufacturer string id
  0, //Product string id
  0, //Serial number string id
  0x01  //Number of configurations
};
*/

byte deviceDescriptor[] = {
  0x12, //Length
  USB_DESC_DEVICE,
  0x10, 0x01, //USB version,
  0x00, 0x00, 0x00, //Class, subclass, protocol
  EP0_PIPE_SIZE, //Max packet size for EP0
  0x09, 0x12, //VID (https://pid.codes)
  0x02, 0x00, //PID (testing)
  0x00, 0x01, //Device release number
  STRING_DESC_MANUFACTURER, //Manufacturer string id
  STRING_DESC_PRODUCT, //Product string id
  STRING_DESC_SERIAL, //Serial number string id
  0x01  //Number of configurations
};

/*
byte configurationDescriptor[] = {
  0x09, //Length
  USB_DESC_CONFIGURATION,
  46, 0x00, //Total length
  0x01, //Number of interfaces
  0x01, //Configuration value
  0x00, //String descriptor for configuration
  0x80, //Attributes (no self-powered, no remote wake-up)
  34, //Max power (68mA)

  //Interface descriptor

  0x09, //Length
  USB_DESC_INTERFACE,
  0x00, //Interface number
  0x00, //Alternate setting
  4, //Number of endpoints
  0xFF, 0x80, 0x37, //Class, subclass, protocol
  0, //String descriptor for interface

  //Endpoints

  7,
  USB_DESC_ENDPOINT,
  0x82,
  2,
  64,0,
  0,

  7,
  USB_DESC_ENDPOINT,
  0x02,
  2,
  64,0,
  0,

  7,
  USB_DESC_ENDPOINT,
  0x81,
  3,
  8,0,
  1,

  7,
  USB_DESC_ENDPOINT,
  0x01,
  2,
  8,0,
  0
};
*/

byte configurationDescriptor[] = {
  0x09, //Length
  USB_DESC_CONFIGURATION,
  0x12, 0x00, //Total length
  0x01, //Number of interfaces
  0x01, //Configuration value
  0x00, //String descriptor for configuration
  0x80, //Attributes (no self-powered, no remote wake-up)
  34, //Max power (68mA)

  //Interface descriptor

  0x09, //Length
  USB_DESC_INTERFACE,
  0x00, //Interface number
  0x00, //Alternate setting
  0x00, //Number of endpoints
  0xFF, 0xFF, 0xFF, //Class, subclass, protocol
  STRING_DESC_PRODUCT //String descriptor for interface
};

/*
byte configurationDescriptor[] = {
  0x09,        //   bLength
  USB_DESC_CONFIGURATION,        //   bDescriptorType (Configuration)
  0x22, 0x00,  //   wTotalLength 34
  0x01,        //   bNumInterfaces 1
  0x01,        //   bConfigurationValue
  0x00,        //   iConfiguration (String Index)
  0x80,        //   bmAttributes NO Remote Wakeup
  34,          //   bMaxPower 68mA

  0x09,        //   bLength
  USB_DESC_INTERFACE,        //   bDescriptorType (Interface)
  0x00,        //   bInterfaceNumber 0
  0x00,        //   bAlternateSetting
  0x01,        //   bNumEndpoints 1
  0x03,        //   bInterfaceClass
  0x00,        //   bInterfaceSubClass
  0x00,        //   bInterfaceProtocol
  0x00,        //   iInterface (String Index)

  0x09,        //   bLength
  0x21,        //   bDescriptorType (HID)
  0x10, 0x01,  //   bcdHID 1.10
  0x00,        //   bCountryCode
  0x01,        //   bNumDescriptors
  0x22,        //   bDescriptorType[0] (HID)
  0x29, 0x00,  //   wDescriptorLength[0] 41

  0x07,        //   bLength
  0x05,        //   bDescriptorType (Endpoint)
  0x81,        //   bEndpointAddress (IN/D2H)
  0x03,        //   bmAttributes (Interrupt)
  0x03, 0x00,  //   wMaxPacketSize 3
  0x0A,        //   bInterval 10 (unit depends on device speed)
};
*/

byte hidReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
  0xA1, 0x00,        //   Collection (Physical)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x15, 0x00,        //     Logical Minimum (0)
  0x26, 0xFF, 0x00,  //     Logical Maximum (255)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x02,        //     Report Count (2)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x02,        //   Usage Maximum (0x02)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
};

byte languagesDescriptor[] = {
  0x04, //Length
  USB_DESC_STRING,
  0x09, 0x04 //English (US)
};

byte productStringDescriptor[] = {
  0x1A, //length,
  USB_DESC_STRING,
  //"NestorDevice"
  0x4E, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00, 0x6F, 0x00,
  0x72, 0x00, 0x44, 0x00, 0x65, 0x00, 0x76, 0x00, 0x69, 0x00,
  0x63, 0x00, 0x65, 0x00
};

byte manufacturerStringDescriptor[] = {
  0x2A, //length,
  USB_DESC_STRING,
  //"Konamiman Industries"
  0x4B, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x6D, 0x00, 
  0x69, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x20, 0x00, 
  0x49, 0x00, 0x6E, 0x00, 0x64, 0x00, 0x75, 0x00, 0x73, 0x00,
  0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x65, 0x00, 0x73, 0x00
};

byte serialNumberDescriptor[] = {
  0x06, //length
  USB_DESC_STRING,
  0x33, 0x00, 0x34, 0x00
};

byte oneOneByte[] = {1};
byte oneZeroByte[] = {0};
byte twoZeroBytes[] = {0,0};


/****************
 * setup & loop *
 ****************/

void setup()
{
    printf_begin();
    Serial.begin(115200);
    ResetState();
    SetPins();

    while(Serial.available() > 0) {
      Serial.read();
    }
    
    bool initOk = CH376Init();
    if(initOk) {
      //attachInterrupt(digitalPinToInterrupt(CH_INT), HandleInterrupt, LOW);
    }
}

byte lastInterrupt = 0;
void loop()
{
  /*
  As an alternative to the "attachInterrupt" call in setup,
  the following code can be used in loop:
*/
  byte interruptType;

  if((CH_ReadStatus() & 0x80) == 0) {
    CH_WriteCommand(GET_STATUS);
    interruptType = CH_ReadData();
    HandleInterrupt(interruptType);
  }
  /*else if(lastInterrupt != USB_INT_USB_SUSPEND && lastInterrupt != USB_INT_WAKE_UP && (lastInterrupt & 3) != USB_BUS_RESET) {
    CH_WriteCommand(GET_STATUS);
    interruptType = CH_ReadData();
    if(interruptType != lastInterrupt) {
      HandleInterrupt(interruptType);
      lastInterrupt = interruptType;
    }
  }*/

return;

  while(Serial.available() > 0) {
    int byte = Serial.read();
    switch(byte) {
      case 'w': //UP
        hidReportData[0] = 0;
        break;
      case 's': //DOWN
        hidReportData[0] = 255;
        break;
      case 'a': //LEFT
        hidReportData[1] = 0;
        break;
      case 'd': //RIGHT
        hidReportData[1] = 255;
        break;
      case 'z': //BTN 1
        hidReportData[2] |= 1;
        break;
      case 'x': //BTN 2
        hidReportData[2] |= 2;
        break;
      default:
        hidReportData[0] = 0x7F;
        hidReportData[1] = 0x7F;
        hidReportData[2] = 0;
    }
    WriteHidData(1);
  }
}


void WriteHidData(int endpoint)
{
  if(intEndpointHasData) {
    //printf("  Skipping writing int data\r\n");
    return;
  }

  //printf("  Writing int data to ep %i: 0x%02X 0x%02X 0x%02X\r\n", endpoint, hidReportData[0], hidReportData[1], hidReportData[2]);
  byte cmd = WR_USB_DATA3__EP0 + endpoint;
  CH_WriteCommand(cmd);
  CH_WriteData(3);
  CH_WriteData(hidReportData[0]);
  CH_WriteData(hidReportData[1]);
  CH_WriteData(hidReportData[2]);

  if(endpoint == 1) {
    intEndpointHasData = true;
  }
}


/************************
 * USB protocol handler *
 ************************/

void HandleInterrupt(byte interruptType)
{
  initialMillis = millis();
  unlocked = false;

  //CH_WriteCommand(GET_STATUS);
  //byte interruptType = CH_ReadData();

  if((interruptType & USB_BUS_RESET_MASK) == USB_BUS_RESET) {
    interruptType = USB_BUS_RESET;
  }

  /*if(interruptType == USB_INT_EP0_OUT) {
    //printf("!\r\n");
    getConfigReceived = false;
    while(interruptType == USB_INT_EP0_OUT) {
      CH_WriteCommand(UNLOCK_USB);
      CH_WriteCommand(GET_STATUS);
      interruptType = CH_ReadData();
    }
  }*/

  PrintInterruptName(interruptType);

  switch(interruptType)
  {
      case USB_INT_USB_SUSPEND:
        CH_WriteCommand(ENTER_SLEEP);
        ResetState();
        break;
      case USB_BUS_RESET:
        ResetState();
        break;
      case USB_INT_EP0_SETUP:
        HandleSetupToken();
        break;
      case USB_INT_EP0_IN:
        HandleControlInToken();
        break;
      case USB_INT_EP0_OUT:
        //Nothing to do since we don't implement
        //control OUT requests. We're going to receive
        //this interrupt only as the status phase of IN requests,
        //where there will be no data received.
        //SetEndpoint(0, SET_ENDP_RX, 0);
        handlingSetupRequest = false;
        CH_WriteCommand(RD_USB_DATA0);
        //unlocked=true;
        byte length = CH_ReadData();
        //printf("  Received %i bytes\r\n", length);
        //CH_WriteCommand(GET_TOGGLE);
        //byte toggle = CH_ReadData();
        /*if(bitRead(toggle, 4)) {
          //printf("  Toggle OK\r\n");
        }
        else {
          //printf("  Toggle BAD!\r\n");
        }*/
        //wait = true;
        break;
      case USB_INT_EP1_IN:
        intEndpointHasData = false;
        break;
  }

  if(!unlocked && interruptType != USB_INT_USB_SUSPEND) {
    CH_WriteCommand(UNLOCK_USB);
    ////printf("  ms spent: %i\r\n", millis()-initialMillis);
  }
}

void HandleSetupToken()
{
  CH_WriteCommand(RD_USB_DATA0);
  byte length = CH_ReadData();
  if(length != SETUP_COMMAND_SIZE) {
    SetEndpoint(0, SET_ENDP_TX, SET_ENDP_STALL);
    //printf("  *** Bad setup data length: %i\r\n", length);
    return;
  }

  byte requestBytes[SETUP_COMMAND_SIZE];
  //printf("  ");
  for(int i=0; i<SETUP_COMMAND_SIZE; i++) {
    requestBytes[i] = CH_ReadData();
    //printf("0x%02X ", requestBytes[i]);
  }
  //printf("\r\n");

  byte bmRequestType = requestBytes[0];
  byte bRequest = requestBytes[1];
  int wValue = requestBytes[2] + (requestBytes[3] << 8);
  int wIndex = requestBytes[4] + (requestBytes[5] << 8);
  int wLength = requestBytes[6] + (requestBytes[7] << 8);
  //printf("  bRequest: %i\r\n", bRequest);
  //printf("  wValue: 0x%04X\r\n", wValue);
  //printf("  wIndex: 0x%04X\r\n", wIndex);
  //printf("  wLength: 0x%04X\r\n", wLength);

  if(bmRequestType == 0xA1 && bRequest == USB_REQ_GET_HID_REPORT && highByte(wValue) == 1) {
    //printf("  GET_HID_REPORT request\r\n");
    dataToTransfer = hidReportData;
    dataToTransferIndex = 0;
    WriteDataForEndpoint0(3);
    return;
  }

  if(bmRequestType == 0x81 && bRequest == USB_REQ_GET_DESCRIPTOR && highByte(wValue) == USB_DESC_HID_REPORT) {
  }
  else if((bmRequestType & BM_REQ_TYPE_MASK) != BM_REQ_TYPE_STD) {
    //printf("  *** Non-standard request, unsupported\r\n", length);
    SetEndpoint(0, SET_ENDP_TX, SET_ENDP_STALL);
    return;
  }

  sendingData = (bitRead(bmRequestType, 7) == 1);
  dataToTransferIndex = 0;

  if(sendingData) {
    dataLeftToTransfer = PrepareControlDataToSend(bRequest, wValue);
    if(dataLeftToTransfer == 0) {
      //printf("  *** Unsupported IN request\r\n", length);
      SetEndpoint(0, SET_ENDP_TX, SET_ENDP_STALL);
    }
    else {
      dataLeftToTransfer = min(dataLeftToTransfer, wLength);
      byte amountToSendNow = min(dataLeftToTransfer, EP0_PIPE_SIZE);
      WriteDataForEndpoint0(amountToSendNow);
    }
  }
  else {
    SetEndpoint(0, SET_ENDP_TX, 0);
    SetEndpoint(0, SET_ENDP_RX, 0);
    dataLeftToTransfer = 0;
    bool ok = ActOnOutputRquest(bRequest, wValue);
    if(!ok) {
      //printf("  *** Unsupported OUT request: %i\r\n", bRequest);
      SetEndpoint(0, SET_ENDP_TX, SET_ENDP_STALL);
    }
    else {
      //printf("  OK OUT request!\r\n");
    }
  }

  handlingSetupRequest = (dataLeftToTransfer > 0);
}

void WriteDataForEndpoint0(byte amount)
{
  //printf("  Writing %i bytes: ", amount);
  CH_WriteCommand(WR_USB_DATA3__EP0);
  CH_WriteData(amount);
  for(int i=0; i<amount; i++) {
    //printf("0x%02X ", dataToTransfer[dataToTransferIndex+i]);
    CH_WriteData(dataToTransfer[dataToTransferIndex+i]);
  }
  //printf("\r\n");

  dataToTransferIndex += amount;
  dataLeftToTransfer -= amount;

  //SetEndpoint(0, SET_ENDP_TX, amount);
  //Unlock();
}

int PrepareControlDataToSend(byte bRequest, int wValue)
{
    switch(bRequest) {
      case USB_REQ_GET_INTERFACE:
        //printf("  GET_INTERFACE\r\n");
        dataToTransfer = oneOneByte;
        return 1;
      case USB_REQ_GET_CONFIGURATION:
        //printf("  GET_CONFIGURATION\r\n");
        dataToTransfer = configured ? oneOneByte : oneZeroByte;
        return 1;
      case USB_REQ_GET_STATUS:
        //printf("  GET_STATUS\r\n");
        dataToTransfer = twoZeroBytes;
        return 2;  
      case USB_REQ_GET_DESCRIPTOR:
        //printf("  GET_DESCRIPTOR: ");
        byte descriptorType = highByte(wValue);
        switch(descriptorType)
        {
          case USB_DESC_HID_REPORT:
            //printf("HID REPORT\r\n");
            dataToTransfer = hidReportDescriptor;
            return sizeof(hidReportDescriptor);
          case USB_DESC_DEVICE:
            //printf("DEVICE\r\n");
            dataToTransfer = deviceDescriptor;
            return sizeof(deviceDescriptor);
          case USB_DESC_CONFIGURATION:
            //printf("CONFIGURATION\r\n");
            getConfigReceived = true;
            dataToTransfer = configurationDescriptor;
            return sizeof(configurationDescriptor);
          case USB_DESC_STRING:
            //printf("STRING: ");
            byte stringIndex = lowByte(wValue);  
            switch(stringIndex)
            {
              case 0:
                //printf("Language\r\n");
                dataToTransfer = languagesDescriptor;
                return sizeof(languagesDescriptor);
              case STRING_DESC_PRODUCT:
                //printf("Product\r\n");
                dataToTransfer = productStringDescriptor;
                return sizeof(productStringDescriptor);
              case STRING_DESC_MANUFACTURER:
                //printf("Manufacturer\r\n");
                dataToTransfer = manufacturerStringDescriptor;
                return sizeof(manufacturerStringDescriptor);
              case STRING_DESC_SERIAL:
                //printf("Serial number\r\n");
                dataToTransfer = serialNumberDescriptor;
                return sizeof(serialNumberDescriptor);  
              default:
                //printf("Unknown! (%i)\r\n", stringIndex);
                return 0;    
            }
            default:
                //printf("Unknown! (%i)\r\n", descriptorType);
                return 0;    
        }
      default:
        //printf("  Unsupported request! (0x%02X)\r\n", bRequest);
        return 0;
    }
}

bool ActOnOutputRquest(byte bRequest, int wValue)
{
  switch(bRequest)
  {
    case USB_REQ_SET_ADDRESS:
      addressToSet = lowByte(wValue);
      //printf("  SET_ADDRESS: %i\r\n", addressToSet);
      return true;
    case USB_REQ_SET_CONFIGURATION:
      //printf("  SET_CONFIGURATION: 0x%04X\r\n", wValue);
      configured = (wValue != 0);
      return true;
  }

  return false;
}

void HandleControlInToken()
{
  if(addressToSet != 0) {
    //printf("  Setting address: %i\r\n", addressToSet);
    CH_WriteCommand(SET_USB_ADDR);
    CH_WriteData(addressToSet);
    addressToSet = 0;
    handlingSetupRequest = false;
    dataLeftToTransfer = 0;
    CH_WriteCommand(UNLOCK_USB);
    return;
  }

  byte amountToSendNow = 0;

  if(handlingSetupRequest && !sendingData) {
    SetEndpoint(0, SET_ENDP_TX, 0);
  }
  else if(handlingSetupRequest && sendingData) {
    amountToSendNow = min(dataLeftToTransfer, EP0_PIPE_SIZE);
    
    if(amountToSendNow > 0) {
      WriteDataForEndpoint0(amountToSendNow);
    }
    if(dataLeftToTransfer == 0) {
      //SetEndpoint(0, SET_ENDP_TX, 0);
      handlingSetupRequest = false;
      //CH_WriteCommand(SET_ENDP3__TX_EP0);
      //CH_WriteCommand(128+64+amountToSendNow);
      //CH_WriteCommand(SET_ENDP2__RX_EP0);
      //CH_WriteCommand(128+64);
      //Unlock();
      //delay(100);
    }
    /*if(dataLeftToTransfer == 0) {
      if(handlingSetupRequest) {
        SetEndpoint(0, SET_ENDP_TX, 0);
      }
      handlingSetupRequest = false;*/
      /*CH_WriteCommand(SET_ENDP2__RX_EP0);
      CH_WriteCommand(128+0);
      CH_WriteCommand(SET_ENDP3__TX_EP0);
      CH_WriteCommand(128+64+amountToSendNow);
    }*/
  }

  if(amountToSendNow == 0 && dataLeftToTransfer == 0) {
    //SetEndpoint(0, SET_ENDP_TX, 0);
  }
      CH_WriteCommand(UNLOCK_USB);
}

void SetEndpoint(int endpointNumber, int rxOrTx, int sizeOrStatus)
{
  ////printf("  SET_ENDPT: %i, %i, %i\r\n", endpointNumber, rxOrTx, sizeOrStatus);
  byte cmd = SET_ENDP2__RX_EP0 + 2 * endpointNumber + rxOrTx;
  CH_WriteCommand(cmd);
  CH_WriteData(sizeOrStatus);
}

void Unlock()
{
  CH_WriteCommand(UNLOCK_USB);
  unlocked = true;
}


/**********************
 * CH376 reset & init *
 **********************/

void SetPins()
{
  	pinMode(CH_RD, OUTPUT);
	  pinMode(CH_WR, OUTPUT);
	  pinMode(CH_PCS, OUTPUT);
	  pinMode(CH_A0, OUTPUT);
    pinMode(CH_INT, INPUT_PULLUP);
	
	  digitalWrite(CH_PCS, HIGH);
	  digitalWrite(CH_RD, HIGH);
	  digitalWrite(CH_WR, HIGH);
}

bool CH376Init()
{
    CH_WriteCommand(RESET_ALL);
    delay(50);

    CH_WriteCommand(CHECK_EXIST);
    CH_WriteData(0x57);
    byte data = CH_ReadData();
    if(data != 0xA8) {
      // For some reason the first check always fails
      CH_WriteData(0x57);
      byte data = CH_ReadData();
      if(data != 0xA8) {
        //printf("*** CH372 hardware not found! (0x57 --> 0x%02X)\r\n", data);
        return false;
      }
    }

    CH_WriteCommand(SET_USB_MODE);
    CH_WriteData(CH_USB_MODE_INVALID_DEVICE);
    delay(1);
    if (CH_ReadData() != CMD_RET_SUCCESS) {
      //printf("*** CH372 set mode %i failed\r\n", CH_USB_MODE_INVALID_DEVICE);
      return false;
    }

    /*CH_WriteCommand(0x12);
    CH_WriteData(0x09);
    CH_WriteData(0x12);
    CH_WriteData(0x02);
    CH_WriteData(0x00);*/

    CH_WriteCommand(SET_USB_MODE);
    CH_WriteData(CH_USB_MODE_DEVICE_EXTERNAL_FIRMWARE);
    delay(1);
    if (CH_ReadData() != CMD_RET_SUCCESS) {
      //printf("*** CH372 set mode %i failed\r\n", CH_USB_MODE_DEVICE_EXTERNAL_FIRMWARE);
      return false;
    }

    CH_WriteCommand(CHK_SUSPEND);
    CH_WriteData(0x10);
    CH_WriteData(0x04);

    //printf("CH372 initialized!\r\n");
    return true;
}


/*********************************
 * Low-level access to the CH372 *
 *********************************/

byte CH_ReadData()
{
	return CH_ReadPort(CH_A0_DATA);
}

byte CH_ReadStatus()
{
	return CH_ReadPort(CH_A0_COMMAND);
}

byte CH_ReadPort(int address)
{
	byte data = 0;

	digitalWrite(CH_A0, address);

	for (int i = 0; i < 8; i++)
	{
		pinMode(CH_D0 + i, INPUT);
		digitalWrite(CH_D0 + i, LOW);
	}

	digitalWrite(CH_PCS, LOW);
	digitalWrite(CH_RD, LOW);

	for (int i = 0; i < 8; i++)
	{
		data >>= 1;
		data |= (digitalRead(CH_D0 + i) == HIGH) ? 128 : 0;
	}

	digitalWrite(CH_RD, HIGH);
	digitalWrite(CH_PCS, HIGH);

	return data;
}

byte CH_WriteData(byte data)
{
	return CH_WritePort(CH_A0_DATA, data);
}

byte CH_WriteCommand(byte command)
{
	return CH_WritePort(CH_A0_COMMAND, command);
}

byte CH_WritePort(int address, byte data)
{
	digitalWrite(CH_A0, address);

	for (int i = 0; i < 8; i++)
	{
		pinMode(CH_D0 + i, OUTPUT);
	}

	digitalWrite(CH_PCS, LOW);
	digitalWrite(CH_WR, LOW);
	
	for (int i = 0; i < 8; i++)
	{
		digitalWrite(CH_D0 + i, (((data >> i) & 1) == 0) ? LOW : HIGH);
	}

	digitalWrite(CH_WR, HIGH);
	digitalWrite(CH_PCS, HIGH);

	return data;
}


/**********************************
 * Redirect printf to serial port *
 **********************************/

//https://forum.arduino.cc/t/printf-to-the-serial-port/333975/4

int serial_putc( char c, FILE * ) 
{
  Serial.write( c );
  return c;
} 

void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}


/*****************
 * Debug helpers *
 *****************/

 void PrintInterruptName( byte interruptCode)
 {
   char* name = NULL;

   switch(interruptCode) {
      case USB_BUS_RESET:
        name = "BUS_RESET";
        break;
      case USB_INT_EP0_SETUP:
        name = "EP0_SETUP";
        break;
      case USB_INT_EP0_OUT:
        name = "EP0_OUT";
        break;
      case USB_INT_EP0_IN:
        name = "EP0_IN";
        break;
      case USB_INT_EP1_OUT:
        name = "EP1_OUT";
        break;
      case USB_INT_EP1_IN:
        name = "EP1_IN";
        break;
      case USB_INT_EP2_OUT:
        name = "EP2_OUT";
        break;
      case USB_INT_EP2_IN:
        name = "EP2_IN";
        break;
      case USB_INT_USB_SUSPEND:
        name = "USB_SUSPEND";
        break;
      case USB_INT_WAKE_UP:
        name = "WAKE_UP";
        break;
   }

   if(name == NULL) {
    //printf("Unknown interrupt received: 0x%02X\r\n", interruptCode);
   }
   else {
     //printf("Int: %s @ %i\r\n", name, millis());
   }
 }