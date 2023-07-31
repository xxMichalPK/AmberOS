; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]
LoadGDT:
    lgdt [gdt_descriptor]
    ret

gdt_start:
    gdt_null:           ; the null descriptor
        dd 0x0
        dd 0x0
    gdt_code32:         ; the code segment descriptor
        dw 0xffff       ; Limit ( bits 0 -15)
        dw 0x0          ; Base ( bits 0 -15)
        db 0x0          ; Base ( bits 16 -23)
        db 10011010b    ; 1st flags , type flags
        db 11001111b    ; 2nd flags , Limit ( bits 16 -19)
        db 0x0          ; Base ( bits 24 -31)
    gdt_data32:         ; the data segment descriptor
        dw 0xffff       ; Limit ( bits 0 -15)
        dw 0x0          ; Base ( bits 0 -15)
        db 0x0          ; Base ( bits 16 -23)
        db 10010010b    ; 1st flags , type flags
        db 11001111b    ; 2nd flags , Limit ( bits 16 -19)
        db 0x0          ; Base ( bits 24 -31)
    gdt_code16:         ; the code segment descriptor
        dw 0xffff       ; Limit ( bits 0 -15)
        dw 0x0          ; Base ( bits 0 -15)
        db 0x0          ; Base ( bits 16 -23)
        db 10011010b    ; 1st flags , type flags
        db 00001111b    ; 2nd flags , Limit ( bits 16 -19)
        db 0x0          ; Base ( bits 24 -31)
    gdt_data16:         ; the data segment descriptor
        dw 0xffff       ; Limit ( bits 0 -15)
        dw 0x0          ; Base ( bits 0 -15)
        db 0x0          ; Base ( bits 16 -23)
        db 10010010b    ; 1st flags , type flags
        db 00001111b    ; 2nd flags , Limit ( bits 16 -19)
        db 0x0          ; Base ( bits 24 -31)
    gdt_end:

    gdt_descriptor:
        dw gdt_end - gdt_start - 1  ; Size of the GDT , always less one
                                    ; of the true size
        dd gdt_start                ; Start address of the GDT

CODE32_SEG equ gdt_code32 - gdt_start
DATA32_SEG equ gdt_data32 - gdt_start

CODE16_SEG equ gdt_code16 - gdt_start
DATA16_SEG equ gdt_data16 - gdt_start