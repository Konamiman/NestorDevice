; USB device implementation for MSX with Rookie Drive
; By Konamiman, 7/2021
;
; This is an attempt to implement a full device
; in CH372 "external firmware" mode. It doesn't really work,
; for some reason Windows throws timeout errors
; when trying to configure the device.
; It looks like a limitation of the CH376.
;
; So this is pretty much experimental code
; and not fully completed/tested.


;*************
;* Constants *
;*************

	;0: Generate MSX-DOS executable
	;1: Generate ROM
ROM:	equ	1

NOOB_SETS_ADD:	equ	0

	if	ROM=1

CHPUT:	equ	00A2h
SAVE_SP:	equ	8010h

	endif

	if	ROM=0

TERM0:	equ	0
CONOUT:	equ	2
DOS:	equ	0005h

	endif

CH_DATA_PORT:	equ	20h
CH_COMMAND_PORT:	equ	21h

;* CH372 commands

GET_IC_VER:	equ	01h
ENTER_SLEEP:	equ	03h
RESET_ALL:	equ	05h
CHECK_EXIST:	equ	06h
GET_TOGGLE:	equ	0Ah
CHK_SUSPEND:	equ	0Bh
SET_USB_ID:	equ	12h
SET_USB_ADDR:	equ	13h
SET_ENDP2_R0:	equ	18h
SET_ENDP3_T0:	equ	19h
SET_ENDP4_R1:	equ	1Ah
SET_ENDP5_T1:	equ	1Bh
SET_ENDP6_R2:	equ	1Ch
SET_ENDP7_T2:	equ	1Dh
SET_USB_MODE:	equ	15h
GET_STATUS:	equ	22h
UNLOCK_USB:	equ	23h
RD_USB_DATA0:	equ	27h
RD_USB_DATA:	equ	28h
WR_USB_DATA3_0:	equ	29h
WR_USB_DATA5_1:	equ	2Ah
WR_USB_DATA7_2:	equ	2Bh

;* CH372 operation status

CMD_RET_SUCCESS:	equ	51h
CMD_RET_ABORT:	equ	5Fh

;* CH372 interruption status

BUS_RESET:	equ	03h
BUS_RESET_MASK:	equ	03h
INT_EP0_SETUP:	equ	0Ch
INT_EP0_OUT:	equ	00h
INT_EP0_IN:	equ	08h
INT_EP1_OUT:	equ	01h
INT_EP1_IN:	equ	09h
INT_EP2_OUT:	equ	02h
INT_EP2_IN:	equ	0Ah
INT_USB_SUSP:	equ	05h
INT_WAKE_UP:	equ	06h

;* USB descriptor codes

USBD_DEVICE:	equ	1
USBD_CONFIG:	equ	2
USBD_STRING:	equ	3
USBD_INTERF:	equ	4
USBD_ENDPT:	equ	5

USBD_HID:	equ	21
USBD_HIDREP:	equ	22

;* USB string descriptor ids

STRD_MANUF:	equ	1
STRD_PRODUCT:	equ	2
STRD_SERIAL:	equ	3

;* USB requests

;* IN
R_GET_CONFIG:	equ	8
R_GET_DESCR:	equ	6
R_GET_INTERF:	equ	10
R_GET_STATUS:	equ	0
R_GET_HIDREP:	equ	6

;* OUT
R_CLEAR_FEAT:	equ	1
R_SET_ADDRESS:	equ	5
R_SET_CONFIG:	equ	9
R_SET_FEATURE:	equ	3


;*************************
;* Startup and main loop *
;*************************

	if	ROM=1

	org	4000h
	db	41h,42h
	dw	START
	ds	12
START:
	ld	hl,0
	add	hl,sp
	ld	(SAVE_SP),hl

	endif

	if	ROM=0

	org	100h

	endif

	di
	call	CH_INIT

LOOP:
	in	a,(CH_COMMAND_PORT)
	and	80h
	call	z,HANDLE_CH_INT

	;On space press, exit.
	;On cursor press, reset CH372.
	ld	a,8
	call	DO_SNSMAT
	bit	0,a
	jp	z,EXIT
	and	11110000b
	cp	11110000b
	call	nz,CH_INIT

	jr	LOOP


;****************************
;* CH372 and variables init *
;****************************

