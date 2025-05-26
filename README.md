# Logical Expression Compiler

A compiler for logical expressions that supports parsing, semantic analysis, and code generation. The compiler can handle various logical operators and generate intermediate representations or assembly code.

## Features

- **Lexical Analysis**: Tokenizes input using Flex
- **Parsing**: Builds an Abstract Syntax Tree (AST) using Bison
- **Semantic Analysis**: Validates expressions and checks for semantic correctness
- **LLVM Backend**: Generates optimized machine code using LLVM
- **Efficient Execution**: Produces optimized native executables
- **Cross-Platform**: Works on any platform with LLVM support

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

- GCC and Clang
- Flex
- Bison
- LLVM (for LLVM backend)
- Make

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/Bradkibs/Logic-Expression-Compiler.git
   cd Logic-Expression-Compiler
   ```

2. Install dependencies using the provided install script:
   ```bash
   chmod +x install.sh
   ./install.sh
   ```
   
   This will automatically detect your OS and install the required dependencies (Flex, Bison, GCC, NASM, and Go).

3. Build the compiler:
   ```bash
   make -f Makefile.llvm
   ```

   This will create the `lec_compiler_llvm` executable.

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

## Usage

### Basic Usage

1. Create a file with your logical expressions (e.g., `example.lec`):
   ```
   A = TRUE
   B = FALSE
   C = A AND B
   C XOR A
   ```

2. Compile and run:
   ```bash
   ./lec_compiler_llvm example.lec
   ./output
   ```

### Optimization Levels

The compiler supports different optimization levels (0-3) using the `-oN` flag, where N is the optimization level:

- `-o0`: No optimization (default)
- `-o1`: Basic optimizations
- `-o2`: More aggressive optimizations
- `-o3`: Maximum optimizations

Example:
```bash
# Compile with maximum optimizations
./lec_compiler_llvm example.lec -o3

# The output will be in 'output' by default
./output
```

### Output Files

- `output`: The compiled executable
- `output.ll`: Generated LLVM IR (Intermediate Representation)
- `output.bc`: LLVM bitcode (intermediate format)

## Compiler Architecture

The Logical Expression Compiler uses LLVM for efficient code generation and optimization. Here's the compilation pipeline:

1. **Frontend**
   - **Lexical Analysis** (`lexer.l`)
     - Tokenizes input into meaningful tokens
     - Recognizes keywords, operators, and variables
     - Handles whitespace and comments
   
   - **Syntax Analysis** (`parser.y`)
     - Parses tokens into an Abstract Syntax Tree (AST)
     - Validates grammar and syntax rules
     - Manages operator precedence and associativity

2. **Semantic Analysis** (`semantic_analyzer.c/h`)
   - Performs type checking and validation
   - Manages symbol table and variable scopes
   - Ensures semantic correctness of expressions

3. **LLVM Backend** (`llvm_codegen.c/h`)
   - **IR Generation**: Converts AST to LLVM Intermediate Representation
   - **Optimization**: Applies LLVM optimization passes
   - **Code Generation**: Produces efficient machine code
   - **Linking**: Creates standalone executables

4. **Runtime**
   - Manages program execution
   - Handles I/O operations
   - Provides standard library functions

## LLVM Integration

The compiler leverages LLVM's powerful optimization and code generation capabilities:

- **LLVM IR Generation**: The `llvm_codegen.c` module translates the AST into LLVM IR
- **Optimization Passes**: Multiple optimization levels are available
- **Target Support**: Generates code for various architectures
- **JIT Compilation**: Supports just-in-time compilation (future)

## Development Workflow

1. **Setup Development Environment**
   ```bash
   # Install LLVM and build tools
   sudo apt-get install llvm-18 flex bison
   
   # Build the compiler with LLVM support
   make -f Makefile.llvm
   ```

2. **Testing**
   - Add test cases in `test_*.lec` files
   - Run tests: `make test`
   - Debug with LLVM tools: `opt`, `llc`, `lli`
   - Use `lldb` for debugging generated code

3. **Extending the Compiler**
   - Add new operators in `lexer.l` and `parser.y`
   - Extend the AST in `ast.h/c`
   - Update semantic analysis in `semantic_analyzer.c/h`
   - Add LLVM code generation in `llvm_codegen.c/h`

4. **Performance Optimization**
   - Use `opt` to analyze and optimize LLVM IR
   - Profile with `perf` or `valgrind`
   - Add custom LLVM optimization passes if needed

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
expr1 = ((A AND B) OR C)
expr2 = NOT A AND B
expr3 = ((A -> B) -> C)
expr4 = (A OR B) AND C)()
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
- `C_Unlinked_Components/llvm_codegen.c.bak`, `C_Unlinked_Components/llvm_codegen.c.complex`

## License

GPL v3
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
