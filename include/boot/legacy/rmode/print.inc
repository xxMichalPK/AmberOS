; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]
print16:
    mov ah, 0x0E
    .prntLoop:
        lodsb
        cmp al, 0x00
        je .done

        int 0x10
        jmp .prntLoop

        .done:
            ret

get_cur:
	mov ah, 0x03
	int 0x10
	ret

printc16:
	mov ah, 0x13
	mov al, 0x03
	mov bx, 0
	mov es, bx
	mov bp, si

	push ax
	push bx
	push cx
	call get_cur
	pop cx
	pop bx
	pop ax

	int 0x10
	ret
