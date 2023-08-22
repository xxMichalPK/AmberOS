[BITS 32]
section .text
[global loader_entry]
[extern loader_main]

loader_entry:
    call loader_main

    .hlt:
        jmp .hlt

; The padding is used to ensure proper loading of the loader file
; without the need of using complex calculations in lower stage loaders
; It's aligned to 4KiB by the linker
section .padding
pad: db 0