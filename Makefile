
CC = gcc
CFLAGS = -c
INCLUDES = -I./C_Components


SRC_DIR = C_Components


LEXER_L = $(SRC_DIR)/lexer.l
LEXER_C = $(SRC_DIR)/lexer.c
PARSER_C = $(SRC_DIR)/parser.c
AST_C = $(SRC_DIR)/ast.c


OBJS = lexer.o parser.o ast.o


LIB = liblogic.a


all: $(LIB)

# Generating lexer.c from lexer.l
$(LEXER_C): $(LEXER_L)
	cd $(SRC_DIR) && flex lexer.l && mv lex.yy.c lexer.c

# Compilingg the source files
lexer.o: $(LEXER_C)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

parser.o: $(PARSER_C)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

ast.o: $(AST_C)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Creating a static library
$(LIB): $(OBJS)
	ar rcs $@ $(OBJS)


clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean

