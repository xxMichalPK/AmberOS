# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OUTPUT_DIR = 	$(PROJECT_DIR)/build/boot
OBJ_DIR = 		$(PROJECT_DIR)/obj/boot

OUTPUT_FILE = 	$(OUTPUT_DIR)/amberldr.bin

OBJ_FILES = 	$(OBJ_DIR)/ldrentry.o \
				$(OBJ_DIR)/bios_call_wrapper.o \
				$(OBJ_DIR)/amberldr.o

LD_FLAGS = 		-melf_i386 -T ldr.ld

build: $(OUTPUT_FILE)
	@echo "$(COLOR_OK)[Legacy loader compiled]$(COLOR_RESET)"

$(OUTPUT_FILE): $(OBJ_FILES)
	@echo "[Linking $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@$(LD) $(LD_FLAGS) $(OBJ_FILES) -o $@

$(OBJ_DIR)/%.o: %.c
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@$(CC) $(LOADER_CC_FLAGS) -c $^ -o $@

$(OBJ_DIR)/%.o: %.asm
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@nasm -felf32 $^ -o $@