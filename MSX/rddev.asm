; USB keyboard reporter for MSX using a CH372/5/6
; By Konamiman, 7/2021
;
; Configures the CH372 in internal firmware mode and checks
; the keyboard status continuously, if there's any change
; it sends two bytes (row number + row data) to interrupt
; endpoint 81h.
;
; A circular queue is used in case keyboard status changes 
; happen faster than the USB host reads them from the endpoint.


;*************
;* Constants *
;*************

;* Configuration

;Device identification
;NOTE! If you change these, update INFO_S too
VID_LOW:	equ	09h
VID_HIGH:	equ	12h
PID_LOW:	equ	07h
PID_HIGH:	equ	00h

;0: Generate .BIN file
;1: Generate ROM file
ROM:	equ	1

;How many keyboard rows to scan
ROWS_COUNT:	equ	12

;1: Print bytes sent as well as SUSPEND and WAKEUP eventts
DEBUG_LOG:	equ	0


;* MSX BIOS and work area

INITXT:	equ	006Ch
CHGET:	equ	009Fh
CHPUT:	equ	00A2h
ERAFNK:	equ	00CCh
SNSMAT:	equ	0141h
KILBUF:	equ	0156h

LINL40:	equ	0F3AEh


;* Z80 ports where the CH372 ports are mapped

CH_DATA_PORT:	equ	20h
CH_COMMAND_PORT:	equ	21h


;* CH372 commands

GET_IC_VER:	equ	01h
ENTER_SLEEP:	equ	03h
RESET_ALL:	equ	05h
CHECK_EXIST:	equ	06h
CHK_SUSPEND:	equ	0Bh
SET_USB_ID:	equ	12h
SET_USB_MODE:	equ	15h
GET_STATUS:	equ	22h
UNLOCK_USB:	equ	23h
RD_USB_DATA0:	equ	27h
RD_USB_DATA:	equ	28h
WR_USB_DATA5_1:	equ	2Ah
WR_USB_DATA7_2:	equ	2Bh


;* CH372 operation status

CMD_RET_SUCCESS:	equ	51h
CMD_RET_ABORT:	equ	5Fh


;* CH372 interruption status

INT_EP1_OUT:	equ	01h
INT_EP1_IN:	equ	09h
INT_EP2_OUT:	equ	02h
INT_EP2_IN:	equ	0Ah
INT_USB_SUSP:	equ	05h
INT_WAKE_UP:	equ	06h


;*************************
;* Startup and main loop *
;*************************

	if	ROM=1

	org	4000h
	db	41h,42h
	dw	PROG_START
	ds	12

	else

	org	0C010h-7
	db	0FEh
	dw	PROG_START
	dw	PROG_END
	dw	PROG_START

	endif

PROG_START:
	ld	hl,0
	add	hl,sp
	ld	(SAVE_SP),hl

	di
	call	CH_INIT

	ld	a,40
	ld	(LINL40),a
	call	INITXT
	call	ERAFNK

	ld	hl,INFO_S
	call	PRINT

LOOP:
	ld	a,(OLD_KEYS+7)
	and	10010100b
	jp	z,EXIT	;ENTER+STOP+ESC pressed

	in	a,(CH_COMMAND_PORT)
	and	80h
	call	z,HANDLE_CH_INT

	;* Check keys, if there are changes send them

	ld	hl,OLD_KEYS
	ld	de,NEW_KEYS
	ld	b,0
SNSLOOP:
	ld	a,b
	call	SNSMAT
	cp	(hl)
	jr	z,SNSLOOP_END

	ld	(de),a
	ld	c,a

	if	DEBUG_LOG = 1
	push	bc
	ld	a,b
	call	PRINTHEX
	ld	a,' '
	call	CHPUT
	ld	a,c
	call	PRINTHEX
	push	hl
	push	de
	ld	hl,CRLF_S
	call	PRINT
	pop	de
	pop	hl
	pop	bc
	endif

	ld	a,b
	call	QUEUE_WR
	ld	a,c
	call	QUEUE_WR

