# A Logic Expression Compiler

## Tools used and the function they perform

- `Flex` -- Used for Lexical Analysis.
- `Bison` -- Used for Parsing and building the AST.
- `C & Golang` -- Language used for Semantic analysis.

## Steps used to build it

- Installing the required tools; eg; flex, bison, rustc and go version 1.23+.
- Compiling lexer.l and parser.y to C components(Manual approach).

```
flex -o ./C_Components/lexer.c ./C_Components/lexer.l
bison -d -o ./C_Components/parser.c ./C_Components/parser.y
```

- Compiling the lexer, parser and ast c files and creating a static library.

```
gcc -I./C_Components -c ./C_Components/lexer.c ./C_Components/parser.c ./C_Components/ast.c && ar rcs liblogic.a lexer.o parser.o ast.o
```

- Use Makefile to compile by running(Better approach)

```
make
```

- Clean object files with

```
make clean
```

- Remove object files with the static linked library with

```
make clean-everything
```

- Fixing ambiguity of grammar by requiring parenthese for quantified expressions.

1. Added operator precedence and associativity declarations (from lowest to highest):

- %left IFF EQUIV (lowest)
  - %right IMPLIES
  - %left XOR XNOR
  - %left OR
  - %left AND
  - %right NOT EXISTS FORALL (highest)

2. Modified the quantifier rules to require parentheses:
   - EXISTS IDENTIFIER LPAREN expr RPAREN
   - FORALL IDENTIFIER LPAREN expr RPAREN
