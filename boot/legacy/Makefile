OUTPUT_DIR = ./../../build/boot
OUTPUT_FILES = mbr.bin bootrec.bin bootext.bin

build: $(OUTPUT_FILES)

%.bin: %.asm
	@echo "[Compiling $^ --> $@]"
	@mkdir -p $(OUTPUT_DIR)
	@nasm -f bin $^ -o $(OUTPUT_DIR)/$@