CH_INIT:
	ld	a,RESET_ALL
	out	(CH_COMMAND_PORT),a

	ei
	halt
	halt
	halt
	di

	ld	a,CHECK_EXIST
	out	(CH_COMMAND_PORT),a
	ld	a,0A8h
	out	(CH_DATA_PORT),a
	in	a,(CH_DATA_PORT)
	cp	57h
	ld	hl,NO_CH_S
	jp	nz,PREXIT

	ei
	halt
	halt
	halt
	di

	ld	a,SET_USB_MODE
	out	(CH_COMMAND_PORT),a
	xor	a	;Invalid device mode
	out	(CH_DATA_PORT),a
	ei
	halt
	halt
	halt
	di
	in	a,(CH_DATA_PORT)
	cp	CMD_RET_SUCCESS
	ld	hl,CH_MODE_ERR_S
	jp	nz,PREXIT

	ld	a,SET_USB_MODE
	out	(CH_COMMAND_PORT),a
	ld	a,1	;External firmware mode
	out	(CH_DATA_PORT),a
	ei
	halt
	halt
	halt
	di
	in	a,(CH_DATA_PORT)
	cp	CMD_RET_SUCCESS
	ld	hl,CH_MODE_ERR_S
	jp	nz,PREXIT

	ld	a,CHK_SUSPEND
	out	(CH_COMMAND_PORT),a
	ld	a,10h
	out	(CH_DATA_PORT),a
	ld	a,04h
	out	(CH_DATA_PORT),a

	ld	hl,INIT_OK_S
	call	PRINT

CLEAR_VARS:
	ld	hl,VAR_START
	ld	de,VAR_START+1
	ld	bc,VAR_END-VAR_START-1
	ld	(hl),0
	ldir
	ret


;***************************
;* Handle CH372 interrupts *
;***************************

HANDLE_CH_INT:
	ld	a,GET_STATUS
	out	(CH_COMMAND_PORT),a
	in	a,(CH_DATA_PORT)

	cp	INT_USB_SUSP
	jp	z,HANDLE_SUSPEND

	;All interrupts except SUSPEND
	;require UNLOCK_USB execution at the end
	ld	hl,DO_UNLOCK
	push	hl

	ld	b,a
	and	BUS_RESET_MASK
	cp	BUS_RESET_MASK
	jp	z,HANDLE_BUSRESET
	ld	a,b

	cp	INT_WAKE_UP
	jp	z,HANDLE_WAKEUP
	cp	INT_EP0_SETUP
	jp	z,HANDLE_SETUP
	cp	INT_EP0_IN
	jp	z,HANDLE_EP0_IN
	cp	INT_EP0_OUT
	jp	z,HANDLE_EP0_OUT

	ld	hl,UNK_INT_S
	push	af
	call	PRINT
	pop	af
	call	PRINTHEX
	ld	hl,CRLF_S
	call	PRINT
	ret

DO_UNLOCK:
	ld	a,UNLOCK_USB
	out	(CH_COMMAND_PORT),a
	ret


;--- Handle BUS RESET, SUSPEND, WAKE UP interrupts

HANDLE_BUSRESET:
	ld	hl,BUSRESET_S
	call	PRINT
	call	CLEAR_VARS
	ret

HANDLE_SUSPEND:
	ld	a,ENTER_SLEEP
	out	(CH_COMMAND_PORT),a
	ld	hl,SUSPEND_S
	call	PRINT
	call	CLEAR_VARS
	ret

HANDLE_WAKEUP:
	ld	hl,WAKEUP_S
	call	PRINT
	ret


;--- Handle SETUP token received interrupt

HANDLE_SETUP:
	ld	hl,SETUP_S
	call	PRINT

	ld	c,CH_DATA_PORT

	;* Read SETUP data to BUFFER, must be 8 bytes

	ld	a,RD_USB_DATA0
	out	(CH_COMMAND_PORT),a
	in	a,(c)
	cp	8
	ld	hl,BAD_SETUP_S
	jp	nz,PRSTALL

	ld	hl,TWOSPACES_S
	call	PRINT

	ld	hl,BUFFER
	ld	b,8
	ld	c,CH_DATA_PORT
RD_SETUP_LOOP:
	in	a,(c)
	ld	(hl),a
	push	hl
	call	PRINTHEX2
	ld	a,' '
	call	CHPUT
	pop	hl
	inc	hl
	djnz	RD_SETUP_LOOP
	ld	hl,CRLF_S
	call	PRINT2

	;* Ensure it's a standard request

	ld	a,(BUFFER)	;bmRequestType
	ld	b,a
	and	60h
	ld	hl,UNSUP_SETUP_S
	jp	nz,PRSTALL

	;* Check if it's IN our OUT request

	bit	7,b
	jr	z,HANDLE_SETUP_OUT

	;* IN request: get data to send, then send 1st 8 bytes

