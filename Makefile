CC = gcc
CFLAGS = -c -g -Wall -I./C_Components -MMD -MP
AR = ar
ARFLAGS = rcs
SHELL := /bin/bash


SRC_DIR = C_Components

LEXER_L = $(SRC_DIR)/lexer.l
PARSER_Y = $(SRC_DIR)/parser.y
LEXER_C = $(SRC_DIR)/lexer.c
PARSER_C = $(SRC_DIR)/parser.c
PARSER_H = $(SRC_DIR)/parser.h
AST_C = $(SRC_DIR)/ast.c
SYMBOL_TABLE_C = $(SRC_DIR)/symbol_table.c
SYMBOL_TABLE_H = $(SRC_DIR)/symbol_table.h
SEMANTIC_ANALYZER_C = $(SRC_DIR)/semantic_analyzer.c
INTERMEDIATE_CODE_GEN_C = $(SRC_DIR)/intermediate_code_gen.c
ASSEMBLY_GEN_C = $(SRC_DIR)/assembly_gen.c

OBJS = lexer.o parser.o ast.o symbol_table.o semantic_analyzer.o intermediate_code_gen.o assembly_gen.o


LIB = liblogic.a

# Define main targets
.PHONY: all clean clean-everything frontend check-deps

# Main build target with all executables
all: $(LIB) lec_compiler Lec

-include $(OBJS:.o=.d)


# Bison rule
$(PARSER_C) $(PARSER_H): $(PARSER_Y)
	cd $(SRC_DIR) && bison -d -o parser.c parser.y

# Flex rule — now depends on parser.h!
$(LEXER_C): $(LEXER_L) $(PARSER_H)
	cd $(SRC_DIR) && flex -o lexer.c lexer.l

# Object files
lexer.o: $(LEXER_C)
	$(CC) $(CFLAGS) -o $@ $(LEXER_C)

parser.o: $(PARSER_C) $(PARSER_H) $(SRC_DIR)/ast.h
	$(CC) $(CFLAGS) -o $@ $(PARSER_C)

ast.o: $(AST_C) $(SRC_DIR)/ast.h $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -o $@ $(AST_C)


symbol_table.o: $(SYMBOL_TABLE_C) $(SYMBOL_TABLE_H)
	$(CC) $(CFLAGS) -o $@ $(SYMBOL_TABLE_C)

semantic_analyzer.o: $(SEMANTIC_ANALYZER_C) $(SRC_DIR)/semantic_analyzer.h
	$(CC) $(CFLAGS) -o $@ $(SEMANTIC_ANALYZER_C)

intermediate_code_gen.o: $(INTERMEDIATE_CODE_GEN_C) $(SRC_DIR)/intermediate_code_gen.h
	$(CC) $(CFLAGS) -o $@ $(INTERMEDIATE_CODE_GEN_C)

assembly_gen.o: $(ASSEMBLY_GEN_C) $(SRC_DIR)/assembly_gen.h
	$(CC) $(CFLAGS) -o $@ $(ASSEMBLY_GEN_C)

# Static library
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo "Checking for symbol table functions in library:"
	@nm $(LIB) | grep -E 'init_symbol_table|free_symbol_table|get_symbol_value|add_or_update_symbol'

clean:
	rm -f $(OBJS) $(LEXER_C) $(PARSER_C) $(PARSER_H) *.d *.o Lec lec_compiler

clean-everything: clean
	rm -f $(LIB)

frontend: check-deps
	go build

# Check dependencies
check-deps:
	@if ! command -v nasm > /dev/null || ! command -v bison > /dev/null || ! command -v flex > /dev/null; then \
		echo "Required dependencies missing. Running install script..."; \
		./install.sh; \
	fi

# Logical Expression Compiler executable
lec_compiler: lec_compiler.c $(LIB)
	$(CC) -g -Wall -o lec_compiler lec_compiler.c -I. -L. -llogic -lm

# Lec executable - now just a symbolic link to lec_compiler
Lec: lec_compiler
	ln -sf lec_compiler Lec
