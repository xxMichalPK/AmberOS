# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

OBJ_DIR = ./../../obj
OBJ_FILES = memcpy.o
CC_FLAGS = -m64 -ffreestanding -I../../include -Ofast

build: $(OBJ_FILES)

%.o: %.cpp
	@echo "[Compiling $^ --> $@]"
	@mkdir -p $(OBJ_DIR)
	@g++ $(CC_FLAGS) -c $^ -o $(OBJ_DIR)/$@