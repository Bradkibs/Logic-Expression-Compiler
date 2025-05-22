#ifndef LLVM_CODEGEN_H
#define LLVM_CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include "multi_statement.h"

// Error codes for LLVM code generation
typedef enum {
    LLVM_CODEGEN_OK,
    LLVM_CODEGEN_ERROR,
    LLVM_CODEGEN_SYMBOL_ERROR,
    LLVM_CODEGEN_AST_ERROR,
    LLVM_CODEGEN_FILE_ERROR
} LLVMCodegenErrorCode;

// Result structure
typedef struct {
    LLVMCodegenErrorCode error_code;
    char* error_message;  // Error message if any
    char* output_file;    // Path to the generated file
} LLVMCodegenResult;

// Function to generate LLVM IR from AST
LLVMCodegenResult generate_llvm_ir(MultiStatementAST* multi_ast, SymbolTable* symbol_table, const char* output_filename);

// Function to compile and link the generated LLVM IR
LLVMCodegenResult compile_and_link_ir(const char* ir_filename, const char* output_filename);

// Cleanup function
void free_llvm_codegen_result(LLVMCodegenResult* result);

#endif /* LLVM_CODEGEN_H */
