# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OUTPUT_DIR = 	$(PROJECT_DIR)/build/AmberOS

OUTPUT_FILES = 	$(OUTPUT_DIR)/mbr.bin \
				$(OUTPUT_DIR)/isohd.bin

build: $(OUTPUT_FILES)
	@echo "$(COLOR_OK)[MBR files compiled]$(COLOR_RESET)"

$(OUTPUT_DIR)/%.bin: %.asm
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@nasm -f bin $^ -o $@