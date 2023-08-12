# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OUTPUT_DIR = ./../build/AmberOS/System
OUTPUT_FILE = amberkrn.elf
OBJ_FILES = amberkrn.o
CC_FLAGS = -m64 -ffreestanding -I../include
LD_FLAGS = -T amberkrn.ld -static -Bsymbolic -nostdlib

build: $(OUTPUT_FILE)

$(OUTPUT_FILE): $(OBJ_FILES)
	@echo "[Linking $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@ld $(LD_FLAGS) $(OBJ_FILES) -o $(OUTPUT_DIR)/$@
	@rm -rf *.o

%.o: %.cpp
	@echo "[Compiling $^ --> $@]"
	@g++ $(CC_FLAGS) -c $^ -o $@