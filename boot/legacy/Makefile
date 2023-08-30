# /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
#  * This file is part of the "AmberOS" project
#  * Unauthorized copying of this file, via any medium is strictly prohibited
#  * Proprietary and confidential
#  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
# */

LEGACY_MAKEFILES = 	mbr/Makefile \
					ISO9660/Makefile \
					FAT32/Makefile \
					EXT2/Makefile \
				   	loader/Makefile

build: $(LEGACY_MAKEFILES)

$(LEGACY_MAKEFILES): FORCE
	@$(MAKE) -C $(@D) --no-print-directory

FORCE: