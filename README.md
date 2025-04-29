# A Logic Expression Compiler

## Tools used and the function they perform

- `Flex` -- Used for Lexical Analysis.
- `Bison` -- Used for Parsing and building the AST.
- `C & Rust` -- Language used for Semantic analysis.

## Steps used to build it

- Installing the required tools; eg; flex, bison, rustc and cargo.
- Compiling lexer.l and parser.y to C components.

```
flex -o ./C_Components/lexer.c lexer.l
bison -d -o ./C_Components/parser.c parser.y
```

- Compiling the lexer, parser and ast c files and creating a static library.

```
gcc -c ./C_Components/lexer.c p./C_Components/arser.c ./C_Components/ast.c
ar rcs liblogic.a lexer.o parser.o ast.o
```
