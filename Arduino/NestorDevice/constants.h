//CH376 pins to Arduino digital connections mapping

#define CH_RD 3
#define CH_WR 4
#define CH_PCS A0
#define CH_A0 13
#define CH_INT 2
#define CH_D0 5
#define CH_A0_DATA LOW
#define CH_A0_COMMAND HIGH

// CH372 USB modes

#define CH_USB_MODE_INVALID_DEVICE 0
#define CH_USB_MODE_DEVICE_EXTERNAL_FIRMWARE 1

// CH372 commands

#define GET_IC_VER 0x01
#define ENTER_SLEEP 0x03
#define RESET_ALL 0x05
#define CHECK_EXIST 0x06
#define GET_TOGGLE 0x0A
#define CHK_SUSPEND 0x0B
#define SET_USB_ID 0x12
#define SET_USB_ADDR 0x13
#define SET_ENDP2__RX_EP0 0x18
#define SET_ENDP3__TX_EP0 0x19
#define SET_ENDP4__RX_EP1 0x1A
#define SET_ENDP5__TX_EP1 0x1B
#define SET_ENDP6__RX_EP2 0x1C
#define SET_ENDP7__TX_EP3 0x1D
#define SET_USB_MODE 0x15
#define GET_STATUS 0x22
#define UNLOCK_USB 0x23
#define RD_USB_DATA0 0x27
#define RD_USB_DATA 0x28
#define WR_USB_DATA3__EP0 0x29
#define WR_USB_DATA5__EP1 0x2A
#define WR_USB_DATA7__EP2 0x2B

// CH372 operation status

#define CMD_RET_SUCCESS 0x51
#define CMD_RET_ABORT 0x5F

// CH372 interruption status

#define USB_BUS_RESET 0x03
#define USB_BUS_RESET_MASK 0x03
#define USB_INT_EP0_SETUP 0x0C
#define USB_INT_EP0_OUT 0x00
#define USB_INT_EP0_IN 0x08
#define USB_INT_EP1_OUT 0x01
#define USB_INT_EP1_IN 0x09
#define USB_INT_EP2_OUT 0x02
#define USB_INT_EP2_IN 0x0A
#define USB_INT_USB_SUSPEND 0x05
#define USB_INT_WAKE_UP 0x06

