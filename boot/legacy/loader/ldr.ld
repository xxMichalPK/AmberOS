OUTPUT_FORMAT("binary")
ENTRY(ldrentry)

PHYSICAL_ADDRESS = 0x00010000;

SECTIONS {
    . = PHYSICAL_ADDRESS;
    _loadAddr = .;

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ : { *(.fini_array*) *(.comment) }
}