# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

AMBER_INC		= $(INCLUDE_DIR)
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

OUTPUT_DIR 	= $(PROJECT_DIR)/build/AmberOS/EFI/BOOT
OBJ_DIR 	= $(PROJECT_DIR)/obj/boot

OUTPUT_FILE	= $(OUTPUT_DIR)/BOOTX64.EFI
OBJ_FILE	= $(OBJ_DIR)/BOOTX64.o

build: $(OUTPUT_FILE)
	@echo "$(COLOR_OK)[UEFI loader compiled]$(COLOR_RESET)"

$(OBJ_DIR)/%.o: %.c
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ_DIR)/%.so: $(OBJ_DIR)/%.o
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) $^ -o $@ -lefi -lgnuefi

$(OUTPUT_DIR)/%.EFI: $(OBJ_DIR)/%.so
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $(OUTPUT_FILE)