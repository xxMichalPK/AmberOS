# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OUTPUT_DIR = ./../../../build/AmberOS
OUTPUT_FILES = mbr.bin isohd.bin

build: $(OUTPUT_FILES)

%.bin: %.asm
	@echo "[Compiling $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@nasm -f bin $^ -o $(OUTPUT_DIR)/$@