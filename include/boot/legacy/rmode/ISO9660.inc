; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]
; INPUT: si - directory name; dl - name length; ax - parent directory; es - load segment; bx - load address; ecx - directory size;
loadISODirectoryEntry:
    xchg ax, bx
    push ax
    .findEntryLoop:
        cmp ecx, 0
        je .notFound

        add bx, 32
        mov al, BYTE [bx]
        cmp al, dl
        je .nameCmp

        sub bx, 32
        xor ax, ax
        mov al, BYTE [bx]
        add bx, ax
        sub ecx, eax
        jmp .findEntryLoop
    
        .nameCmp:
            push ecx

            mov di, bx
            inc di
            xor cx, cx
            mov cl, al
            call strncmp16
            pop ecx
            cmp ax, 1
            je .foundEntry

            sub bx, 32
            xor ax, ax
            mov al, BYTE [bx]
            add bx, ax
            sub ecx, eax
            jmp .findEntryLoop

        .foundEntry:
            sub bx, 30
            xor eax, eax
            mov eax, DWORD [bx]
            mov DWORD [dap_lba_lo], eax             ; Set the LBA address
            add bx, 8
            mov eax, DWORD [bx]                     ; Size (in Bytes), convert it to number of sectors
            mov ebx, eax                            ; Save the original size
            pop ax
            push ebx
            push ax
            mov eax, ebx
            mov ebx, 2048
            xor edx, edx
            div ebx
            cmp edx, 0
            je .sizeOK
            inc ax
            .sizeOK:
            mov WORD [dap_sector_count], ax         ; Load 1, 2KiB Long sector (The entire PVD)
            mov WORD [dap_segment], es              ; Read to segment specified in es
            pop ax
            mov WORD [dap_offset], ax               ; Read to address defined as PVD_ADDR
            call LBA_Read                           ; Call the function to read the disk
            pop eax
            ret
        
    .notFound:
        pop ax
        mov ax, 0
        ret