HANDLE_SETUP_IN:
	call	GET_DATA_TO_SEND
	ex	de,hl
	ld	a,b
	or	a
	ld	hl,UNSUP_SETUP_S
	jp	z,PRSTALL

	ld	(SEND_PNT),de
	ld	(BYTES_LEFT),a
	jp	WRITE_EP0_DATA

	;* OUT request: act accordingly

HANDLE_SETUP_OUT:
	xor	a
	ld	(BYTES_LEFT),a
	ld	a,SET_ENDP3_T0
	out	(CH_COMMAND_PORT),a
	xor	a
	out	(CH_DATA_PORT),a

	ld	a,(BUFFER+1)	;bRequest
	cp	R_SET_ADDRESS
	jp	z,HANDLE_SET_ADDRESS
	cp	R_SET_CONFIG
	jp	z,HANDLE_SET_CONFIG
	ld	hl,UNSUP_SETUP_S
	jp	PRSTALL

	ret


	;--- Handle SET_ADDRESS request

HANDLE_SET_ADDRESS:
	ld	hl,SET_ADDRESS_S
	call	PRINT
	ld	a,(BUFFER+2)	;low(wValue)
	ld	(ADDRESS_TO_SET),a
	call	PRINTHEX
	ld	hl,CRLF_S
	call	PRINT

	if	NOOB_SETS_ADD=1

	ld	a,255
	out	(CH_COMMAND_PORT),a
	ld	a,(ADDRESS_TO_SET)
	out	(CH_DATA_PORT),a
	xor	a
	ld	(ADDRESS_TO_SET),a

	endif

	ret


	;--- Handle SET_CONFIGURATION request

HANDLE_SET_CONFIG:
	ld	hl,SET_CONFIG_S
	call	PRINT
	ld	a,(BUFFER+2)	;low(wValue)
	ld	(CURRENT_CONFIG),a
	call	PRINTHEX
	ld	hl,CRLF_S
	call	PRINT
	ret


	;--- Write data to endpoint 0,
	;    depending on SEND_PNT and BYTES_LEFT
	;    and updating both

WRITE_EP0_DATA:
	ld	hl,WRITING_S
	call	PRINT
	ld	a,(BYTES_LEFT)
	cp	8
	jr	c,WREP0GO
	ld	a,8
WREP0GO:
	push	af	;A = Bytes to send now
	call	PRINTHEX
	ld	hl,BYTES_S
	call	PRINT
	ld	hl,TWOSPACES_S
	call	PRINT
	pop	bc
	ld	hl,SEND_PNT

	ld	a,WR_USB_DATA3_0
	out	(CH_COMMAND_PORT),a
	ld	a,b
	out	(CH_DATA_PORT),a

	or	a
	ret	z	;Nothing to send actually?

	push	bc
	ld	hl,(SEND_PNT)
	ld	c,CH_DATA_PORT

WREP0LOOP:
	ld	a,(hl)
	push	af
	call	PRINTHEX
	ld	a,' '
	call	CHPUT
	pop	af
	out	(c),a
	inc	hl
	djnz	WREP0LOOP

	ld	(SEND_PNT),hl

	ld	hl,CRLF_S
	call	PRINT

	pop	bc	;B = Bytes sent
	ld	a,(BYTES_LEFT)
	sub	b
	ld	(BYTES_LEFT),a

	ret


	;--- Get data to send depending on request
	;    Input:  Request in BUFFER
	;    Output: HL = Response address
	;            B  = Response length, 0 if unsupported

GET_DATA_TO_SEND:
	ld	a,(BUFFER+1)	;bRequest

	cp	R_GET_INTERF
	ld	hl,ONE_BYE
	ld	b,1
	ret	z

	cp	R_GET_CONFIG
	ld	hl,CURRENT_CONFIG
	ld	b,1
	ret	z

	cp	R_GET_STATUS
	ld	hl,ZERO_BYTES
	ld	b,2
	ret	z

	cp	R_GET_DESCR
	ld	b,0
	ret	nz

	ld	a,(BUFFER+3)	;high(wValue), descriptor type

	cp	USBD_DEVICE
	ld	hl,DEV_DESC_START
	ld	b,DEV_DESC_SIZE
	ret	z

	cp	USBD_CONFIG
	ld	hl,CONF_DESC_START
	ld	b,CONF_DESC_SIZE
	ret	z

	cp	USBD_STRING
	ld	b,0
	ret	nz

	ld	a,(BUFFER+2)	;low(wValue), descriptor index

	or	a
	ld	hl,LANG_DESC_START
	ld	b,LANG_DESC_SIZE
	ret	z

	cp	STRD_MANUF
	ld	hl,MANUF_DESC_START
	ld	b,MANUF_DESC_SIZE
	ret	z

	cp	STRD_PRODUCT
	ld	hl,PROD_DESC_START
	ld	b,PROD_DESC_SIZE
	ret	z

	cp	STRD_SERIAL
	ld	hl,SERIAL_DESC_START
	ld	b,SERIAL_DESC_SIZE
	ret	z

	ld	b,0
	ret


	;--- Handle endpoint 0 IN token received interrupt

