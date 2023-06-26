LEGACY_BOOT_DIR = boot/legacy/
UEFI_BOOT_DIR = boot/UEFI/

build: build_legacy_boot

build_legacy_boot:
	@$(MAKE) -C $(LEGACY_BOOT_DIR)

build_uefi_boot:
	@$(MAKE) -C $(UEFI_BOOT_DIR)

.PHONY:
clean:
	rm -rf build
