# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

AMBER_INC		= ../../include/
EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol -I$(AMBER_INC)
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS          = $(EFIINCS) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wno-unused-function -Wno-unused-variable
LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFILIB) $(EFI_CRT_OBJS)

ifeq ($(ARCH),x86_64)
  CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

OUTPUT_DIR = ./../../build/AmberOS/EFI/BOOT
OUTPUT_FILE = amberldr.EFI
BOOT_FILE = BOOTX64.EFI

build: $(OUTPUT_FILE)

%.o: %.c
	@echo "[Compiling $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@gcc $(CFLAGS) -c $^ -o $@

%.so: %.o
	@echo "[Compiling $^ --> $@]"
	@ld $(LDFLAGS) $^ -o $@ -lefi -lgnuefi

%.EFI: %.so
	@echo "[Compiling $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@objcopy -j .text -j .sdata -j .data -j .dynamic \
	-j .dynsym -j .rel -j .rela -j .reloc \
	--target=efi-app-$(ARCH) $^ $(OUTPUT_DIR)/$(BOOT_FILE)