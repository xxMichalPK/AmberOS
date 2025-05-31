; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]                           ; Ensure we are in 16-bit real mode

; Check if our disk supports int 13h extensions for reading the disk
Check_Ext:
	xor ax, ax						    ; Zero AX
	mov BYTE [EXT13h], al			    ; Zero out flag
    mov dl, BYTE [bootDrive]	        ; DL = Disk Number
	mov ah, 0x41					    ; INT 13h AH=41h: Check Extensions Present
	mov bx, 0x55AA					    ; Magic value(?)
	int 0x13						    ; Check extensions
	jc .done						    ; Carry flag set? No extensions
	test cx, 1						    ; Does it say we can use packet addressing?
	jz .done						    ; Nope, No extensions
	mov ax, 0xFF					    ; If we reached this point extensions are usable
	mov BYTE [EXT13h], al			    ; Set flag
    
    .done:
        mov dl, BYTE [EXT13h]           ; Set dl to flag
        or dl, dl                       ; Check if dl is 1 (if ext 13h is supported)
        jz EXTnotSupported              ; If dl = 0 extensions are not supported

        ret								; Else Return

LBA_Read:
	pusha
	mov dl, [bootDrive]
	mov ah, 0x42 ; EXT Read Sectors
	mov si, dap
	int 0x13 ; Read Sectors
	jc .error
	.success:
		popa
		ret
	.error:
		; Carry Flag Set
		mov si, ERR_Read
		call print16
		cli
		hlt
		jmp $ ; Hang Here

EXTnotSupported:
    mov si, ERR_NS
	call print16
	cli
	hlt
    jmp $

;-------------------------------------------------------------------------------
; DISK ADDRESS PACKET
dap:
	db 0x10 ; Size of Packet, 16B
	db 0
dap_sector_count: 		dw 28 ; Number of Sectors to Read
dap_offset:				dw 0x1000
dap_segment:			dw 0
dap_lba_lo:				dd 0
dap_lba_hi:				dd 0
;-------------------------------------------------------------------------------

bootDrive:  db 0
partition:  dw 0
EXT13h:     db 0

ERR_NS: db "Disk extensions are not supported! Halting system...", 0x0A, 0x0D, 0x00
ERR_Read: db "Failed to read the disk! Halting system...", 0x0A, 0x0D, 0x00
