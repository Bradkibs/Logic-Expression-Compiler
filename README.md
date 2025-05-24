# Logical Expression Compiler

A compiler for logical expressions that supports parsing, semantic analysis, and code generation. The compiler can handle various logical operators and generate intermediate representations or assembly code.

## Features

- **Lexical Analysis**: Tokenizes input using Flex
- **Parsing**: Builds an Abstract Syntax Tree (AST) using Bison
- **Semantic Analysis**: Validates expressions and checks for semantic correctness
- **Intermediate Code Generation**: Converts AST to Three-Address Code (TAC)
- **Assembly Generation**: Produces NASM x86-64 assembly code
- **LLVM Backend**: Optional LLVM-based code generation
- **Multiple Output Formats**: Can generate executable binaries or assembly code

## Supported Logical Operators

- **Basic Operators**:
  - AND (`AND`, `&&`)
  - OR (`OR`, `||`)
  - NOT (`NOT`, `~`)
  - XOR (`XOR`)
  - XNOR (`XNOR`)
  - IMPLIES (`->`, `==>`, `IMPLIES`)
  - IFF (`<->`, `<=>`, `DOUBLEIMPLIES`)
  - EQUIV (`EQUIVALENT`, `===`)

- **Quantifiers**:
  - EXISTS (`EXISTS`, `E_Q`)
  - FORALL (`FORALL`, `U_Q`)

## Prerequisites

- GCC or Clang
- Flex
- Bison
- NASM (for assembly generation)
- LLVM (optional, for LLVM backend)
- Go 1.16+ (for some build scripts)

## Installation

1. Clone the repository:
   ```bash
   git clone [repository-url]
   cd LogicalExpressionsCompiler
   ```

2. Install dependencies:
   - On Ubuntu/Debian:
     ```bash
     sudo apt-get update
     sudo apt-get install flex bison nasm golang llvm
     ```
   - On macOS (using Homebrew):
     ```bash
     brew install flex bison nasm go llvm
     ```

## Building the Project

### Prerequisites

- LLVM development libraries
- Flex (lexer generator)
- Bison (parser generator)
- GCC or Clang
- Make

### Build Process

The project uses LLVM for code generation. To build:

```bash
# Clone the repository (if not already done)
git clone [repository-url]
cd LogicalExpressionsCompiler

# Install dependencies (Ubuntu/Debian)
sudo apt-get install llvm flex bison

# Build the compiler
make -f Makefile.llvm
```

This will create the following files:
- `liblogic_llvm.a` - Static library with core components
- `lec_compiler_llvm` - Main compiler executable

### Cleanup

To clean build artifacts:

```bash
make -f Makefile.llvm clean
```

### Handling Ambiguity with Parentheses

To ensure expressions are evaluated as intended, it's important to use parentheses to explicitly define the order of operations, especially for non-associative operators. The compiler enforces this to prevent ambiguous expressions.

### Parenthesization Rules

1. **Top-level Expressions**:
   - Always enclose the entire expression in parentheses
   - Example: `(A AND B)` instead of `A AND B`

2. **Operator Precedence**:
   - `NOT` has the highest precedence
   - `AND` has higher precedence than `OR`
   - `IMPLIES`, `IFF`, and `EQUIV` have the lowest precedence
   - When in doubt, use parentheses to make the intended order explicit

3. **Associative Operations**:
   - For associative operators (AND, OR, XOR, XNOR), use parentheses to group operations with the same precedence
   - Example: `((A AND B) AND C)` instead of `A AND B AND C`

4. **Non-Associative Operations**:
   - For non-associative operators (IMPLIES, IFF, EQUIV), always use parentheses to specify the evaluation order
   - Example: `(A IMPLIES (B EQUIV C))` instead of `A IMPLIES B EQUIV C`

### Examples

**Ambiguous (will cause an error):**
```
A AND B OR C
```

**Unambiguous (preferred):**
```
((A AND B) OR C)
```

**More complex example with multiple operators:**
```
((NOT A) AND (B OR C)) IMPLIES (D EQUIV (E AND F))
```

## Running the Compiler

### Basic Usage
```bash
# Compile a .lec file (output will be named 'output')
./lec_compiler_llvm input.lec

# Compile with custom output filename
./lec_compiler_llvm input.lec my_program

# Run the compiled program
./output  # or ./my_program if you specified a custom name
```

### Command Line Options
- `input.lec`: Required. The input file containing logical expressions
- `output_file`: Optional. The name of the output executable (default: 'output')

## Compiler Development Phases

The Logical Expression Compiler follows these standard compiler phases, each implemented as a separate component:

1. **Lexical Analysis** (`lexer.l`)
   - Tokenizes input characters into meaningful tokens
   - Recognizes keywords, operators, variables, and literals
   - Handles whitespace and comments

2. **Syntax Analysis** (`parser.y`)
   - Parses tokens into an Abstract Syntax Tree (AST)
   - Validates grammar and syntax
   - Handles operator precedence and associativity

3. **Semantic Analysis** (`semantic_analyzer.c/h`)
   - Performs type checking
   - Validates variable declarations and usage
   - Checks for undefined variables and type mismatches

4. **Intermediate Code Generation**
   - Converts AST to Three-Address Code (TAC)
   - Handles temporary variables and labels
   - Prepares for target code generation

5. **Code Generation** (`llvm_codegen.c/h`)
   - Generates LLVM Intermediate Representation (IR)
   - Optimizes the generated code
   - Produces target machine code

