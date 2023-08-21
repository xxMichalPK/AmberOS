; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

; The Master Boot Record of the System
; ! Can be used only on disks with int 13h extensions !
; It checks every partition for bootable flag and the OS signature
; If the signature is found it loads this system
; If not, it loads the last checked bootable partition
; Example:                P1. This OS P2. Win7
; Boot state:               boot        don't
; 
; Example:                P1. MSDOS     P2. Win7
; Boot state:               don't         boot (last checked partition)

; Define the Boot Signature - M(4D) P(50) in reverse order
%define OS_SIG 0x504D

[BITS 16]                               ; We are in 16-bit real mode
[ORG 0x0600]                            ; Position of this programm in memory
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
    mov BYTE [bootDrive], dl            ; Save BootDrive
    mov bp, sp

    sti                                 ; Enable interrupts

    ; Check if disk supports extended int 13h functions to read the LBA of bootable partition
    call Check_Ext                      ; Call check function

    .CheckPartitions:                   ; Check Partition Table For Bootable Partition
        mov bx, PT1_Status              ; Base = Partition Table Entry 1
        mov cx, 4                       ; There are 4 Partition Table Entries
        .CheckLoop:
            mov WORD [CheckPart], bx
            mov al, BYTE [bx]           ; Get Boot indicator bit flag
            test al, 0x80               ; Check For Active Bit
            jnz .BootPartFound          ; We Found an Active Partition
            .nextPart:
                mov bx, WORD [CheckPart]
                add bx, 0x10                ; Jump to next Partition Table Entry
                                            ; Partition Table Entry is 16 Bytes !! All 4 partitions are 64B !!
                dec cx                      ; Decrement Counter
                jnz .CheckLoop              ; Loop
                mov ax, WORD [LastBootPart]
                cmp ax, 0
                jne .forceOtherVBR
                jmp noBOOT                  ; ERROR!
        
            .BootPartFound:
                mov WORD [LastBootPart], bx    ; Save First Bootable Partition Offset

            .ReadVBR:
                mov bx, WORD [LastBootPart]
                mov ebx, DWORD [bx + 8]         ; Start LBA of Active Partition
                mov [dap_lba_lo], ebx
                mov word [dap_sector_count], 1
                mov bx, 0x7C00
                mov word [dap_offset], 0x7C00
                call LBA_Read

            .jumpToVBR:
                cmp WORD [0x7DFE], 0xAA55       ; Check Boot Signature
                jne noSIG                       ; Error if not Boot Signature
                cmp WORD [0x7DFC], OS_SIG       ; Check OS signature
                jne .nextPart
                mov si, WORD [LastBootPart]     ; Set DS:SI to Partition Table Entry
                mov dl, BYTE [bootDrive]        ; Set DL to Drive Number
                jmp 0:0x7C00                    ; Jump To VBR (Bootloader)

            .forceOtherVBR:
                mov ebx, DWORD [bx + 8]         ; Start LBA of Active Partition
                mov [dap_lba_lo], ebx
                mov word [dap_sector_count], 1
                mov bx, 0x7C00
                mov word [dap_offset], 0x7C00
                call LBA_Read

                cmp WORD [0x7DFE], 0xAA55       ; Check Boot Signature
                jne noSIG                       ; Error if not Boot Signature
                mov si, WORD [LastBootPart]     ; Set DS:SI to Partition Table Entry
                mov dl, BYTE [bootDrive]        ; Set DL to Drive Number
                jmp 0:0x7C00                    ; Jump To VBR (Bootloader)

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

noBOOT:
    mov si, notBootable
    call print
    jmp $

noSIG:
    mov si, noSignature
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

noEXT13h:       db "Disk extensions not supported!", 0x0A, 0x0D, 0x00
notBootable:    db "No bootable partition found!", 0x0A, 0x0D, 0x00
noSignature:    db "Partition has wrong boot signature!", 0x0A, 0x0D, 0x00
diskReadError:  db "Disk read error!", 0x00
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

EXT13h: db 0
bootDrive db 0                  ; Our Drive Number Variable
LastBootPart dw 0               ; The last found bootable partition on disk
CheckPart dw 0                  ; The partition we are currently checking

times 440-($-$$)            db 0x00         ;Make sure code section is 440 bytes in length

Disk_Sig                    dd 0x00000000   ;Disk signature, used to track drives 
Reserved                    dw 0x0000       ;Reserved   
;********************************************Partition Table*********************************************
; Partition table - Four 16-Byte entries describing the disk partitioning  
PT1_Status                  db 0x00         ;Drive number/Bootable flag
PT1_First_Head              db 0x00         ;First Head
PT1_First_Sector            db 0x00         ;Bits 0-5:First Sector|Bits 6-7 High bits of First Cylinder
PT1_First_Cylinder          db 0x00         ;Bits 0-7 Low bits of First Cylinder
PT1_Part_Type               db 0x00         ;Partition Type
PT1_Last_Head               db 0x00         ;Last Head 
PT1_Last_Sector             db 0x00         ;Bits 0-5:Last Sector|Bits 6-7 High bits of Last Cylinder
PT1_Last_Cylinder           db 0x00         ;Bits 0-7 Low bits of Last Cylinder
PT1_First_LBA               dd 0x00000000   ;Starting LBA of Partition
PT1_Total_Sectors           dd 0x00000000   ;Total Sectors in Partition
PT2_Status                  db 0x00
PT2_First_Head              db 0x00
PT2_First_Sector            db 0x00
PT2_First_Cylinder          db 0x00
PT2_Part_Type               db 0x00
PT2_Last_Head               db 0x00
PT2_Last_Sector             db 0x00
PT2_Last_Cylinder           db 0x00
PT2_First_LBA               dd 0x00000000
PT2_Total_Sectors           dd 0x00000000
PT3_Status                  db 0x00
PT3_First_Head              db 0x00
PT3_First_Sector            db 0x00
PT3_First_Cylinder          db 0x00
PT3_Part_Type               db 0x00
PT3_Last_Head               db 0x00
PT3_Last_Sector             db 0x00
PT3_Last_Cylinder           db 0x00
PT3_First_LBA               dd 0x00000000
PT3_Total_Sectors           dd 0x00000000
PT4_Status                  db 0x00
PT4_First_Head              db 0x00
PT4_First_Sector            db 0x00
PT4_First_Cylinder          db 0x00
PT4_Part_Type               db 0x00
PT4_Last_Head               db 0x00
PT4_Last_Sector             db 0x00
PT4_Last_Cylinder           db 0x00
PT4_First_LBA               dd 0x00000000
PT4_Total_Sectors           dd 0x00000000
;---------------------------------------------------------------------------------------------------------------------------------;
times 510 - ($-$$) db 0
dw 0xAA55
