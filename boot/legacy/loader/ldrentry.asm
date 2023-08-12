[BITS 32]
section .text
[global loader_entry]
[extern loader_main]

loader_entry:
    call loader_main

    .hlt:
        jmp .hlt