CC = /Users/roberto/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-gcc
CFLAGS = -march=rv32imafc -mabi=ilp32f -c -mno-relax -fno-builtin -fno-pic -I../../../include -I.
TARGET = desktop.mobj
SRC = desktop.c

all: $(TARGET)

$(TARGET): $(SRC) font_8x16.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

