[BITS 32]
section .text
[global ldrentry]
[extern ldrmain]

ldrentry:
    call ldrmain

    .hlt:
        jmp .hlt