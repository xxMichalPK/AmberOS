OS_NAME = AmberOS
OS_ARCH = x86_64
OS_VERSION = 0.1a
OS_CODENAME = Tyro

LEGACY_BOOT_DIR = boot/legacy/
UEFI_BOOT_DIR = boot/UEFI/

iso: build
	@echo "[Preparing files for ISO creation...]"
	mkdir -p iso
	cp -r build/* iso/

	@echo "[Creating ISO file...]"
	xorriso -as mkisofs -U -b boot/isoboot.bin -no-emul-boot -hide boot/isoboot.bin \
		-V "$(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH) Installer" -iso-level 3 -o $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso iso/

build: build_legacy_boot

build_legacy_boot:
	@echo "[Building the legacy bootloader...]"
	@$(MAKE) -C $(LEGACY_BOOT_DIR) --no-print-directory

build_uefi_boot:
	@echo "[Building UEFI bootloader...]"
	@$(MAKE) -C $(UEFI_BOOT_DIR) --no-print-directory

run: iso
	@echo "[Running $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso...]"
	@qemu-system-x86_64 -cdrom $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso

.PHONY:
clean:
	@echo "[Cleaning build files...]"
	@rm -rf build iso *.iso
