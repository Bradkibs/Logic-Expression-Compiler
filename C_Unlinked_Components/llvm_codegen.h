#ifndef LLVM_CODEGEN_H
#define LLVM_CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/IRReader.h>
// Forward declaration of LLVM types
typedef struct LLVMOpaqueModule *LLVMModuleRef;

// Optimization function (static to be used only in this module)

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

// Function to generate LLVM IR from AST with optimization level
// The returned LLVMCodegenResult contains the generated module in the 'module' field
// optimization_level: 0 = no optimization, 1 = basic optimizations, 2 = more optimizations, 3 = aggressive optimizations
LLVMCodegenResult generate_llvm_ir(MultiStatementAST* multi_ast, SymbolTable* symbol_table, 
                                 const char* output_filename, int optimization_level);

// Function to compile and link the generated LLVM IR
LLVMCodegenResult compile_and_link_ir(const char* ir_filename, const char* output_filename);

// Function to save LLVM IR to a file
LLVMCodegenResult save_llvm_ir(LLVMModuleRef module, const char* filename);

// Cleanup function
void free_llvm_codegen_result(LLVMCodegenResult* result);

#endif /* LLVM_CODEGEN_H */