HANDLE_EP0_IN:
	ld	hl,EP0IN_S
	call	PRINT

	ld	a,(ADDRESS_TO_SET)
	or	a
	jr	z,EP0INGO

	ld	hl,SETTINGAD_S
	;call    PRINT
	ld	a,SET_USB_ADDR
	out	(CH_COMMAND_PORT),a
	ld	a,(ADDRESS_TO_SET)
	out	(CH_DATA_PORT),a
	xor	a
	ld	(ADDRESS_TO_SET),a
	ret

EP0INGO:
	ld	a,(BYTES_LEFT)
	or	a
	call	nz,WRITE_EP0_DATA
	ret


	;--- Handle endpoint 0 OUT token received interrupt

HANDLE_EP0_OUT:
	ld	a,RD_USB_DATA0
	out	(CH_COMMAND_PORT),a
	in	a,(CH_DATA_PORT)
	ld	a,SET_ENDP2_R0
	out	(CH_COMMAND_PORT),a
	xor	a
	out	(CH_DATA_PORT),a
	ld	hl,EP0OUT_S
	call	PRINT
	ret


	;--- Print message passed in HL, then STALL endpoint 0

PRSTALL:
	call	PRINT


	;--- STALL endpoint 0

STALL_EP0:
	ld	a,WR_USB_DATA3_0
	out	(CH_COMMAND_PORT),a
	ld	a,0Fh
	out	(CH_DATA_PORT),a
	ret



;****************
;* MSX specific *
;****************

	if	ROM=0

;--- Print string passed in HL, then exit

PREXIT:
	call	PRINT

;--- Exit

EXIT:
	ld	c,TERM0
	jp	5

	endif

	if	ROM=1

;--- Print string passed in HL, then halt

PREXIT:
	call	PRINT
PREXLP:
	ld	a,8
	call	DO_SNSMAT
	inc	a
	jr	z,PREXLP

;--- Exit

EXIT:
	ld	hl,(SAVE_SP)
	ld	sp,hl
	ret

	endif


;--- Read keyboard matrix row A

DO_SNSMAT:
	ld	c,a
	di
	in	a,(0AAh)
	and	0F0h
	add	c
	out	(0AAh),a
	ei
	in	a,(0A9h)
	ret

;row 8:	right  down     up  left    DEL   INS    HOME  SPACE


;--- Print zero-terminated string passed in HL

PRINT:
	ret
PRINT2:
	ld	a,(hl)
	or	a
	ret	z
	call	CHPUT
	di
	inc	hl
	jr	PRINT2


;--- Print byte passed in A in hex

PRINTHEX:
	ret
PRINTHEX2:
	push	af
	call	_PRINTHEX_1
	pop	af
	push	af
	call	_PRINTHEX_2
	pop	af
	ret

_PRINTHEX_1:
	rra
	rra
	rra
	rra
_PRINTHEX_2:
	or	0F0h
	daa
	add	a,0A0h
	adc	a,40h

	call	CHPUT
	ret


	if	ROM=0

;--- Print character passed in A

CHPUT:
	push	hl
	push	bc
	push	de
	ld	e,a
	ld	c,CONOUT
	call	DOS
	pop	de
	pop	bc
	pop	hl
	ret

	endif


;*****************
;* Debug strings *
;*****************

INIT_OK_S:
	db	"CH372 Init ok!"
CRLF_S:
	db	13,10,0

NO_CH_S:
	db	"*** CH732 not found",13,10,0

CH_MODE_ERR_S:
	db	"*** Error setting USB mode",13,10,0

UNK_INT_S:
	db	"*** Unknown interrupt received: ",0

BUSRESET_S:
	db	"BUS_RESET",13,10,0

SUSPEND_S:
	db	"SUSPEND",13,10,0

WAKEUP_S:
	db	"WAKEUP",13,10,0

SETUP_S:
	db	"SETUP",13,10,0

EP0IN_S:
	db	"EP0_IN",13,10,0

