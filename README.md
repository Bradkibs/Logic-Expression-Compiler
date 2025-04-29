# A Logic Expression Compiler

## Tools used and the function they perform

- `Flex` -- Used for Lexical Analysis.
- `Bison` -- Used for Parsing and building the AST.
- `C & Rust` -- Language used for Semantic analysis.

## Steps used to build it

- Installing the required tools; eg; flex, bison, rustc and cargo.
- Compiling lexer.l and parser.y to C components.

```
flex -o ./C_Components/lexer.c ./C_Components/lexer.l
bison -d -o ./C_Components/parser.c parser.y
```

- Compiling the lexer, parser and ast c files and creating a static library.

```
gcc -I./C_Components -c ./C_Components/lexer.c ./C_Components/parser.c ./C_Components/ast.c && ar rcs liblogic.a lexer.o parser.o ast.o
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