SNSLOOP_END:
	inc	hl
	inc	de
	inc	b
	ld	a,b
	cp	ROWS_COUNT
	jr	c,SNSLOOP

	ld	hl,NEW_KEYS
	ld	de,OLD_KEYS
	ld	bc,ROWS_COUNT
	ldir

	call	SEND_EP1_DATA

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
	di

	ld	a,SET_USB_ID
	out	(CH_COMMAND_PORT),a
	ld	a,VID_LOW
	out	(CH_DATA_PORT),a
	ld	a,VID_HIGH
	out	(CH_DATA_PORT),a
	ld	a,PID_LOW
	out	(CH_DATA_PORT),a
	ld	a,PID_HIGH
	out	(CH_DATA_PORT),a

	ld	a,SET_USB_MODE
	out	(CH_COMMAND_PORT),a
	xor	a	;Invalid device mode
	out	(CH_DATA_PORT),a
	ei
	halt
	halt
	di
	in	a,(CH_DATA_PORT)
	cp	CMD_RET_SUCCESS
	ld	hl,CH_MODE_ERR_S
	jp	nz,PREXIT

	ld	a,SET_USB_MODE
	out	(CH_COMMAND_PORT),a
	ld	a,2	;Internal firmware mode
	out	(CH_DATA_PORT),a
	ei
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

	call	CLEAR_VARS

	ret


CLEAR_VARS:
	ld	a,0FFh
	ld	(OLD_KEYS),a
	ld	hl,OLD_KEYS
	ld	de,OLD_KEYS+1
	ld	bc,ROWS_COUNT-1
	ldir
	ld	hl,OLD_KEYS
	ld	de,NEW_KEYS
	ld	bc,ROWS_COUNT
	ldir
	call	QUEUE_INIT
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

	cp	INT_WAKE_UP
	jp	z,HANDLE_WAKEUP
	cp	INT_EP1_IN
	jp	z,HANDLE_EP1_IN

	if	DEBUG_LOG = 1

	ld	hl,UNK_INT_S
	push	af
	call	PRINT
	pop	af
	call	PRINTHEX
	ld	hl,CRLF_S
	call	PRINT

	endif

	ret

DO_UNLOCK:
	ld	a,UNLOCK_USB
	out	(CH_COMMAND_PORT),a
	ret


;--- Handle SUSPEND, WAKE UP interrupts

HANDLE_SUSPEND:
	ld	a,ENTER_SLEEP
	out	(CH_COMMAND_PORT),a

	if	DEBUG_LOG=1
	ld	hl,SUSPEND_S
	call	PRINT
	endif

	ret

HANDLE_WAKEUP:
	if	DEBUG_LOG=1
	ld	hl,WAKEUP_S
	call	PRINT
	endif
	ret


;--- Handle EP1 IN token received interrupt

HANDLE_EP1_IN:
	call	SEND_EP1_DATA
	ret


;--- Send interrupt endpoint data

SEND_EP1_DATA:
	ld	a,(QUEUE_LEN)
	or	a
	ret	z

	cp	8+1
	jr	c,SEND_EP1_DATA2
	ld	a,8
SEND_EP1_DATA2:
	ld	b,a	;B = amount of bytes to send

	ld	a,WR_USB_DATA5_1
	out	(CH_COMMAND_PORT),a
	ld	a,b
	out	(CH_DATA_PORT),a

	ld	c,CH_DATA_PORT
SEND_EP1_LOOP:
	call	QUEUE_RD
	out	(c),a
	djnz	SEND_EP1_LOOP

	ret


;****************
;* MSX specific *
;****************

;--- Print string passed in HL, then exit

PREXIT:
	call	PRINT

	if	ROM=1
	ld	hl,PRESSK_S
	call	PRINT
	call	CHGET
	endif


;--- Exit program

