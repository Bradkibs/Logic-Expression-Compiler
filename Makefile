CC = gcc
CFLAGS = -c -fPIC -g -DYYDEBUG=1 -I./C_Components
LDFLAGS = -shared

SRC_DIR = C_Components

LEXER_L = $(SRC_DIR)/lexer.l
LEXER_C = $(SRC_DIR)/lexer.c
PARSER_C = $(SRC_DIR)/parser.c
AST_C = $(SRC_DIR)/ast.c
PARSER_GLOBALS_C = $(SRC_DIR)/parser_globals.c

OBJS = lexer.o parser.o ast.o parser_globals.o

LIB = liblogic.so

all: $(LIB)

$(LEXER_C): $(LEXER_L)
	cd $(SRC_DIR) && flex lexer.l && mv lex.yy.c lexer.c

lexer.o: $(LEXER_C)
	$(CC) $(CFLAGS) $< -o $@

parser.o: $(PARSER_C)
	$(CC) $(CFLAGS) $< -o $@

ast.o: $(AST_C)
	$(CC) $(CFLAGS) $< -o $@

parser_globals.o: $(PARSER_GLOBALS_C)
	$(CC) $(CFLAGS) $< -o $@

$(LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(LIB) $(LEXER_C)

.PHONY: all clean
