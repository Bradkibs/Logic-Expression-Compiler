#include "llvm_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// LLVM C API headers
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/TargetMachine.h>

// Simple code generation context
typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMValueRef main_function;
    SymbolTable* symbol_table;
} LLVMContext;

// Helper function to create a context
static LLVMContext* create_context(const char* module_name, SymbolTable* symbol_table) {
    LLVMContext* ctx = (LLVMContext*)malloc(sizeof(LLVMContext));
    if (!ctx) return NULL;
    
    // Initialize LLVM components
    ctx->context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->context);
    ctx->symbol_table = symbol_table;
    
    // Create main function
    LLVMTypeRef main_func_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx->context), NULL, 0, 0);
    ctx->main_function = LLVMAddFunction(ctx->module, "main", main_func_type);
    
    // Create entry block for main
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, ctx->main_function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);
    
    return ctx;
}

// Free the context
static void free_context(LLVMContext* ctx) {
    if (!ctx) return;
    
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
    LLVMContextDispose(ctx->context);
    free(ctx);
}

// Generate a boolean constant
static LLVMValueRef gen_bool(LLVMContext* ctx, int value) {
    return LLVMConstInt(LLVMInt1TypeInContext(ctx->context), value ? 1 : 0, 0);
}

// Generate code for a variable reference
static LLVMValueRef gen_var(LLVMContext* ctx, const char* name) {
    // Check for TRUE/FALSE literals
    if (strcmp(name, "TRUE") == 0) {
        return gen_bool(ctx, 1);
    }
    if (strcmp(name, "FALSE") == 0) {
        return gen_bool(ctx, 0);
    }
    
    // Look up in symbol table
    int value = get_symbol_value(ctx->symbol_table, name);
    if (value == ERROR_SYMBOL_NOT_FOUND) {
        fprintf(stderr, "Error: Undefined variable '%s'\n", name);
        return NULL;
    }
    
    return gen_bool(ctx, value);
}

// Generate code for logical NOT
static LLVMValueRef gen_not(LLVMContext* ctx, LLVMValueRef operand) {
    return LLVMBuildNot(ctx->builder, operand, "not_result");
}

// Generate code for logical AND
static LLVMValueRef gen_and(LLVMContext* ctx, LLVMValueRef left, LLVMValueRef right) {
    return LLVMBuildAnd(ctx->builder, left, right, "and_result");
}

// Generate code for logical OR
static LLVMValueRef gen_or(LLVMContext* ctx, LLVMValueRef left, LLVMValueRef right) {
    return LLVMBuildOr(ctx->builder, left, right, "or_result");
}

// Generate code for logical XOR
static LLVMValueRef gen_xor(LLVMContext* ctx, LLVMValueRef left, LLVMValueRef right) {
    return LLVMBuildXor(ctx->builder, left, right, "xor_result");
}

// Generate code for IMPLIES (a -> b is equivalent to !a || b)
static LLVMValueRef gen_implies(LLVMContext* ctx, LLVMValueRef left, LLVMValueRef right) {
    LLVMValueRef not_left = gen_not(ctx, left);
    return gen_or(ctx, not_left, right);
}

// Generate code for IFF (a <-> b is equivalent to (a && b) || (!a && !b))
static LLVMValueRef gen_iff(LLVMContext* ctx, LLVMValueRef left, LLVMValueRef right) {
    LLVMValueRef not_left = gen_not(ctx, left);
    LLVMValueRef not_right = gen_not(ctx, right);
    
    LLVMValueRef left_and_right = gen_and(ctx, left, right);
    LLVMValueRef not_left_and_not_right = gen_and(ctx, not_left, not_right);
    
    return gen_or(ctx, left_and_right, not_left_and_not_right);
}

// Forward declaration for recursive AST traversal
static LLVMValueRef gen_expression(LLVMContext* ctx, Node* node);

// Generate code for assignment
static LLVMValueRef gen_assignment(LLVMContext* ctx, Node* node) {
    if (!node->name) {
        fprintf(stderr, "Error: Assignment node missing variable name\n");
        return NULL;
    }
    
    // Generate code for the right-hand side
    LLVMValueRef rhs = NULL;
    if (node->right) {
        rhs = gen_expression(ctx, node->right);
    } else if (node->left) {
        rhs = gen_expression(ctx, node->left);
    }
    
    if (!rhs) {
        fprintf(stderr, "Error: Assignment to %s has invalid right-hand side\n", node->name);
        return NULL;
    }
    
    // Update the symbol table
    int value = LLVMConstIntGetZExtValue(rhs);
    add_or_update_symbol(ctx->symbol_table, node->name, value);
    
    return rhs;
}

