; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

; REMEMER:
;   In ISO 9660 the LBA size is 2Kib (2048B) instead of 512B!

%define PVD_ADDR 0x1000
%define PATH_TABLE_SIZE_OFFSET 132		; The offset of the low endian path table size in the PVD
%define PATH_TABLE_OFFSET 140			; The offset of the low endian path table LBA address in the PVD

%define PATH_TABLE_ADDR PVD_ADDR + 0x800; Load right after the PVD

%define PT_ENTRY_NAME_LEN_OFFSET 0
%define PT_ENTRY_EARL_OFFSET 1
%define PT_ENTRY_LBA_OFFSET 2
%define PT_ENTRY_PARENT_DIR_NUM_OFFSET 6
%define PT_ENTRY_NAME_OFFSET 8

; Macro for faster include
%macro RMODE_INCLUDE 1
	%define INC_DIR '../../../include/boot/legacy/rmode/'
	%define INC_FILE %1
	%strcat INC_PATH INC_DIR, INC_FILE
	%include INC_PATH
%endmacro

[BITS 16]
[ORG 0x7C00]
; Since this is a ISO 9660 bootloader, make space for the Boot Information Table
jmp short preboot
times 8 - ($-$$) db 0

BIT_PRIMARY_VOLUME_DESCRIPTOR_LBA 	resd 1
BIT_BOOT_FILE_LBA					resd 1
BIT_BOOT_FILE_LENGTH				resd 1
BIT_CHECKSUM						resd 1
BIT_RESERVED						resb 40

preboot:
	mov [bootDrive], dl                 ; Set boot Drive
	mov [partition], si                 ; Set the current partition

	cli                                 ; Clear interrupts to avoid interrupts while 
										; changing important registers and setting stack
	xor ax, ax                          ; Zero out ax register
	mov ds, ax                          ; ds = 0
	mov es, ax                          ; es = 0
	mov ss, ax                          ; ss = 0
	mov sp, 0x7c00                      ; stack pointer sp = 0x7c00 - our load address
										; stack grows down from the specified address
	push ax                             ; set code segment cs = 0
	push boot                           ; in case some BIOS sets us to 0x7c0:0000 instead of 0000:0x7c00
	retf                                ; Far return to next instruction to ensure a normal CS:IP

boot:
	mov ah, 0x00                    ; AH - 0 (change video mode)
	mov al, 0x03                    ; AL - 3 (80x25 text mode)
	int 0x10                        ; Set the mode

	mov si, startMSG
	call print16

	call Check_Ext

	; Now when we know the disk extensions are supported
	; We need to parse the ISO 9660 Primary Volume Descriptor (PVD)
	mov eax, [BIT_PRIMARY_VOLUME_DESCRIPTOR_LBA]	; Get the PVD address from the Boot Information Table
	mov DWORD [dap_lba_lo], eax             ; Set the LBA address
	mov WORD [dap_sector_count], 1          ; Load 1 2KiB Long sector (The entire PVD)
	mov WORD [dap_segment], 0               ; Read to current segment (0)
	mov WORD [dap_offset], PVD_ADDR         ; Read to address defined as PVD_ADDR
	call LBA_Read                           ; Call the function to read the disk

	; Read the size of the Path Table into the variable to save it for later use
	mov eax, DWORD [PVD_ADDR + PATH_TABLE_SIZE_OFFSET]	; Path Table size !IN BYTES!
	;   Convert it to number of sectors
	xor edx, edx
	mov ebx, 2048							; !!! The sectors on an ISO drive are 2048 Bytes in size !!!
	div ebx
	inc ax									; The minimum size is 1 sector
	mov [PT_Size], ax

	; Read the address of the Path Table LBA address and save it for later use
	mov eax, DWORD [PVD_ADDR + PATH_TABLE_OFFSET]	; Read the LBA address of the Path Table to eax
	mov [PT_LBA], eax

	; Read the Path Table
	mov eax, [PT_LBA]
	mov DWORD [dap_lba_lo], eax             ; Set the LBA address
	mov cx, [PT_Size]
	mov WORD [dap_sector_count], cx			; Load the number of sectors used by the Path Table
	mov WORD [dap_segment], 0               ; Read to current segment (0)
	mov WORD [dap_offset], PATH_TABLE_ADDR  ; Read to address defined as PATH_TABLE_ADDR
	call LBA_Read                           ; Call the function to read the disk

	; Read the Path Table and locate the boot directory
	mov eax, PATH_TABLE_ADDR
	locateBootDir:	
		cmp	BYTE [eax], 4
		je .cmpName

		push eax
		xor edx, edx
		xor ebx, ebx
		mov bl, BYTE [eax]
		xchg eax, ebx
		mov ebx, 2
		div ebx
		cmp edx, 0
		jne .addOne

		pop eax
		xor ebx, ebx
		mov bl, BYTE [eax]
		add eax, ebx
		add eax, 8
		jmp locateBootDir

		.addOne:
			pop eax
			xor ebx, ebx
			mov bl, BYTE [eax]
			add eax, ebx
			add eax, 9
			jmp locateBootDir


		.cmpName:
			push eax
			add eax, PT_ENTRY_NAME_OFFSET
			mov si, ax
			mov di, bootDirName
			mov cx, 4
			call strncmp16
			cmp ax, 1
			je dirFound

			pop eax
			add eax, 12
			jmp locateBootDir

			
	dirFound:
	pop eax

	mov eax, DWORD [eax + PT_ENTRY_LBA_OFFSET]
	mov DWORD [dap_lba_lo], eax             ; Set the LBA address
	mov cx, [PT_Size]
	mov WORD [dap_sector_count], 1			; Load 1 sector of the directory
	mov WORD [dap_segment], 0               ; Read to current segment (0)
	mov WORD [dap_offset], 0x5000			; Read destination
	call LBA_Read                           ; Call the function to read the disk

	mov ah, 0x0E
	mov si, 0x5000
	mov cx, 512
	.lo:
		lodsb
		cmp cx, 0
		je .done
		int 0x10
		dec cx
		jmp .lo

		.done:


	jmp $

RMODE_INCLUDE 'disk.inc'
RMODE_INCLUDE 'print.inc'
RMODE_INCLUDE 'strncmp.inc'

bootDirName: db "boot"

startMSG: db "AmberOS v0.1a (Tyro) Installer", 0x0A, 0x0D
		  db "(C)Copyright Michal Pazurek 2023", 0x0A, 0x0D, 0x0A, 0x0D
		  db "Loading Installer...", 0x0A, 0x0D, 0x00

PT_LBA dd 0x00000000
PT_Size dw 0x0000							; Page Table size !IN SECTORS!

; REMEMBER:
;   The maximum size of the iso bootloader can not exceed 0x8400b ~ 33KiB
times 2048 - ($-$$) db 0
