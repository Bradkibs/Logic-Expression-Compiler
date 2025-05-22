#include "llvm_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// LLVM C API headers
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/BitWriter.h>

// Forward declarations
static LLVMValueRef gen_expression(LLVMContextRef context, LLVMBuilderRef builder, 
                                  Node* node, SymbolTable* symbol_table, 
                                  LLVMValueRef true_str, LLVMValueRef false_str,
                                  LLVMValueRef printf_func, LLVMTypeRef printf_type);

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

// Generate LLVM IR for an AST
LLVMCodegenResult generate_llvm_ir(MultiStatementAST* multi_ast, SymbolTable* symbol_table, const char* output_filename) {
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
    LLVMModuleRef module = LLVMModuleCreateWithName("logic_module");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    
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
    
    // Process all variable assignments and print them
    int non_assignment_count = 0;
    
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
            non_assignment_count++;
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
    
    // Create bitcode filename
    char ir_filename[256];
    snprintf(ir_filename, sizeof(ir_filename), "%s.bc", output_filename);
    
    // Write module to file
    if (LLVMWriteBitcodeToFile(module, ir_filename) != 0) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to write bitcode to file");
        
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        LLVMContextDispose(context);
        
        return result;
    }
    
    // Clean up
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);
    
    // Return success
    result.output_file = strdup(ir_filename);
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
    
    // Create clang command
    char command[512];
    snprintf(command, sizeof(command), "clang -o %s %s", output_filename, ir_filename);
    
    // Execute command
    int status = system(command);
    if (status != 0) {
        result.error_code = LLVM_CODEGEN_ERROR;
        result.error_message = strdup("Failed to compile and link bitcode");
        return result;
    }
    
    result.output_file = strdup(output_filename);
    return result;
}

// Free result
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
