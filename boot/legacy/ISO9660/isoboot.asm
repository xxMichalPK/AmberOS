; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

; REMEMER:
;   In ISO 9660 the LBA size is 2Kib (2048B) instead of 512B!

; Macro for faster include
%macro RMODE_INCLUDE 1
	%define INC_DIR '../../../include/boot/legacy/rmode/'
	%define INC_FILE %1
	%strcat INC_PATH INC_DIR, INC_FILE
	%include INC_PATH
%endmacro


%define PVD_ADDR 0x1000
%define ROOT_DIRECTORY_ENTRY PVD_ADDR + 156

%define ROOT_DIRECTORY_ADDRESS PVD_ADDR + 0x800
[BITS 16]
[ORG 0x7C00]
; Since this is a ISO 9660 bootloader, make space for the Boot Information Table
jmp short preboot
times 8 - ($-$$) db 0

BIT_PRIMARY_VOLUME_DESCRIPTOR_LBA 	dd 0x00
BIT_BOOT_FILE_LBA					dd 0x00
BIT_BOOT_FILE_LENGTH				dd 0x00
BIT_CHECKSUM						dd 0x00
BIT_RESERVED						times 40 db 0x00

preboot:
	mov [bootDrive], dl                 ; Set boot Drive

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
    ; Change the 
    cmp dh, 'M'
    jne .start
    mov BYTE [LBA_MULTIPLIER], 2
    mov BYTE [SIZE_DIVIDER], 9

    .start:
	mov ah, 0x00                    ; AH - 0 (change video mode)
	mov al, 0x03                    ; AL - 3 (80x25 text mode)
	int 0x10                        ; Set the mode

	mov si, startMSG
	call print16

	call Check_Ext

	; Now when we know the disk extensions are supported
	; We need to parse the ISO 9660 Primary Volume Descriptor (PVD)
	mov eax, [BIT_PRIMARY_VOLUME_DESCRIPTOR_LBA]	; Get the PVD address from the Boot Information Table
    mov cl, [LBA_MULTIPLIER]
    shl eax, cl
	mov DWORD [dap_lba_lo], eax             ; Set the LBA address
    mov eax, 2048
    mov cl, [SIZE_DIVIDER]
    shr eax, cl
	mov WORD [dap_sector_count], ax         ; Load 1, 2KiB Long sector (The entire PVD)
	mov WORD [dap_segment], 0               ; Read to current segment (0)
	mov WORD [dap_offset], PVD_ADDR         ; Read to address defined as PVD_ADDR
	call LBA_Read                           ; Call the function to read the disk

    ; Read the root directory
    mov eax, DWORD [ROOT_DIRECTORY_ENTRY + 2]   ; Root directory LBA address
    mov cl, [LBA_MULTIPLIER]
    shl eax, cl
    mov DWORD [dap_lba_lo], eax             ; Set the LBA address
    mov eax, DWORD [ROOT_DIRECTORY_ENTRY + 10]  ; Root directory size (in Bytes), convert it to number of sectors
    push eax                                ; Save the original size
    mov cl, [SIZE_DIVIDER]
    shr eax, cl
    push ax                                 ; Save this for later
    mov WORD [dap_sector_count], ax         ; The sector size is now in ax
    mov WORD [dap_segment], 0               ; Read to current segment (0)
	mov WORD [dap_offset], ROOT_DIRECTORY_ADDRESS   ; Read to address defined as PVD_ADDR
	call LBA_Read                           ; Call the function to read the disk

    ; Load the 'boot' directory
    ; Get the size of root directory and round up to the next 2048B
    pop ax
    xor edx, edx
    mov bx, 2048
    mul bx
    ; Load the boot directory right after the root directory
    mov ebx, ROOT_DIRECTORY_ADDRESS
    add ebx, eax
    pop eax
    push ebx                                ; Save the boot directory address
    mov ecx, eax                            ; The original size of the directory
    mov si, bootDirName
    mov dl, [bootDirName.len]
    mov eax, ROOT_DIRECTORY_ADDRESS
    call loadISODirectoryEntry              ; It returns the size of loaded data (0 if failed)
    cmp ax, 0
    je loadFailed

    ; Load the 2'nd stage to 0x10000 (cs: 0x1000, ip: 0x0000)
    mov ecx, eax
    pop ebx
    mov eax, ebx
    mov bx, 0x1000
    mov fs, bx
    mov ebx, 0x0000
    mov si, loaderName
    mov dl, [loaderName.len]
    call loadISODirectoryEntry              ; It returns the size of loaded data (0 if failed)
    cmp ax, 0
    je loadFailed

    ; Now we can jump to the C code in bootldr.bin file!
    ; But first change to 32bit mode
    call EnableA20
    call LoadGDT
    ; Enable protected mode
    cli
    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp CODE32_SEG:JumpToLoader

	jmp $

loadFailed:
    mov si, loadFailedMessage
    call print16
    jmp $

RMODE_INCLUDE 'disk.inc'
RMODE_INCLUDE 'print.inc'
RMODE_INCLUDE 'strncmp.inc'
RMODE_INCLUDE 'ISO9660.inc'
RMODE_INCLUDE 'gdt.inc'
RMODE_INCLUDE 'a20.inc'

bootDirName: db "boot"
    .len: db 4
loaderName: db "amberldr.bin"
    .len: db 12

LBA_MULTIPLIER: db 0
SIZE_DIVIDER:   db 11

startMSG: db "AmberOS v0.1a (Tyro) Live", 0x0A, 0x0D
		  db "(C)Copyright Michal Pazurek 2023", 0x0A, 0x0D, 0x0A, 0x0D
		  db "Loading AmberOS...", 0x0A, 0x0D, 0x00

loadFailedMessage: db 0x0A, 0x0D, "Couldn't load one of the core system components!", 0x0A, 0x0D
                   db "Please reboot your PC...", 0x0A, 0x0D, 0x00


[BITS 32]
JumpToLoader:
    mov ax, DATA32_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x80000    ; Set stack above loader and below the EBDA
    mov esp, ebp        ; Set the stack
    
    ; Send the boot drive number to the loader
    xor edx, edx
    mov dl, [bootDrive]
    push edx
    jmp 0x10000         ; Jump to the loader

    jmp $

; REMEMBER:
;   The maximum size of the iso bootloader can not exceed 0x8400b ~ 33KiB
;   If the size of 2KiB is not sufficient, extend it in the makefile:
;   -boot-load-size (next multiple of 4) and add 2KiB to the times command
times 2048 - ($-$$) db 0
