# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OBJ_DIR 	= $(PROJECT_DIR)/obj/kernel

OBJ_FILES 	= $(OBJ_DIR)/cpuid.o \
			  $(OBJ_DIR)/sse.o \
			  $(OBJ_DIR)/avx.o \
			  $(OBJ_DIR)/ermsb.o \
			  $(OBJ_DIR)/tsc.o

build: $(OBJ_FILES)
	@echo "$(COLOR_OK)[CPU kernel components compiled]$(COLOR_RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@echo "[Compiling $(^F) --> $(@F)]"
	@mkdir -p $(@D)
	@$(CXX) $(CXX_FLAGS) -c $^ -o $@