CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
INCLUDES = -Iinclude -I../assembler/include -I../lib/maiaregex

REGEX_SRC = ../lib/maiaregex/maia_regex.c
PARSER_SRC = src/parser.cpp
CODEGEN_SRC = src/codegen_riscv.cpp
ASM_SRCS = ../assembler/src/riscv_isa.cpp ../assembler/src/riscv_asm.cpp ../assembler/src/riscv_disasm.cpp

COMMON_OBJS = $(PARSER_SRC:.cpp=.o) $(CODEGEN_SRC:.cpp=.o) \
              ../assembler/src/riscv_isa.o ../assembler/src/riscv_asm.o ../assembler/src/riscv_disasm.o \
              ../lib/maiaregex/maia_regex.o

all: bin/maiasc bin/test_compiler

../lib/maiaregex/maia_regex.o: $(REGEX_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

../assembler/src/%.o: ../assembler/src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

bin/maiasc: src/main_maiasc.cpp $(COMMON_OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@
	@cp $@ maiasc

bin/test_compiler: tests/test_compiler.cpp $(COMMON_OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

test: bin/test_compiler
	@./bin/test_compiler

clean:
	rm -f src/*.o bin/* maiasc

