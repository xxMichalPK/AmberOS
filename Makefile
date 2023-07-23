# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OS_NAME = AmberOS
OS_ARCH = x86_64
OS_VERSION = 0.1a
OS_CODENAME = Tyro

LEGACY_BOOT_DIR = boot/legacy/
UEFI_BOOT_DIR = boot/UEFI/

ISO: build_legacy_boot build_uefi_boot
	@echo "[Preparing files for ISO creation...]"
	@mkdir -p iso
	@cp -r build/* iso/

	@echo "[Creating EFI boot image...]"
	@dd if=/dev/zero of=iso/boot/efiboot.img bs=512 count=93750
	@mformat -i iso/boot/efiboot.img -F ::
	@mmd -i iso/boot/efiboot.img ::/EFI
	@mmd -i iso/boot/efiboot.img ::/EFI/BOOT
	@mcopy -i iso/boot/efiboot.img iso/boot/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT
	@mcopy -i iso/boot/efiboot.img startup.nsh ::

	@echo "[Creating ISO file...]"
	@xorriso -as mkisofs -U \
		-b boot/isoboot.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-isohybrid-mbr ./iso/boot/mbr.bin \
		-eltorito-alt-boot \
		-e boot/efiboot.img \
		-no-emul-boot -isohybrid-gpt-basdat \
		-hide boot/efiboot.img \
		-hide boot/isoboot.img \
		-V "$(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH) Installer" \
		-iso-level 4 \
		-o $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso \
		iso/

build_legacy_boot:
	@echo "[Building the legacy bootloader...]"
	@$(MAKE) -C $(LEGACY_BOOT_DIR) --no-print-directory

build_uefi_boot:
	@echo "[Building UEFI bootloader...]"
	@$(MAKE) -C $(UEFI_BOOT_DIR) --no-print-directory

run_legacy: ISO
	@echo "[Running $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso...]"
	@qemu-system-x86_64 -cdrom $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso

run_UEFI: ISO
	@echo "[Running $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso...]"
	@qemu-system-x86_64 -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="./tools/OVMFbin/OVMF_CODE-pure-efi.fd",readonly=on \
		-drive if=pflash,format=raw,unit=1,file="./tools/OVMFbin/OVMF_VARS-pure-efi.fd" -cdrom $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso

.PHONY:
clean:
	@echo "[Cleaning build files...]"
	@rm -rf build iso *.iso