6. **Linking & Execution**
   - Links with runtime libraries
   - Produces an executable binary
   - Handles program execution

## Development Workflow

1. **Setup Development Environment**
   ```bash
   # Install dependencies
   sudo apt-get install llvm flex bison
   
   # Build the compiler
   make -f Makefile.llvm
   ```

2. **Testing**
   - Add test cases in `test_*.lec` files
   - Run tests: `make test`
   - Debug using `gdb` or `lldb`

3. **Adding New Features**
   - Update lexer for new tokens
   - Extend the grammar in the parser
   - Implement semantic analysis
   - Add code generation support

4. **Optimization**
   - Profile the compiler with `perf` or `valgrind`
   - Optimize critical paths
   - Add compiler optimization passes

## Example Development Cycle

1. **Add a new operator** (e.g., NAND):
   - Add token to `lexer.l`
   - Update grammar in `parser.y`
   - Add semantic checks
   - Implement code generation
   - Add test cases

2. **Optimize performance**:
   - Profile the compiler
   - Identify bottlenecks
   - Implement optimizations
   - Verify correctness with tests

3. **Debug an issue**:
   - Reproduce with minimal test case
   - Add debug logging
   - Fix the issue
   - Add regression test

## Running Tests

1. Simple test cases:
   ```bash
   make test
   ```

2. Run specific test file:
   ```bash
   ./lec_compiler test.lec
   # or for LLVM version
   ./lec_compiler_llvm test.lec
   ```

## Testing the Compiler

The repository includes several test files that demonstrate proper parenthesization and test various aspects of the compiler's functionality:

### Test Files

### Running All Tests

To run all test files and verify the output:

```bash
for test_file in test_*.lec; do
    echo "\nTesting $test_file..."
    echo "Input:"
    cat "$test_file"
    echo -e "\nCompiling..."
    ./lec_compiler_llvm "$test_file"
    if [ -f "./output" ]; then
        echo -e "\nOutput of $test_file:"
        ./output
    fi
done
```

### Example Test File (test_parenthesized.lec)

This file demonstrates proper parenthesization for complex expressions:

```
A = TRUE
B = FALSE
C = TRUE
D = FALSE

((A AND B) OR (C AND D))
((A OR B) AND (C OR D))
((NOT A) AND (B OR C))
((A IMPLIES B) EQUIV (C IMPLIES D))
((A AND (B OR C)) IMPLIES (D EQUIV (A AND C)))
```

Each expression is fully parenthesized to ensure the intended order of evaluation, making the code more readable and eliminating any potential ambiguity.

## Usage

### Basic Usage

```bash
./lec_compiler input.lec
```

### Example Input File

```
/* Define variables */
A = TRUE
B = FALSE
C = TRUE

/* Test expressions */
expr1 = A AND B OR C
expr2 = NOT A AND B
expr3 = A -> B -> C
expr4 = (A OR B) AND C
```

### Output

The compiler will generate an executable that, when run, will evaluate the expressions and print the results.

## Project Structure

### Core Components (C_Components/)
- `lexer.l` - Flex lexer definition (tokenizes input)
- `parser.y` - Bison parser definition (builds AST from tokens)
- `ast.[ch]` - Abstract Syntax Tree implementation
- `symbol_table.[ch]` - Manages variables and their values
- `semantic_analyzer.[ch]` - Validates expressions and checks semantics
- `llvm_codegen.[ch]` - Generates LLVM IR from AST
- `node_to_string.c` - Converts AST nodes to string representation
- `multi_statement.[ch]` - Handles multiple statements in source files

### Main Executable
- `lec_compiler_llvm.c` - Main compiler driver using LLVM backend
- `Makefile.llvm` - Build configuration for LLVM version

### Test Files
- `test.lec` - Basic test cases
- `test_ambiguous.lec` - Tests for ambiguous expressions
- `test_precedence.lec` - Operator precedence tests
- `test_parenthesized.lec` - Parenthesized expression tests

### Build Artifacts
- `liblogic_llvm.a` - Static library of core components
- `lec_compiler_llvm` - Final compiler executable

## Cleanup

To clean up build artifacts:

```bash
make clean          # Clean object files
make clean-everything  # Clean everything including libraries
```

## Unused Files

The following files can be safely removed as they appear to be test outputs or temporary files:
- `test1`, `test_complex`, `test_custom1`, `test_custom_llvm`
- `test_fixed`, `test_llvm`, `test_precedence`, `test_single`
- `test_uno`, `output`, `output.asm`, `output.bc`, `output.txt`
- `C_Components/llvm_codegen.c.bak`, `C_Components/llvm_codegen.c.complex`

## License

[Specify your license here]
  - %right NOT EXISTS FORALL (highest)

2. Modified the quantifier rules to require parentheses:
   - EXISTS IDENTIFIER LPAREN expr RPAREN
   - FORALL IDENTIFIER LPAREN expr RPAREN

### Running the project(works only on linux for now).

1. First install golang v 1.23+
2. Clone the repository to your Linux machine
3. Install the required dependencies (see Installation section)
4. Build the compiler: `make -f Makefile.llvm`
5. Run the compiler with test files: `./lec_compiler_llvm test_custom_vars.lec`
6. Execute the generated program: `./output`
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
3. C i.e ast.c and ast.h symbol_table.c and symbol_table.h

#### compilation toolchain

1. gcc
2. created a makefile to simplify the steps.
