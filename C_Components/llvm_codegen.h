#ifndef LLVM_CODEGEN_H
#define LLVM_CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/IRReader.h>

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
    char* error_message;    // Error message if any
    char* output_file;      // Path to the generated file
    LLVMModuleRef module;   // The generated LLVM module (if any)
} LLVMCodegenResult;

// Function to generate LLVM IR from AST
// The returned LLVMCodegenResult contains the generated module in the 'module' field
LLVMCodegenResult generate_llvm_ir(MultiStatementAST* multi_ast, SymbolTable* symbol_table, const char* output_filename);

// Function to compile and link the generated LLVM IR
LLVMCodegenResult compile_and_link_ir(const char* ir_filename, const char* output_filename);

// Function to save LLVM IR to a file
LLVMCodegenResult save_llvm_ir(LLVMModuleRef module, const char* filename);

// Cleanup function
void free_llvm_codegen_result(LLVMCodegenResult* result);

#endif /* LLVM_CODEGEN_H */
