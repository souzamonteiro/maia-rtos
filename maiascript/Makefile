CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
INCLUDES = -Iinclude -I../assembler/include -I../lib/maiaregex

GRAMMAR_EBNF = docs/MaiaScript_Native_v0_1_MaiaCC.ebnf
PARSER_INC = src/maiascript_parser.inc
TREX_SH = ../tools/maiacc/bin/tREx.sh

REGEX_SRC = ../lib/maiaregex/maia_regex.c
PARSER_SRC = src/parser.cpp
CODEGEN_SRC = src/codegen_riscv.cpp
ASM_SRCS = ../assembler/src/riscv_isa.cpp ../assembler/src/riscv_asm.cpp ../assembler/src/riscv_disasm.cpp

COMMON_OBJS = $(PARSER_SRC:.cpp=.o) $(CODEGEN_SRC:.cpp=.o) \
              ../assembler/src/riscv_isa.o ../assembler/src/riscv_asm.o ../assembler/src/riscv_disasm.o \
              ../lib/maiaregex/maia_regex.o

all: bin/maiasc bin/test_compiler

parser: $(GRAMMAR_EBNF)
	bash $(TREX_SH) --ebnf --language cpp98 $< $(PARSER_INC)
	@python3 tests/tools/patch_parser_try.py $(PARSER_INC)
	@echo "==> Running full regression test suite after grammar update..."
	$(MAKE) test

src/parser.o: src/parser.cpp $(PARSER_INC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

../lib/maiaregex/maia_regex.o: $(REGEX_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

../assembler/src/%.o: ../assembler/src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

bin/maiasc: src/main_maiasc.cpp include/emulator_rv32.hpp $(COMMON_OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(COMMON_OBJS) -o $@
	@cp $@ maiasc

bin/test_compiler: tests/test_compiler.cpp include/emulator_rv32.hpp $(COMMON_OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(COMMON_OBJS) -o $@

bin/test_parser_lifetime: tests/test_parser_lifetime.cpp src/parser.o ../lib/maiaregex/maia_regex.o include/parser.hpp include/ast.hpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< src/parser.o ../lib/maiaregex/maia_regex.o -o $@

test: bin/test_compiler bin/test_parser_lifetime
	@./bin/test_compiler
	@./bin/test_parser_lifetime

clean:
	rm -f src/*.o bin/* maiasc

