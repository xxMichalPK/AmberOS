; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

; The hybrid ISO protective MBR of the System
; ! Can be used only on disks with int 13h extensions !

; If the BIOS uses Hard Disk emulation the sectors are 512b long \_(-_-)_/
; Define the Boot Signature - M(4D) P(50) in reverse order
%define OS_SIG 0x504D

; Hard code all the ISO values because ??? why not!?
%define ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA 16 * 4
; This is the number of 512b sectors that normally add up to one ISO sector
%define ISO_SECTOR_SIZE 4

%define PVD_ADDR 0x1000
%define ROOT_DIRECTORY_ENTRY PVD_ADDR + 156

%define ROOT_DIRECTORY_ADDRESS PVD_ADDR + 0x800

[BITS 16]                               ; We are in 16-bit real mode
[ORG 0x0600]                            ; Position of this programm in memory
jmp short start
dw OS_SIG                               ; The signature is used by the UEFI loader to get the boot disk

start:
    cli                                 ; We do not want to be interrupted
    xor ax, ax                          ; Set ax to 0
    mov ds, ax                          ; Set Data Segment to 0
    mov es, ax                          ; Set Extra Segment to 0
    mov fs, ax                          ; Set FS register to 0
    mov gs, ax                          ; Set GS register to 0
    mov ss, ax                          ; Set Stack Segment to 0
    mov sp, 0x7c00                      ; Set Stack Pointer to 0x7c00 because we landed here
                                        ; and we want to copy the code from this location
    .CopyLower:
        cld
        mov cx, 0x0100                  ; There are 256 WORDs (512b) in MBR
        mov si, 0x7C00                  ; Current MBR Address
        mov di, 0x0600                  ; New MBR Address
        rep movsw                       ; Copy MBR
    jmp 0x0000:LowStart                 ; Far jump to new Address


; Now we are at address 0x0600 + the offset of this function
LowStart:
    mov [bootDrive], dl                 ; Save BootDrive
    mov bp, sp

    mov BYTE [dap_size], 0x10
    sti                                 ; Enable interrupts

    ; Check if disk supports extended int 13h functions to read the LBA of bootable partition
    call Check_Ext                      ; Call check function

    ; Read the PVD
    mov dword [dap_lba_lo], ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA
    mov word [dap_sector_count], ISO_SECTOR_SIZE
    mov word [dap_offset], PVD_ADDR
    call LBA_Read

    ; Read the root directory
    mov eax, DWORD [ROOT_DIRECTORY_ENTRY + 2]   ; Root directory LBA address
    mov cl, [LBA_MULTIPLIER]
    shl eax, cl
    mov DWORD [dap_lba_lo], eax             ; Set the LBA address
    mov eax, DWORD [ROOT_DIRECTORY_ENTRY + 10]  ; Root directory size (in Bytes), convert it to number of sectors
    push eax                                ; Save the original size
    mov cl, [SIZE_DIVIDER]
    shr eax, cl                             ; Divide by 512 using bit shift. Those are always an increment of 2048
    mov WORD [dap_sector_count], ax         ; The sector size is now in ax
	mov WORD [dap_offset], ROOT_DIRECTORY_ADDRESS   ; Read to address defined as PVD_ADDR
	call LBA_Read                           ; Call the function to read the disk

    ; Load the boot directory
    mov ebx, 0x3000                         ; Destination address
    pop ecx                                 ; The original size of the file
    mov si, bootDirName
    mov dl, [bootDirName.len]
    mov eax, ROOT_DIRECTORY_ADDRESS
    call loadISODirectoryEntry              ; It returns the size of loaded data (0 if failed)
    cmp ax, 0
    je loadFailed

    ; Load the 'isoboot.bin' file to 0x7c00
    mov ecx, eax
    mov eax, 0x3000
    mov ebx, 0x7c00
    mov si, isobootName
    mov dl, [isobootName.len]
    call loadISODirectoryEntry              ; It returns the size of loaded data (0 if failed)
    cmp ax, 0
    je loadFailed

    mov dh, 'M'                            ; 0x41 - M in dh indicates that it loaded from mbr
    mov dl, [bootDrive]
    jmp 0x0000:0x7c00

    jmp $

loadFailed:
    mov si, diskReadError
    call print
    jmp $

%include '../../../include/boot/legacy/rmode/strncmp.inc'
%include '../../../include/boot/legacy/rmode/ISO9660.inc'

; Check if our disk supports int 13h extensions for reading the disk
Check_Ext:
    mov dl, [bootDrive]	                ; DL = Disk Number
    mov ax, 0x4100					    ; INT 13h AH=41h: Check Extensions Present
    mov bx, 0x55AA					    ; Magic value(?)
    int 0x13						    ; Check extensions
    jc .done						    ; Carry flag set? No extensions
    test cx, 1						    ; Does it say we can use packet addressing?
    jz .done						    ; Nope, No extensions
    mov ax, 0xFF					    ; If we reached this point extensions are usable
    mov [EXT13h], al			        ; Set flag

    .done:
        or al, al                       ; Check if al is 1 (if ext 13h is supported)
        jz EXTnotSupported              ; If al = 0 extensions are not supported

        ret								; Else Return

LBA_Read:
    pusha
    mov dl, [bootDrive]
    mov ah, 0x42                        ; EXT Read Sectors
    mov si, dap
    int 0x13                            ; Read Sectors
    jc .error
    .success:
        popa
        ret
    .error:
        mov si, diskReadError
        call print
        jmp $

EXTnotSupported:
    mov si, noEXT13h
    call print
    jmp $

print:
    mov ah, 0x0E
    .loop:
        lodsb
        cmp al, 0x00
        je .done
        int 0x10
        jmp .loop

    .done:
        ret

isobootName: db "isoboot.bin"
    .len: db 11

bootDirName: db "boot"
    .len: db 4

EXT13h:     db 0
bootDrive:  db 0                  ; Our Drive Number Variable

; We have to save space! Use error codes - NS (INT 0x13 extensions not supported), DRE (Disk read error)
noEXT13h:       db "NS", 0x00
diskReadError:  db "DRE", 0x00

LBA_MULTIPLIER: db 2
SIZE_DIVIDER:   db 9

;-------------------------------------------------------------------------------
; DISK ADDRESS PACKET
; Since we have to fit this code in about 430 bytes, move the dap to address 0x0800
; by statically assigning an address to every value
dap             EQU 0x0800
dap_size        EQU 0x0800 	; db 0x10 ; Size of Packet, 16B
dap_zero        EQU 0x0801 	; db 0
dap_sector_count EQU 0x0802 ; dw 28 ; Number of Sectors to Read
dap_offset      EQU 0x0804 ; dw 0x1000
dap_segment     EQU 0x0806 ; dw 0
dap_lba_lo      EQU 0x0808 ; dd 0
dap_lba_hi      EQU 0x080C ; dd 0
;-------------------------------------------------------------------------------

; This has to stay because mkisofs writes data here (Pratitions and some reserved stuff)
times 432 - ($-$$) db 0     ; Make sure code section is 432 bytes in length
times 510 - ($-$$) db 0
dw 0xAA55