EP0OUT_S:
	db	"EP0_OUT",13,10,0

WRITING_S:
	db	"  Writing ",0

BYTES_S:
	db	" bytes",13,10,0

BAD_SETUP_S:
	db	"  *** Wrong SETUP data length",13,10,0

UNSUP_SETUP_S:
	db	"  *** Unsupported control request",13,10,0

SET_ADDRESS_S:
	db	"  SET_ADDRESS: ",0

SET_CONFIG_S:
	db	"  SET_CONFIGURATION: ",0

SETTINGAD_S:
	db	"  Setting address",13,10,0

TWOSPACES_S:
	db	"  ",0


;*******************
;* USB Descriptors *
;*******************

DEV_DESC_START:
	db	12h	;Length
	db	USBD_DEVICE
	db	00h,02h	;USB version,
	db	00h,00h,00h	;Class,subclass,protocol
	db	8	;Max packet size for EP0
	db	09h,12h	;VID (https://pid.codes)
	db	07h,00h	;PID (testing)
	db	00h,01h	;Device release number
	db	STRD_MANUF	;Manufacturer string id
	db	STRD_PRODUCT	;Product string id
	db	STRD_SERIAL	;Serial number string id
	db	01h	;Number of configurations
DEV_DESC_END:
DEV_DESC_SIZE:	equ	DEV_DESC_END-DEV_DESC_START

CONF_DESC_START:
	db	09h	;Length
	db	USBD_CONFIG
	db	12h,00h	;Total length
	db	01h	;Number of interfaces
	db	01h	;Configuration value
	db	00h	;String descriptor for configuration
	db	80h	;Attributes (no self-poweredh,no remote wake-up)
	db	34h	;Max power (68mA)

	;Interface descriptor

	db	09h	;Length
	db	USBD_INTERF
	db	00h	;Interface number
	db	00h	;Alternate setting
	db	00h	;Number of endpoints
	db	0FFh,0FFh,0FFh	;Class,subclass,protocol
	db	00h	;String descriptor for interface
CONF_DESC_END:
CONF_DESC_SIZE:	equ	CONF_DESC_END-CONF_DESC_START

LANG_DESC_START:
	db	04h	;Length
	db	USBD_STRING
	db	09h,04h	;English (US)
LANG_DESC_END:
LANG_DESC_SIZE:	equ	LANG_DESC_END-LANG_DESC_START

PROD_DESC_START:
	db	1Ah	;Length
	db	USBD_STRING
	;"NestorDevice"
	db	4Eh,00h,65h,00h,73h,00h,74h,00h,6Fh,00h
	db	72h,00h,44h,00h,65h,00h,76h,00h,69h,00h
	db	63h,00h,65h,00h
PROD_DESC_END:
PROD_DESC_SIZE:	equ	PROD_DESC_END-PROD_DESC_START

MANUF_DESC_START:
	db	2Ah	;Length
	db	USBD_STRING
	;"Konamiman Industries"
	db	4Bh,00h,6Fh,00h,6Eh,00h,61h,00h,6Dh,00h
	db	69h,00h,6Dh,00h,61h,00h,6Eh,00h,20h,00h
	db	49h,00h,6Eh,00h,64h,00h,75h,00h,73h,00h
	db	74h,00h,72h,00h,69h,00h,65h,00h,73h,00h
MANUF_DESC_END:
MANUF_DESC_SIZE:	equ	MANUF_DESC_END-MANUF_DESC_START

SERIAL_DESC_START:
	db	06h	;Length
	db	USBD_STRING
	db	33h,00h,34h,00h
SERIAL_DESC_END:
SERIAL_DESC_SIZE:	equ	SERIAL_DESC_END-SERIAL_DESC_START

ONE_BYE:	db	1
ZERO_BYTES:	db	0,0


;*************
;* Variables *
;*************

	if	ROM=1

VAR_START:	equ	8012h

	endif

	if	ROM=0

VAR_START:

	endif

;0: Not handling request, 1: handling IN request, 2: handling OUT request
CUR_REQ_TYPE:	equ	VAR_START

;How many bytes left to send
BYTES_LEFT:	equ	CUR_REQ_TYPE+1

;Pointer to data being sent
SEND_PNT:	equ	BYTES_LEFT+1

;If not 0, address to set in next IN token receive
ADDRESS_TO_SET:	equ	SEND_PNT+2

;Current config number
CURRENT_CONFIG:	equ	ADDRESS_TO_SET+1

VAR_END:	equ	CURRENT_CONFIG+1

BUFFER:	equ	VAR_END
