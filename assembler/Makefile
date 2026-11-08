CXX ?= clang++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2 -Iinclude -Isrc

SRCS = src/riscv_isa.cpp src/riscv_disasm.cpp src/riscv_asm.cpp
OBJS = $(SRCS:.cpp=.o)

all: maiaasm test_disasm test_roundtrip

maiaasm: $(OBJS) src/main_asm.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test_disasm: src/riscv_isa.o src/riscv_disasm.o tests/test_disasm.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test_roundtrip: $(OBJS) tests/test_roundtrip.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: all
	./test_disasm
	./test_roundtrip

clean:
	rm -f src/*.o tests/*.o maiaasm test_disasm test_roundtrip output.bin

.PHONY: all test clean

