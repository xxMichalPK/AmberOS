# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

# OS Name variables
OS_NAME = AmberOS
OS_ARCH = x86_64
OS_VERSION = 0.1a
OS_CODENAME = Tyro

# Useful directories
LEGACY_BOOT_DIR = boot/legacy/
UEFI_BOOT_DIR = boot/UEFI/
KERNEL_DIR = kernel/
export PROJECT_DIR = $(CURDIR)
export INCLUDE_DIR = $(PROJECT_DIR)/include

# Build tools
export CC	= gcc
export CXX	= g++
export LD	= ld

# Build flags
export LOADER_CC_FLAGS = -m32 -ffreestanding -fno-PIC -fno-PIE -I$(INCLUDE_DIR)
export CC_FLAGS = -m64 -ffreestanding -I$(INCLUDE_DIR)/
export CXX_FLAGS = -m64 -ffreestanding -I$(INCLUDE_DIR)/

# Message colors and cursor movements
export CURSOR_UP	= \033[1F
export MSG_REPLACE 	= $(CURSOR_UP)\033[2K
export COLOR_OK 	= \033[1;32m
export COLOR_INFO 	= \033[1;33m
export COLOR_ERROR 	= \033[1;31m
export COLOR_RESET 	= \033[22;0m

# Output files
ISO_FILE = $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso

ISO: $(ISO_FILE)
	@echo "$(COLOR_OK)[ISO creation finished]$(COLOR_RESET)"

$(ISO_FILE): build_legacy_boot build_uefi_boot build_kernel
ifeq (,$(wildcard ./$(ISO_FILE)))
	@echo "$(COLOR_INFO)[Preparing files for ISO creation...]$(COLOR_RESET)"
	@mkdir -p iso iso/AmberOS/SysConfig/ \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to create directories]$(COLOR_RESET)" && exit 1; }
	@cp -r build/* iso/ \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to copy the build directory]$(COLOR_RESET)" && exit 1; }
	@cp -r branding/* iso/AmberOS/ \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to copy the branding directory]$(COLOR_RESET)" && exit 1; }
	@cp -r configuration/* iso/AmberOS/SysConfig/ \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to copy the configuration directory]$(COLOR_RESET)" && exit 1; }
	@echo "$(MSG_REPLACE)$(COLOR_OK)[Prepared files for ISO creation]$(COLOR_RESET)"

	@echo "$(COLOR_INFO)[Creating EFI boot image...]$(COLOR_RESET)"
	@dd if=/dev/zero of=iso/boot/efiboot.img bs=512 count=10240 \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to create boot image]$(COLOR_RESET)" && exit 1; }
	@mformat -i iso/boot/efiboot.img :: \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to format the boot image]$(COLOR_RESET)" && exit 1; }
	@mmd -i iso/boot/efiboot.img ::/EFI \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to create EFI directory]$(COLOR_RESET)" && exit 1; }
	@mmd -i iso/boot/efiboot.img ::/EFI/BOOT \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to create EFI/BOOT directory]$(COLOR_RESET)" && exit 1; }
	@mcopy -i iso/boot/efiboot.img iso/AmberOS/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to copy the UEFI loader to boot image]$(COLOR_RESET)" && exit 1; }
	@mcopy -i iso/boot/efiboot.img startup.nsh :: \
		> /dev/null 2>&1 || \
		{ echo "$(COLOR_ERROR)[Failed to copy the startup script to boot image]$(COLOR_RESET)" && exit 1; }
	@echo "$(MSG_REPLACE)$(COLOR_OK)[EFI boot image created]$(COLOR_RESET)"

	@echo "$(COLOR_INFO)[Creating ISO file...]$(COLOR_RESET)"
	@xorriso -as mkisofs -U \
		-c boot/boot.cat \
		-b boot/isoboot.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-isohybrid-mbr ./iso/AmberOS/isohd.bin \
		-eltorito-alt-boot \
		-e boot/efiboot.img \
		-no-emul-boot -isohybrid-gpt-basdat \
		-V "$(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH) Installer" \
		-iso-level 4 \
		-o $(ISO_FILE) \
		iso/ \
		> /dev/null 2>&1 || \
			{ echo "$(MSG_REPLACE)$(COLOR_ERROR)[Failed to create ISO file]$(COLOR_RESET)" && exit 1; }
	
	@echo "$(MSG_REPLACE)$(CURSOR_UP)"
endif

build_legacy_boot:
	@$(MAKE) -C $(LEGACY_BOOT_DIR) --no-print-directory

build_uefi_boot:
	@$(MAKE) -C $(UEFI_BOOT_DIR) --no-print-directory

build_kernel:
	@$(MAKE) -C $(KERNEL_DIR) build --no-print-directory

run_legacy: ISO
	@echo "$(COLOR_INFO)[Running $(OS_NAME) in BIOS mode]$(COLOR_RESET)"
	@qemu-system-x86_64 -cdrom $(ISO_FILE)

run_UEFI: ISO
	@echo "$(COLOR_INFO)[Running $(OS_NAME) in UEFI mode]$(COLOR_RESET)"
	@qemu-system-x86_64 -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="./tools/OVMFbin/OVMF_CODE-pure-efi.fd",readonly=on \
		-drive if=pflash,format=raw,unit=1,file="./tools/OVMFbin/OVMF_VARS-pure-efi.fd" -hda $(ISO_FILE)

debug: ISO
	@echo "$(COLOR_INFO)[Debugging $(OS_NAME) in BIOS mode]$(COLOR_RESET)"
	@bochs -q -f ./bochsrc.bxrc

.PHONY:
clean:
	@echo "$(COLOR_INFO)[Cleaning build files...]$(COLOR_RESET)"
	@rm -rf build obj iso *.iso > /dev/null 2>&1 || \
		{ echo "$(MSG_REPLACE)$(COLOR_ERROR)[Failed to clean the working directory]$(COLOR_RESET)" && exit 1; }
	@echo "$(MSG_REPLACE)$(COLOR_OK)[Cleaned working directory]$(COLOR_RESET)"
