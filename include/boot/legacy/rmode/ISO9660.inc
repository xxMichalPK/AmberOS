; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]
; INPUT: si - directory name; dl - name length; ax - parent directory; fs - load segment; bx - load address; ecx - directory size;
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
            movzx cx, al
            call strncmp16
            pop ecx
            cmp ax, 1
            je .foundEntry

            sub bx, 32
            movzx ax, BYTE [bx]
            add bx, ax
            sub ecx, eax
            jmp .findEntryLoop

        .foundEntry:
            sub bx, 30
            mov eax, DWORD [bx]
            mov cl, [LBA_MULTIPLIER]
            shl eax, cl
            mov DWORD [dap_lba_lo], eax             ; Set the LBA address
            add bx, 8
            mov eax, DWORD [bx]                     ; Size (in Bytes), convert it to number of sectors
            pop bx
            push eax
            mov cl, [SIZE_DIVIDER]
            shr eax, cl
            mov WORD [dap_sector_count], ax         ; Load 1, 2KiB Long sector (The entire PVD)
            mov WORD [dap_segment], fs              ; Read to segment specified in es
            mov WORD [dap_offset], bx               ; Read to address defined as PVD_ADDR
            call LBA_Read                           ; Call the function to read the disk
            pop eax
            ret
        
    .notFound:
        pop ax
        mov ax, 0
        ret