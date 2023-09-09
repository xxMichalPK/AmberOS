# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

COMPONENTS 	= cpu/ memory/

OUTPUT_DIR 	= $(PROJECT_DIR)/build/AmberOS/System
OUTPUT_FILE = $(OUTPUT_DIR)/amberkrn.elf

OBJ_DIR 	= $(PROJECT_DIR)/obj/kernel

OBJ_FILES 	= $(OBJ_DIR)/amberkrn.o \
			  $(OBJ_DIR)/dlloader.o

ALL_OBJ_FILES	= $(wildcard $(OBJ_DIR)/*.o)

LD_FLAGS 	= -T amberkrn.ld -static -Bsymbolic -nostdlib

build: $(OUTPUT_FILE)
	@echo "$(COLOR_OK)[KERNEL compiled and linked]$(COLOR_RESET)"

$(OUTPUT_FILE): $(COMPONENTS) $(OBJ_FILES)
	@echo "[Linking $(notdir $(ALL_OBJ_FILES)) --> $(@F)]"
	@mkdir -p $(@D)
	@$(LD) $(LD_FLAGS) $(ALL_OBJ_FILES) -o $(OUTPUT_FILE)
	@rm -rf *.o

$(OBJ_DIR)/%.o: %.cpp
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@$(CXX) $(CXX_FLAGS) -c $^ -o $@

$(COMPONENTS): FORCE
	@$(MAKE) -C $(@D) --no-print-directory

FORCE: