#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For mkdtemp
#include <libgen.h>  // For dirname
#include "ast.h"
#include "symbol_table.h"
#include <limits.h>  // For PATH_MAX

// LLVM C API headers
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/IRReader.h>

#include "llvm_codegen.h"

// Forward declarations
static LLVMValueRef gen_expression(LLVMContextRef context, LLVMBuilderRef builder, 
                                  Node* node, SymbolTable* symbol_table, 
                                  LLVMValueRef true_str, LLVMValueRef false_str,
                                  LLVMValueRef printf_func, LLVMTypeRef printf_type);

// Optimization function declaration
static void optimize_module(LLVMModuleRef module, int opt_level);

// Function to save LLVM IR to a file
LLVMCodegenResult save_llvm_ir(LLVMModuleRef module, const char* filename);

// Generate detailed evaluation messages
static void add_evaluation_message(LLVMBuilderRef builder, LLVMValueRef printf_func, 
                                  LLVMTypeRef printf_type, const char* format, ...) {
    LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, format, "fmt_str");
    LLVMValueRef args[] = { fmt_str };
    LLVMBuildCall2(builder, printf_type, printf_func, args, 1, "");
}

// Generate a detailed output message for variable substitution
static void add_var_substitution_message(LLVMBuilderRef builder, LLVMValueRef printf_func, 
                                       LLVMTypeRef printf_type, const char* var_name, 
                                       LLVMValueRef true_str, LLVMValueRef false_str, 
                                       int value) {
    LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, "Substituted variable %s with value %s\n", "var_subst_fmt");
    LLVMValueRef name_str = LLVMBuildGlobalStringPtr(builder, var_name, "var_name");
    LLVMValueRef value_str = value ? true_str : false_str;
    
    LLVMValueRef args[] = { fmt_str, name_str, value_str };
    LLVMBuildCall2(builder, printf_type, printf_func, args, 3, "");
}

// Generate code for a logical expression with detailed output
static LLVMValueRef gen_expression(LLVMContextRef context, LLVMBuilderRef builder, 
                                  Node* node, SymbolTable* symbol_table,
                                  LLVMValueRef true_str, LLVMValueRef false_str,
                                  LLVMValueRef printf_func, LLVMTypeRef printf_type) {
    if (!node) return NULL;
    
    LLVMValueRef left, right;
    
    switch (node->type) {
        case NODE_BOOL:
            return LLVMConstInt(LLVMInt1Type(), node->bool_val, 0);
            
        case NODE_VAR:
            // Handle TRUE/FALSE literals
            if (strcmp(node->name, "TRUE") == 0) {
                return LLVMConstInt(LLVMInt1Type(), 1, 0);
            }
            if (strcmp(node->name, "FALSE") == 0) {
                return LLVMConstInt(LLVMInt1Type(), 0, 0);
            }
            
            // Look up in symbol table
            int value = get_symbol_value(symbol_table, node->name);
            if (value == ERROR_SYMBOL_NOT_FOUND) {
                fprintf(stderr, "Error: Undefined variable '%s'\n", node->name);
                return NULL;
            }
            
            // Add substitution message
            add_var_substitution_message(builder, printf_func, printf_type, 
                                       node->name, true_str, false_str, value);
            
            return LLVMConstInt(LLVMInt1Type(), value, 0);
            
        case NODE_NOT:
            left = gen_expression(context, builder, node->left, symbol_table, 
                                true_str, false_str, printf_func, printf_type);
            if (!left) return NULL;
            
            // Add evaluation message
            add_evaluation_message(builder, printf_func, printf_type, "Evaluated NOT operation\n");
            
            return LLVMBuildNot(builder, left, "not");
            
        case NODE_AND:
            left = gen_expression(context, builder, node->left, symbol_table, 
                                true_str, false_str, printf_func, printf_type);
            right = gen_expression(context, builder, node->right, symbol_table, 
                                 true_str, false_str, printf_func, printf_type);
            if (!left || !right) return NULL;
            
            // Add evaluation message
            add_evaluation_message(builder, printf_func, printf_type, "Evaluated AND operation\n");
            
            return LLVMBuildAnd(builder, left, right, "and");
            
        case NODE_OR:
            left = gen_expression(context, builder, node->left, symbol_table, 
                                true_str, false_str, printf_func, printf_type);
            right = gen_expression(context, builder, node->right, symbol_table, 
                                 true_str, false_str, printf_func, printf_type);
            if (!left || !right) return NULL;
            
            // Add evaluation message
            add_evaluation_message(builder, printf_func, printf_type, "Evaluated OR operation\n");
            
            return LLVMBuildOr(builder, left, right, "or");
            
        case NODE_XOR:
            left = gen_expression(context, builder, node->left, symbol_table, 
                                true_str, false_str, printf_func, printf_type);
            right = gen_expression(context, builder, node->right, symbol_table, 
                                 true_str, false_str, printf_func, printf_type);
            if (!left || !right) return NULL;
            
            // Add evaluation message
            add_evaluation_message(builder, printf_func, printf_type, "Evaluated XOR operation\n");
            
            return LLVMBuildXor(builder, left, right, "xor");
            
        case NODE_IMPLIES: {
            left = gen_expression(context, builder, node->left, symbol_table, 
                                true_str, false_str, printf_func, printf_type);
            right = gen_expression(context, builder, node->right, symbol_table, 
                                 true_str, false_str, printf_func, printf_type);
            if (!left || !right) return NULL;
            
            // Add evaluation message
            add_evaluation_message(builder, printf_func, printf_type, "Evaluated IMPLIES operation\n");
            
            // a -> b is equivalent to !a || b
            LLVMValueRef not_left = LLVMBuildNot(builder, left, "not_left");
            return LLVMBuildOr(builder, not_left, right, "implies");
        }
            
        case NODE_IFF:
        case NODE_EQUIV: {
            left = gen_expression(context, builder, node->left, symbol_table, 
                                true_str, false_str, printf_func, printf_type);
            right = gen_expression(context, builder, node->right, symbol_table, 
                                 true_str, false_str, printf_func, printf_type);
            if (!left || !right) return NULL;
            
            // Add evaluation message
            add_evaluation_message(builder, printf_func, printf_type, "Evaluated IFF/EQUIV operation\n");
            
            // a <-> b is equivalent to (a && b) || (!a && !b)
            LLVMValueRef left_and_right = LLVMBuildAnd(builder, left, right, "left_and_right");
            
            LLVMValueRef not_left = LLVMBuildNot(builder, left, "not_left");
            LLVMValueRef not_right = LLVMBuildNot(builder, right, "not_right");
            LLVMValueRef not_left_and_not_right = LLVMBuildAnd(builder, not_left, not_right, "not_left_and_not_right");
            
            return LLVMBuildOr(builder, left_and_right, not_left_and_not_right, "iff");
        }
            
        case NODE_ASSIGN:
            // Assignments are handled during pre-processing, just evaluate the right side
            if (node->right) {
                return gen_expression(context, builder, node->right, symbol_table, 
                                    true_str, false_str, printf_func, printf_type);
            } else if (node->left) {
                return gen_expression(context, builder, node->left, symbol_table, 
                                    true_str, false_str, printf_func, printf_type);
            }
            return NULL;
            
        default:
            fprintf(stderr, "Unsupported node type: %d\n", node->type);
            return NULL;
    }
}

// Generate LLVM IR for an AST with optimization level
LLVMCodegenResult generate_llvm_ir(MultiStatementAST* multi_ast, SymbolTable* symbol_table, 
                                 const char* output_filename, int optimization_level) {
    LLVMCodegenResult result = {LLVM_CODEGEN_OK, NULL, NULL};
    
    // Validate inputs
    if (!multi_ast || !symbol_table || !output_filename) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Invalid input parameters");
        return result;
    }
    
    // Initialize LLVM
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    
    // Create context, module, and builder
    LLVMContextRef context = LLVMContextCreate();
    char module_name[256];
    snprintf(module_name, sizeof(module_name), "%s_module", output_filename);
    LLVMModuleRef module = LLVMModuleCreateWithName(module_name);
    LLVMBuilderRef builder = LLVMCreateBuilder();
    
    // Store the module in the result structure
    result.module = module;
    
    // Create main function
    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", main_type);
    
    // Create entry block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    
    // Create printf function
    LLVMTypeRef printf_param_types[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), printf_param_types, 1, 1);
    LLVMValueRef printf_func = LLVMAddFunction(module, "printf", printf_type);
    
    // Create strings for output
    LLVMValueRef true_str = LLVMBuildGlobalStringPtr(builder, "TRUE", "true_str");
    LLVMValueRef false_str = LLVMBuildGlobalStringPtr(builder, "FALSE", "false_str");
    
    // Print header
    add_evaluation_message(builder, printf_func, printf_type, 
                         "Logical Expression Evaluation\n");
    add_evaluation_message(builder, printf_func, printf_type, 
                         "---------------------------\n\n");
    
    // Indicate start of evaluation
    add_evaluation_message(builder, printf_func, printf_type, 
                         "Starting evaluation of multiple expressions\n");
    
    // Process all statements in the multi-statement AST
    for (int i = 0; i < multi_ast->count; i++) {
        Node* node = multi_ast->statements[i];
        
        if (node->type == NODE_ASSIGN) {
            // Handle variable assignment
            char* var_name = node->left->name;
            LLVMValueRef value = gen_expression(context, builder, node->right, symbol_table, 
                                             true_str, false_str, printf_func, printf_type);
            
            if (!value) {
                fprintf(stderr, "Error generating code for assignment to %s\n", var_name);
                continue;
            }
            
            // Store the value in the symbol table
            int symbol_value = get_symbol_value(symbol_table, var_name);
            if (symbol_value != ERROR_SYMBOL_NOT_FOUND) {
                // Update the symbol value in the table
                add_or_update_symbol(symbol_table, var_name, value ? 1 : 0);
            }
            
            // Print the assignment result
            add_evaluation_message(builder, printf_func, printf_type, "Assigned %s = ", var_name);
            LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, "%s\n", "fmt_assign");
            LLVMValueRef val_str = value ? true_str : false_str;
            LLVMValueRef args[] = { fmt_str, val_str };
            LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
        } else if (node->type == NODE_VAR) {  // Assuming NODE_VAR is used for print statements
            // Handle print statement
            LLVMValueRef value = gen_expression(context, builder, node->left, symbol_table,
                                             true_str, false_str, printf_func, printf_type);
            
            if (!value) {
                fprintf(stderr, "Error generating code for print statement\n");
                continue;
            }
            
            // Print the value
            LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, "%s\n", "fmt_print");
            LLVMValueRef val_str = value ? true_str : false_str;
            LLVMValueRef args[] = { fmt_str, val_str };
            LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
        }
    }
    
    // Indicate completion
    add_evaluation_message(builder, printf_func, printf_type, 
                         "Completed evaluation of all expressions\n");
    
    // Return 0 from main
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
    
    // Verify the module
    char* error = NULL;
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error)) {
        fprintf(stderr, "Error verifying module: %s\n", error);
        LLVMDisposeMessage(error);
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Module verification failed");
        return result;
    }
    
    for (int i = 0; i < multi_ast->count; i++) {
        Node* node = multi_ast->statements[i];
        if (!node) continue;
        
        if (node->type == NODE_ASSIGN && node->left && node->left->type == NODE_VAR) {
            // For assignments, display the variable and its value
            char* var_name = node->left->name;
            int value = get_symbol_value(symbol_table, var_name);
            
            LLVMValueRef name_str = LLVMBuildGlobalStringPtr(builder, var_name, "var_name");
            LLVMValueRef value_str = (value == 1) ? true_str : false_str;
            
            // Show expression evaluation
            LLVMValueRef eval_fmt = LLVMBuildGlobalStringPtr(builder, "Evaluating expression: %s = %s\n", "eval_fmt");
            LLVMValueRef eval_args[] = { eval_fmt, name_str, value_str };
            LLVMBuildCall2(builder, printf_type, printf_func, eval_args, 3, "");
            
            // Show assignment
            LLVMValueRef assign_fmt = LLVMBuildGlobalStringPtr(builder, "Assigned %s = %s\n", "assign_fmt");
            LLVMValueRef assign_args[] = { assign_fmt, name_str, value_str };
            LLVMBuildCall2(builder, printf_type, printf_func, assign_args, 3, "");
            
            // Show result
            LLVMValueRef result_fmt = LLVMBuildGlobalStringPtr(builder, "Result: %s\n\n", "result_fmt");
            LLVMValueRef result_args[] = { result_fmt, value_str };
            LLVMBuildCall2(builder, printf_type, printf_func, result_args, 2, "");
        } else {
            // Count non-assignment expressions for later processing
            // Skip non-assignment nodes for now
        }
    }
    
    // Process any non-assignment expressions (logical operations)
    for (int i = 0; i < multi_ast->count; i++) {
        Node* node = multi_ast->statements[i];
        if (!node || node->type == NODE_ASSIGN) continue;
        
        // Show expression being evaluated
        char* expr_str = node_to_string(node);
        if (expr_str) {
            LLVMValueRef expr_eval_fmt = LLVMBuildGlobalStringPtr(builder, "Evaluating expression: %s\n", "expr_eval_fmt");
            LLVMValueRef expr_str_val = LLVMBuildGlobalStringPtr(builder, expr_str, "expr_str");
            LLVMValueRef expr_eval_args[] = { expr_eval_fmt, expr_str_val };
            LLVMBuildCall2(builder, printf_type, printf_func, expr_eval_args, 2, "");
            free(expr_str);
        }
        
        // Generate code with detailed evaluation
        LLVMValueRef expr_result = gen_expression(context, builder, node, symbol_table, 
                                               true_str, false_str, printf_func, printf_type);
        
        if (expr_result) {
            // Convert boolean result to string
            LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntNE, expr_result, 
                                           LLVMConstInt(LLVMInt1Type(), 0, 0), "cond");
            LLVMValueRef result_str = LLVMBuildSelect(builder, cond, true_str, false_str, "result_str");
            
            // Print result
            LLVMValueRef result_fmt = LLVMBuildGlobalStringPtr(builder, "Result: %s\n\n", "result_fmt");
            LLVMValueRef result_args[] = { result_fmt, result_str };
            LLVMBuildCall2(builder, printf_type, printf_func, result_args, 2, "");
        }
    }
    
    // Indicate completion
    add_evaluation_message(builder, printf_func, printf_type, 
                         "Completed evaluation of all expressions\n");
    
    // Return 0
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
    
    // Create filenames for bitcode and IR
    char bitcode_filename[512];
    char ir_filename[512];
    
    snprintf(bitcode_filename, sizeof(bitcode_filename), "%s.bc", output_filename);
    snprintf(ir_filename, sizeof(ir_filename), "%s.ll", output_filename);
    
    // Save LLVM IR to file first
    LLVMCodegenResult ir_result = save_llvm_ir(module, ir_filename);
    if (ir_result.error_code != LLVM_CODEGEN_OK) {
        // If we fail to save IR, still continue with compilation
        printf("Warning: Failed to save LLVM IR: %s\n", ir_result.error_message);
        free_llvm_codegen_result(&ir_result);
    } else {
        printf("LLVM IR saved to %s\n", ir_filename);
        free_llvm_codegen_result(&ir_result);
    }
    
    // Write bitcode to file
    if (LLVMWriteBitcodeToFile(module, bitcode_filename) != 0) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to write bitcode to file");
        
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        LLVMContextDispose(context);
        
        return result;
    }
    
    result.output_file = strdup(bitcode_filename);
    
    // Run optimizations at the specified level
    if (optimization_level > 0) {
        printf("Running optimizations at level -O%d...\n", optimization_level);
        optimize_module(module, optimization_level);
    }
    
    // Clean up (don't dispose of the module as it's being returned)
    LLVMDisposeBuilder(builder);
    
    // Note: We don't dispose of the context here as it's needed for the module
    // The caller is responsible for cleaning up the module and context
    
    return result;
}

// Save LLVM IR to a human-readable .ll file
LLVMCodegenResult save_llvm_ir(LLVMModuleRef module, const char* filename) {
    LLVMCodegenResult result = {LLVM_CODEGEN_OK, NULL, NULL};
    char* error_msg = NULL;
    
    if (!module || !filename) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Invalid parameters for saving LLVM IR");
        return result;
    }
    
    // First, verify the module
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error_msg)) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup(error_msg);
        LLVMDisposeMessage(error_msg);
        return result;
    }
    
    // Print the module to the file
    if (LLVMPrintModuleToFile(module, filename, &error_msg)) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup(error_msg ? error_msg : "Unknown error printing module");
        if (error_msg) LLVMDisposeMessage(error_msg);
        return result;
    }
    
    result.output_file = strdup(filename);
    return result;
}

// Compile and link the IR
LLVMCodegenResult compile_and_link_ir(const char* ir_filename, const char* output_filename) {
    LLVMCodegenResult result = {LLVM_CODEGEN_OK, NULL, NULL};
    
    // Validate parameters
    if (!ir_filename || !output_filename) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Invalid filename parameters");
        return result;
    }
    
    // Check file extension to determine if it's LLVM IR (.ll) or bitcode (.bc)
    const char* ext = strrchr(ir_filename, '.');
    int is_ir_file = (ext && strcmp(ext, ".ll") == 0);
    
    // Create appropriate clang command
    char command[1024];
    if (is_ir_file) {
        // For LLVM IR (.ll) files, we need to use -x ir
        snprintf(command, sizeof(command), 
                "clang -x ir %s -o %s", ir_filename, output_filename);
    } else {
        // For bitcode (.bc) files, we can use it directly
        snprintf(command, sizeof(command), 
                "clang %s -o %s", ir_filename, output_filename);
    }
    
    printf("Executing: %s\n", command);
    
    // Execute command
    int status = system(command);
    if (status != 0) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to compile and link bitcode");
        return result;
    }
    
    printf("Successfully compiled to: %s\n", output_filename);
    result.output_file = strdup(output_filename);
    return result;
}

// Apply optimizations to the module using LLVM's C API
static void optimize_module(LLVMModuleRef module, int opt_level) {
    if (!module || opt_level < 0 || opt_level > 3) return;
    
    printf("Running optimizations at level -O%d...\n", opt_level);
    
    // For now, we'll just verify the module
    // In a production environment, you would add optimization passes here
    char *error = NULL;
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error)) {
        fprintf(stderr, "Error verifying module: %s\n", error);
        LLVMDisposeMessage(error);
    }
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
    
    // Note: We don't dispose of the module here as it's managed by the caller
    // The caller should call LLVMDisposeModule when done with the module
    result->error_code = LLVM_CODEGEN_OK;
}
