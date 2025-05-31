; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]
EnableA20:
    cli

    call CheckA20
    cmp ax, 1
    je .done

    call BIOSA20
    call CheckA20
    cmp ax, 1
    je .done

    call FastA20
    call CheckA20
    cmp ax, 1
    je .done

    call A20WaitIn
    mov al, KbdDisable
    out KbdControllerCmdP, al

    call A20WaitIn
    mov al, KbdReadCtrl
    out KbdControllerCmdP, al

    call A20WaitOut
    in al, KbdControllerDP
    push eax

    call A20WaitIn
    mov al, KbdWriteCtrl
    out KbdControllerCmdP, al

    call A20WaitIn
    pop eax
    or al, 2
    out KbdControllerDP, al

    call A20WaitIn
    mov al, KbdEnable
    out KbdControllerCmdP, al

    call A20WaitIn
    .done:
        sti        
        ret

BIOSA20:
    mov ax, 0x2403      ;--- A20-Gate Support ---
    int 0x15
    jb .ret             ;INT 15h is not supported
    cmp ah, 0
    jnz .ret            ;INT 15h is not supported
    
    mov ax, 0x2402      ;--- A20-Gate Status ---
    int 0x15
    jb .ret             ;couldn't get status
    cmp ah, 0
    jnz .ret            ;couldn't get status
    
    cmp al, 1
    jz .ret             ;A20 is already activated
    
    mov ax, 0x2401      ;--- A20-Gate Activate ---
    int 0x15
    jb .ret             ;couldn't activate the gate
    cmp ah, 0
    jnz .ret            ;couldn't activate the gate
    
    .ret:
        ret

FastA20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

CheckA20:
    pushf
    push ds
    push es
    push di
    push si
    cli
 
    xor ax, ax
    mov es, ax
    not ax

    mov ds, ax
    mov di, 0x0500
    mov si, 0x0510

    mov al, BYTE [es:di]
    push ax
    mov al, BYTE [ds:si]
    push ax

    mov BYTE [es:di], 0x00
    mov BYTE [ds:si], 0xFF
    cmp BYTE [es:di], 0xFF

    pop ax
    mov BYTE [ds:si], al
    pop ax
    mov BYTE [es:di], al

    cmp ax, 0
    je .noA20

    mov ax, 1

    .noA20:
        pop si
        pop di
        pop es
        pop ds
        popf
        ret
        
A20WaitIn:
    in al, KbdControllerCmdP
    test al, 2
    jnz A20WaitIn
    ret

A20WaitOut:
    in al, KbdControllerCmdP
    test al, 1
    jnz A20WaitOut
    ret


KbdControllerDP     equ 0x60
KbdControllerCmdP   equ 0x64

KbdEnable           equ 0xAE
KbdDisable          equ 0xAD

KbdReadCtrl         equ 0xD0
KbdWriteCtrl        equ 0xD1