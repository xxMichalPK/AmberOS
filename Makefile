OS_NAME = AmberOS
OS_ARCH = x86_64
OS_VERSION = 0.1a
OS_CODENAME = Tyro

LEGACY_BOOT_DIR = boot/legacy/
UEFI_BOOT_DIR = boot/UEFI/

iso: build
	mkdir -p iso
	cp build/boot/* iso/
	xorriso -as mkisofs -U -b isoboot.bin -no-emul-boot -hide isoboot.bin \
		-V "$(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH) Installer" -iso-level 3 -o $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso iso/

build: build_legacy_boot

build_legacy_boot:
	@$(MAKE) -C $(LEGACY_BOOT_DIR)

build_uefi_boot:
	@$(MAKE) -C $(UEFI_BOOT_DIR)

run: iso
	qemu-system-x86_64 -cdrom $(OS_NAME)-$(OS_CODENAME)-$(OS_ARCH).iso

.PHONY:
clean:
	rm -rf build iso *.iso