// Generate code for an expression (AST node)
static LLVMValueRef gen_expression(LLVMContext* ctx, Node* node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case NODE_BOOL:
            return gen_bool(ctx, node->bool_val);
            
        case NODE_VAR:
            return gen_var(ctx, node->name);
            
        case NODE_ASSIGN:
            return gen_assignment(ctx, node);
            
        case NODE_NOT:
            if (!node->left) {
                fprintf(stderr, "Error: NOT node missing operand\n");
                return NULL;
            }
            return gen_not(ctx, gen_expression(ctx, node->left));
            
        case NODE_AND:
            if (!node->left || !node->right) {
                fprintf(stderr, "Error: AND node missing operands\n");
                return NULL;
            }
            return gen_and(ctx, gen_expression(ctx, node->left), gen_expression(ctx, node->right));
            
        case NODE_OR:
            if (!node->left || !node->right) {
                fprintf(stderr, "Error: OR node missing operands\n");
                return NULL;
            }
            return gen_or(ctx, gen_expression(ctx, node->left), gen_expression(ctx, node->right));
            
        case NODE_XOR:
            if (!node->left || !node->right) {
                fprintf(stderr, "Error: XOR node missing operands\n");
                return NULL;
            }
            return gen_xor(ctx, gen_expression(ctx, node->left), gen_expression(ctx, node->right));
            
        case NODE_IMPLIES:
            if (!node->left || !node->right) {
                fprintf(stderr, "Error: IMPLIES node missing operands\n");
                return NULL;
            }
            return gen_implies(ctx, gen_expression(ctx, node->left), gen_expression(ctx, node->right));
            
        case NODE_IFF:
        case NODE_EQUIV:
            if (!node->left || !node->right) {
                fprintf(stderr, "Error: IFF/EQUIV node missing operands\n");
                return NULL;
            }
            return gen_iff(ctx, gen_expression(ctx, node->left), gen_expression(ctx, node->right));
            
        default:
            fprintf(stderr, "Error: Unsupported node type %d\n", node->type);
            return NULL;
    }
}

// Add code to print variable values and results
static void add_print_code(LLVMContext* ctx, SymbolTable* symbol_table, LLVMValueRef result) {
    // Declare external printf function
    LLVMTypeRef printf_param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0) };
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx->context), printf_param_types, 1, 1);
    LLVMValueRef printf_func = LLVMAddFunction(ctx->module, "printf", printf_type);
    
    // Create format strings
    LLVMValueRef var_format_str = LLVMBuildGlobalStringPtr(ctx->builder, "Variable %s = %s\n", "var_fmt");
    LLVMValueRef result_format_str = LLVMBuildGlobalStringPtr(ctx->builder, "Result: %s\n", "result_fmt");
    LLVMValueRef true_str = LLVMBuildGlobalStringPtr(ctx->builder, "TRUE", "true_str");
    LLVMValueRef false_str = LLVMBuildGlobalStringPtr(ctx->builder, "FALSE", "false_str");
    
    // Print all variables
    for (int i = 0; i < symbol_table->size; i++) {
        if (symbol_table->symbols[i].name[0] == '\0') continue;
        
        LLVMValueRef var_name = LLVMBuildGlobalStringPtr(ctx->builder, symbol_table->symbols[i].name, "var_name");
        LLVMValueRef var_value = symbol_table->symbols[i].value ? true_str : false_str;
        
        LLVMValueRef args[] = { var_format_str, var_name, var_value };
        LLVMBuildCall2(ctx->builder, printf_type, printf_func, args, 3, "");
    }
    
    // Print the result if it exists
    if (result) {
        // Convert i1 to string
        LLVMValueRef is_true = LLVMBuildICmp(ctx->builder, LLVMIntNE, result, 
                                            LLVMConstInt(LLVMInt1TypeInContext(ctx->context), 0, 0), 
                                            "is_true");
        LLVMValueRef value_str = LLVMBuildSelect(ctx->builder, is_true, true_str, false_str, "value_str");
        
        LLVMValueRef args[] = { result_format_str, value_str };
        LLVMBuildCall2(ctx->builder, printf_type, printf_func, args, 2, "");
    }
}

// Generate LLVM IR for an AST
LLVMCodegenResult generate_llvm_ir(Node* ast, SymbolTable* symbol_table, const char* output_filename) {
    LLVMCodegenResult result = {LLVM_CODEGEN_OK, NULL, NULL};
    
    // Initialize LLVM targets
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    
    // Create context
    LLVMContext* ctx = create_context("logic_module", symbol_table);
    if (!ctx) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to create LLVM context");
        return result;
    }
    
    // Generate code for the expression
    LLVMValueRef expr_result = gen_expression(ctx, ast);
    
    // Add code to print variables and result
    add_print_code(ctx, symbol_table, expr_result);
    
    // Add return 0 to main
    LLVMBuildRet(ctx->builder, LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, 0));
    
    // Verify the module
    char* error = NULL;
    if (LLVMVerifyModule(ctx->module, LLVMPrintMessageAction, &error)) {
        fprintf(stderr, "Module verification error: %s\n", error);
        LLVMDisposeMessage(error);
        
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Module verification failed");
        free_context(ctx);
        return result;
    }
    
    // Create output filename
    char bitcode_filename[256];
    sprintf(bitcode_filename, "%s.bc", output_filename);
    
    // Write bitcode to file
    if (LLVMWriteBitcodeToFile(ctx->module, bitcode_filename)) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to write bitcode to file");
        free_context(ctx);
        return result;
    }
    
    // Clean up
    free_context(ctx);
    
    // Return success
    result.output_file = strdup(bitcode_filename);
    return result;
}

// Compile and link the IR file to create an executable
LLVMCodegenResult compile_and_link_ir(const char* ir_filename, const char* output_filename) {
    LLVMCodegenResult result = {LLVM_CODEGEN_OK, NULL, NULL};
    
    // Command to compile and link using clang
    char command[512];
    sprintf(command, "clang %s -o %s", ir_filename, output_filename);
    
    // Execute the command
    int ret = system(command);
    if (ret != 0) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to compile and link bitcode file");
        return result;
    }
    
    result.output_file = strdup(output_filename);
    return result;
}

void free_llvm_codegen_result(LLVMCodegenResult* result) {
    if (!result) return;
    
    if (result->error_message) {
        free(result->error_message);
        result->error_message = NULL;
    }
    
    if (result->output_file) {
        free(result->output_file);
        result->output_file = NULL;
    }
}
