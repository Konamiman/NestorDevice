#include "constants.h"


/*************
 * Variables *
 *************/

bool unlocked;

void ClearVariables()
{
}


/****************
 * setup & loop *
 ****************/

void setup()
{
    printf_begin();
    Serial.begin(115200);
    ClearVariables();
    SetPins();
    
    bool initOk = CH376Init();
    if(initOk) {
      attachInterrupt(digitalPinToInterrupt(CH_INT), HandleInterrupt, FALLING);
    }
}

void loop()
{
}


/************************
 * USB protocol handler *
 ************************/

void HandleInterrupt()
{
  unlocked = false;

  CH_WriteCommand(GET_STATUS);
  byte interruptType = CH_ReadData();

  if((interruptType & USB_BUS_RESET_MASK) == USB_BUS_RESET) {
    interruptType = USB_BUS_RESET;
  }

  printInterruptName(interruptType);

  switch(interruptType)
  {
      case USB_INT_USB_SUSPEND:
        CH_WriteCommand(ENTER_SLEEP);
        ClearVariables();
        break;
      case USB_BUS_RESET:
        ClearVariables();
        break;
  }

  if(!unlocked && interruptType != USB_INT_USB_SUSPEND) {
    CH_WriteCommand(UNLOCK_USB);
  }
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
	
	  digitalWrite(CH_PCS, HIGH);
	  digitalWrite(CH_RD, HIGH);
	  digitalWrite(CH_WR, HIGH);
}

bool CH376Init()
{
    CH_WriteCommand(CHECK_EXIST);
    CH_WriteData(0x57);
    byte data = CH_ReadData();
    if(data != 0xA8) {
      // For some reason the first check always fails
      CH_WriteData(0x57);
      byte data = CH_ReadData();
      if(data != 0xA8) {
        printf("*** CH372 hardware not found!\r\n");
        return false;
      }
    }

    CH_WriteCommand(RESET_ALL);
    delay(50);

    CH_WriteCommand(SET_USB_MODE);
    CH_WriteData(CH_USB_MODE_INVALID_DEVICE);
    delay(1);
    if (CH_ReadData() != CMD_RET_SUCCESS) {
      printf("*** CH372 set mode 0 failed\r\n");
      return false;
    }

    CH_WriteCommand(SET_USB_MODE);
    CH_WriteData(CH_USB_MODE_DEVICE_EXTERNAL_FIRMWARE);
    delay(1);
    if (CH_ReadData() != CMD_RET_SUCCESS) {
      printf("*** CH372 set mode 1 failed\r\n");
      return false;
    }

    CH_WriteCommand(CHK_SUSPEND);
    CH_WriteData(0x10);
    CH_WriteData(0x04);

    printf("CH372 initialized!\r\n");
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

 void printInterruptName( byte interruptCode)
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
    printf("Unknown interrupt received: 0x%02X\r\n", interruptCode);
   }
   else {
     printf("Int: %s\r\n", name);
   }
 }