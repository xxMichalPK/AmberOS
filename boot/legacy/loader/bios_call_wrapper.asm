[BITS 32]
section .text
[global bios_call_wrapper]
; Those are the offsets set by the loader. They're hard coded but for now
; I can't figure out a way to make them work without triggering relocation errors
%define CODE16_SEG 0x08
%define DATA16_SEG 0x10
%define CODE32_SEG 0x18
%define DATA32_SEG 0x20

%define TMP_STACK_ADDR 0xF000
%define REGS_SIZE 32

; typedef struct __attribute__((packed)) {
;     uint32_t edi, esi, ebx, edx, ecx, eax;
;     uint16_t gs, fs, es, ds;
; } rmode_regs_t;
; from C: extern void bios_call_wrapper(uint8_t interruptNumber, rmode_regs_t *regs, rmode_regs_t *outRegs);
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
    mov ecx, REGS_SIZE
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
    mov edx, eax            ; use a 'trick' to ensure valid cs:ip. First get the address of rmode and store it in 2 registers
    movzx eax, ax           ; then, get it's offset in the segment using the movzx instruction (this is our IP)
    sub edx, eax            ; subtract the offset from the original address to get the segment (shifted 4 bits to the left)
    shr edx, 4              ; shift the address to the right by 4 bits to get the actual segment
    push dx                 ; Now push the segment first
    push ax                 ; and then the offset
    retf                    ; Do a far return to get into the actual real mode

; We will land right here in the real mode 16bit code section!
rmode:
    mov ax, dx              ; dx holds the current segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Get the stack offset from the base
    mov eax, stack16
    movzx eax, ax
    ; Set the stack to the 16bit stack
    mov ebp, eax
    mov esp, ebp

    ; Restore registers passed in the regs structure
    pop edi
    pop esi
    pop ebx
    pop edx
    pop ecx
    pop eax
    ; And the segments
    pop gs
    pop fs
    pop es
    pop ds
    sti

    db 0xCD                 ; Code of an interrupt instruction
ic: db 0x00                 ; Interrupt number (code) passed to the function

    ; Save all the returned registers
    cli
    ; Start with segments
    push ds
    push es
    push fs
    push gs
    ; And then registers
    push eax
    push ecx
    push edx
    push ebx
    push esi
    push edi

    ; Return to 32bit mode
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

    ; Return the registers to the output registers struct
    mov esi, stack16
    lea edi, [esp + 0x0C]   ; Get the output struct
    mov edi, [edi]
    mov ecx, REGS_SIZE
    rep movsb

    ret

stack32:
    .ebp: dd 0x00000000
    .esp: dd 0x00000000