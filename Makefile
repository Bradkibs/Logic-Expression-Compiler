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

OBJS = lexer.o parser.o ast.o  symbol_table.o



LIB = liblogic.a

all: $(LIB)

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

# Static library
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo "Checking for symbol table functions in library:"
	@nm $(LIB) | grep -E 'init_symbol_table|free_symbol_table|get_symbol_value|add_or_update_symbol'

clean:
	rm -f $(OBJS) $(LEXER_C) $(PARSER_C) $(PARSER_H) *.d

clean-everything: clean
	rm -f $(LIB)

frontend:
	go build
.PHONY: all clean clean-everything