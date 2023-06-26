# Introduction #

"NewOS" -- as for now -- is my most recent attempt on creating a fully working x86_64 Operating System. The goal of this project is to extend my knowledge of OS development as well as understanding the basics of communication between software and hardware components. The entire kernel and bootloader of "NewOS"
will be written in C++ and Assembly Language, which will help me extend my knowledge of programming in those languages.

# Steps & ToDo List #

## 1) Bootloader ##

- MBR code for MBR partitioned disks
- Legacy 1'st stage bootloader capable of loading the 2'nd stage from disk
- Legacy 2'nd stage bootloader that handles the loading process of the final stage of the bootloader, switches the video mode and initializes the 32bit CPU mode for later use in the final stage of the bootloader
- UEFI bootloader that does the same things that the legacy bootloader's 1'st and 2'nd stages do
- 3'rd stage bootloader for both Legacy and UEFI booting. It switches to 64-bit mode if the kernel requires it and and transfers control to the kernel loaded from an ELF file

## 2) Kernel ##

### a) Basic interrupt handling ###

- Remapping the PIC
- Setting up a basic handler for all the interrupts and IRQs

### b) Simple standard library functions ###

- printf
- memcpy
- memset
- ...

### c) Basic drivers ###

- PS2 keyboard driver
- RTC clock driver
- IRQ1 timer driver
- ATA PIO mode driver

### d) Page Manager ###

### e) Memory manager ###

- Physical Memory Manager
- Virtual Memory Manager

### f) More complex functionalities ###

- Dynamic memory allocation + HEAP
- PCI driver
- AHCI disk driver
- Loading programs from any type of disk
- ...

## 3) Applications ##

- Port some applications from Linux
