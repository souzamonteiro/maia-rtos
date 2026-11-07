CC = /Users/roberto/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-gcc
CFLAGS = -march=rv32imaf -mabi=ilp32f -c -mno-relax -fno-builtin -fno-pic -Os -I../../../include -I../common -I.
TARGET = files.mobj
SRC = files.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

