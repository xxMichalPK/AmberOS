[BITS 32]
section .text
[global bios_call_wrapper]
[extern _loadAddr]

%define BASE_SEGMENT 0x1000
; Those are the offsets set by the 1'st stage bootloader
%define CODE32_SEG 0x08
%define DATA32_SEG 0x10
%define CODE16_SEG 0x18
%define DATA16_SEG 0x20
%define TMP_STACK_ADDR 0xF000

; typedef struct __attribute__((packed)) {
;     uint16_t di, si, bx, dx, cx, ax;
;     uint16_t gs, fs, es, ds;
; } rmode_regs_t;
; from C: extern void bios_call_wrapper(uint8_t interruptNumber, rmode_regs_t *regs);
bios_call_wrapper:
    cli                     ; Clear the interrupts
    pushad                  ; Save everything
    pushf                   ; Save the flags register
    mov [stack32.ebp], ebp  ; Save the current stack
    mov [stack32.esp], esp  ; to a variable for restoring it later

    lea esi, [esp + 0x28]   ; Set the interrupt number
    lodsd                   ; passed as the first argument
    mov [ic], al            ; in ic (interrupt code)

    mov esi, [esi]          ; The registers are here
    mov edi, stack16
    mov ecx, 32
    rep movsb
    
    jmp CODE16_SEG:prot16   ; Jump to 16bit protected mode

[BITS 16]
prot16:
    mov ax, DATA16_SEG      ; Set all segments to the 16bit segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, cr0            ; Clear the PE bit
    and al, 0
    mov cr0, eax

    ; Set temporary stack
    mov ebp, TMP_STACK_ADDR
    mov esp, ebp

    mov eax, rmode          ; Since the linker was complaining about relocations
    mov ebx, _loadAddr       ; use a 'trick' to ensure valid cs:ip
    sub eax, ebx            ; Get the base address of this executable (_loadAddr)
    push BASE_SEGMENT       ; And substract it from the address of the rmode function (this is out IP)
    push ax                 ; Push first the segment (BASE_SEGMENT = 0x1000) and then the offset calculated in ax to stack
    retf                    ; And perform a far return

; We will land right here in the real mode 16bit code section!
rmode:
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Get the stack offset from the base
    mov eax, stack16
    mov ebx, _loadAddr
    sub eax, ebx
    ; Set the stack to the 16bit stack
    mov ebp, eax
    mov esp, ebp

    ; Restore registers passed in the regs structure
    pop di
    pop si
    pop bx
    pop dx
    pop cx
    pop ax
    ; And the segments
    pop gs
    pop fs
    pop es
    pop ds
    sti

    db 0xCD                 ; Code of an interrupt instruction
ic: db 0x00                 ; Interrupt number (code) passed to the function

    ; Return to 32bit mode
    cli
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp dword CODE32_SEG:returnTo32bit

align 0x100
section .bss
resb 0x400  ; 1KiB for 16bit stack
stack16:
resb 0x400  ; Make sure that no data will go here

[BITS 32]
section .text
returnTo32bit:
    mov ax, DATA32_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore the saved stack and registers
    mov ebp, [stack32.ebp]
    mov esp, [stack32.esp]
    popf
    popad

    ret

stack32:
    .ebp: dd 0x00000000
    .esp: dd 0x00000000