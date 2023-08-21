[BITS 16]

strncmp16:
	push si
	repe cmpsb
	jz .eq

	.neq:
		pop si
		xor ax, ax
		ret
	
	.eq:
		pop si
		mov ax, 1
		ret
