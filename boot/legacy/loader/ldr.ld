OUTPUT_FORMAT("binary")
ENTRY(loader_entry)

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

    _loaderEnd = .;

    /DISCARD/ : { *(.fini_array*) *(.comment) }
}