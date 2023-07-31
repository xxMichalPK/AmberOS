# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OUTPUT_DIR = ./../../build/AmberOS
OUTPUT_FILES = mbr.bin

FS_SPECIFIC_MAKEFILES = EXT2/Makefile FAT32/Makefile ISO9660/Makefile loader/Makefile

build: $(OUTPUT_FILES) $(FS_SPECIFIC_MAKEFILES)

$(FS_SPECIFIC_MAKEFILES): FORCE
	@$(MAKE) -C $(@D) --no-print-directory

%.bin: %.asm
	@echo "[Compiling $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@nasm -f bin $^ -o $(OUTPUT_DIR)/$@

FORCE: