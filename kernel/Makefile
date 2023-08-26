# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

COMPONENTS = cpu/ memory/

OUTPUT_DIR = ./../build/AmberOS/System
OUTPUT_FILE = amberkrn.elf

OBJ_DIR = ./../obj
OBJ_FILES = amberkrn.o

CC_FLAGS = -m64 -ffreestanding -I../include
LD_FLAGS = -T amberkrn.ld -static -Bsymbolic -nostdlib

build: $(COMPONENTS) $(OBJ_FILES)
	@echo "[Linking $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@ld $(LD_FLAGS) $(OBJ_DIR)/*.o -o $(OUTPUT_DIR)/$(OUTPUT_FILE)
	@rm -rf *.o

%.o: %.cpp
	@echo "[Compiling $^ --> $@]"
	@g++ $(CC_FLAGS) -c $^ -o $(OBJ_DIR)/$@

$(COMPONENTS): FORCE
	@$(MAKE) -C $(@D) --no-print-directory

FORCE: