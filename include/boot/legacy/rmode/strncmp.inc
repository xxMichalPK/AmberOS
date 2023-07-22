[BITS 16]

strncmp16:
	.cmpLoop:
		dec cx
		mov al, [si]
		mov ah, [di]

		cmp al, ah
		jne .neq

		cmp cx, 0
		je .eq

		inc si
		inc di
		jmp .cmpLoop

	.neq:
		mov ax, 0
		ret
	
	.eq:
		mov ax, 1
		ret
