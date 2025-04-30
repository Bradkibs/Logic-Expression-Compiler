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

### Running the project(works only on linux for now).

1. First install golang v 1.23+
2. Clone the repo to your linux machine
3. enable CGO
4. Run `make` command
5. Run `make clean` command
6. Run `go build` ot `make frontend`
7. Write a logical expression using capital letters in assignment operations eg; A = TRUE etc.
8. Save your written expressions in a `.lec` file.
9. Run the executable generated from go build with the .lec file.
10. The output of step by step evaluation will be at a file called output.txt
11. Evaluate the answer of the expression.
12. Leave a github star

### Stack built tools

#### Frontend

1. Golang

#### Backend

1. flex i.e lexer.l
2. bison i.e parser.y
3. C i.e ast.c and ast.h

#### compilation toolchain

1. gcc
2. created a makefile to simplify the steps.
