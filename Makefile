CC = gcc
CFLAGS = -c -g -Wall -I./C_Components
AR = ar
ARFLAGS = rcs

SRC_DIR = C_Components

LEXER_L = $(SRC_DIR)/lexer.l
PARSER_Y = $(SRC_DIR)/parser.y
LEXER_C = $(SRC_DIR)/lexer.c
PARSER_C = $(SRC_DIR)/parser.c
PARSER_H = $(SRC_DIR)/parser.h
AST_C = $(SRC_DIR)/ast.c
PARSER_GLOBALS_C = $(SRC_DIR)/parser_globals.c

OBJS = lexer.o parser.o ast.o parser_globals.o

LIB = liblogic.a

all: $(LIB)

# Bison rule
$(PARSER_C) $(PARSER_H): $(PARSER_Y)
	bison -d -o $(PARSER_C) $(PARSER_Y)

# Flex rule — now depends on parser.h!
$(LEXER_C): $(LEXER_L) $(PARSER_H)
	cd $(SRC_DIR) && flex -o lexer.c lexer.l

# Object files
lexer.o: $(LEXER_C)
	$(CC) $(CFLAGS) -o $@ $<

parser.o: $(PARSER_C) $(PARSER_H)
	$(CC) $(CFLAGS) -o $@ $<

ast.o: $(AST_C) $(SRC_DIR)/ast.h
	$(CC) $(CFLAGS) -o $@ $<

parser_globals.o: $(PARSER_GLOBALS_C)
	$(CC) $(CFLAGS) -o $@ $<

# Static library
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
	rm -f $(OBJS) $(LEXER_C) $(PARSER_C) $(PARSER_H)

clean-everything: clean
	rm -f $(LIB)

.PHONY: all clean clean-everything