EXIT:
	ld	a,SET_USB_MODE
	out	(CH_COMMAND_PORT),a
	xor	a	;Invalid device mode
	out	(CH_DATA_PORT),a

	call	KILBUF

	ld	hl,(SAVE_SP)
	ld	sp,hl
	ret


;--- Print zero-terminated string passed in HL

PRINT:
	ld	a,(hl)
	or	a
	ret	z
	call	CHPUT
	di
	inc	hl
	jr	PRINT


	if	DEBUG_LOG = 1

;--- Print byte passed in A in hex

PRINTHEX:
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

	endif


;******************
;* Circular queue *
;******************

;Queue size is 256 bytes (actually only 254 are used)

	;--- Initialize queue

QUEUE_INIT:
	ld	hl,QUEUE
	ld	(QUEUE_RDPNT),hl
	ld	(QUEUE_WRPNT),hl
	xor	a
	ld	(QUEUE_LEN),a
	ret


	;--- Write one byte to the queue, except if it's full
	;    Input: A = byte
	;    Output: Cy = 1 if queue is full

QUEUE_WR:
	push	hl
	push	bc
	ld	b,a
	ld	a,(QUEUE_LEN)
	inc	a
	cp	255
	scf
	jr	z,QUEUE_WR_END
	ld	(QUEUE_LEN),a
	ld	hl,(QUEUE_WRPNT)
	ld	(hl),b
	inc	l
	ld	(QUEUE_WRPNT),hl
	or	a
QUEUE_WR_END:
	pop	bc
	pop	hl
	ret


	;--- Read one byte from the queue
	;    Output: A = byte
	;            Cy = 1 if queue is empty

QUEUE_RD:
	push	hl
	ld	a,(QUEUE_LEN)
	or	a
	scf
	jr	z,QUEUE_RD_END
	dec	a
	ld	(QUEUE_LEN),a
	ld	hl,(QUEUE_RDPNT)
	ld	a,(hl)
	inc	l
	ld	(QUEUE_RDPNT),hl
	or	a
QUEUE_RD_END:
	pop	hl
DO_RET:
	ret


;***********
;* Strings *
;***********

INFO_S:
	db	"I'm an USB device, VID=1209h, PID=0007h",13,10
	db	13,10
	db	"I'll send changes in keyboard status",13,10
	db	"to endpoint 81h (max length: 8 bytes)",13,10
	db	"as byte pairs: row number, row data",13,10
	db	13,10
	db	"Exit: ESC + ENTER + STOP",13,10
	if	ROM=0
	db	"      (press STOP last)",13,10
	endif
	db	27,120,53	;Hide the cursor
	db	0

NO_CH_S:
	db	"*** CH732 not found"
CRLF_S:
	db	13,10,0

CH_MODE_ERR_S:
	db	"*** Error setting USB mode",13,10,0

UNK_INT_S:
	db	"*** Unknown interrupt received: ",0

	if	ROM = 1

PRESSK_S:
	db	13,10,"Press any key to exit ",0

	endif

	if	DEBUG_LOG = 1

SUSPEND_S:
	db	"SUSPEND",13,10,0

WAKEUP_S:
	db	"WAKEUP",13,10,0

	endif

PROG_END:


;*************
;* Variables *
;*************

	if	ROM=1
VAR_START:	equ	0C400h
	else
VAR_START:
	endif

;Stack pointer at program start time,
;used to restore it at exit time
SAVE_SP:	equ	VAR_START

;Previous state of keyboard, one byte per row
OLD_KEYS:	equ	SAVE_SP+2

;Current state of keyboard, one byte per row
NEW_KEYS:	equ	OLD_KEYS+ROWS_COUNT

;Circular queue address, pointers and length
QUEUE_WRPNT:	equ	NEW_KEYS+ROWS_COUNT
QUEUE_RDPNT:	equ	QUEUE_WRPNT+2
QUEUE_LEN:	equ	QUEUE_RDPNT+2
QUEUE:	equ	0C500h

	if	ROM=1
	;Force ROM size to be 16K to make emulators and flash loaders happy
	ds	8000h-$,0FFh
	endif
