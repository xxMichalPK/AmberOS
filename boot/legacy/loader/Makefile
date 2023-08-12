# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OUTPUT_DIR = ./../../../build/boot
OUTPUT_FILE = amberldr.bin
OBJ_FILES = ldrentry.o bios_call_wrapper.o amberldr.o
CC_FLAGS = -m32 -ffreestanding -fno-PIC -fno-PIE -I../../../include
LD_FLAGS = -melf_i386 -T ldr.ld

build: $(OUTPUT_FILE)

$(OUTPUT_FILE): $(OBJ_FILES)
	@echo "[Linking $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@ld $(LD_FLAGS) $(OBJ_FILES) -o $(OUTPUT_DIR)/$@
	@rm -rf *.o

%.o: %.c
	@echo "[Compiling $^ --> $@]"
	@gcc $(CC_FLAGS) -c $^ -o $@

%.o: %.asm
	@echo "[Compiling $^ --> $@]"
	@nasm -felf32 $^ -o $@