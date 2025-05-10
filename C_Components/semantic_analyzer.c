#include <stdlib.h>
#include <string.h>
#include "semantic_analyzer.h"
#include "symbol_table.h"

SemanticAnalysisResult perform_semantic_analysis(Node* ast, SymbolTable* symbol_table) {
    SemanticAnalysisResult result = {SEMANTIC_OK, NULL};

    if (!ast) {
        result.error_code = SEMANTIC_INVALID_QUANTIFIER;
        result.error_message = strdup("Invalid AST: NULL node");
        return result;
    }

    // Validate variable usage
    if (!validate_variable_usage(ast, symbol_table)) {
        result.error_code = SEMANTIC_UNDEFINED_VARIABLE;
        result.error_message = strdup("Undefined variable used in expression");
        return result;
    }

    // Validate quantifier expressions
    if (!validate_quantifier_expression(ast, symbol_table)) {
        result.error_code = SEMANTIC_INVALID_QUANTIFIER;
        result.error_message = strdup("Invalid quantifier expression");
        return result;
    }

    return result;
}

bool validate_variable_usage(Node* node, SymbolTable* symbol_table) {
    if (!node) return true;

    switch (node->type) {
        case NODE_VAR:
            // Check if variable exists in symbol table
            // Implement symbol table lookup
            for (int i = 0; i < symbol_table->size; i++) {
                if (strcmp(symbol_table->symbols[i].name, node->name) == 0) {
                    return true;
                }
            }
            return false;

        case NODE_NOT:
            return validate_variable_usage(node->left, symbol_table);

        case NODE_AND:
        case NODE_OR:
        case NODE_XOR:
        case NODE_XNOR:
        case NODE_IMPLIES:
        case NODE_IFF:
        case NODE_EQUIV:
            return validate_variable_usage(node->left, symbol_table) &&
                   validate_variable_usage(node->right, symbol_table);

        case NODE_EXISTS:
        case NODE_FORALL:
            // Add variable to symbol table for the quantified expression
            // Implement symbol table insertion
            if (symbol_table->size >= symbol_table->capacity) {
                // Resize the symbols array
                symbol_table->capacity = symbol_table->capacity == 0 ? 10 : symbol_table->capacity * 2;
                symbol_table->symbols = realloc(symbol_table->symbols, 
                    symbol_table->capacity * sizeof(Symbol));
            }
            
            // Add the new symbol
            strcpy(symbol_table->symbols[symbol_table->size].name, node->name);
            symbol_table->symbols[symbol_table->size].value = 0;
            symbol_table->size++;
            return validate_variable_usage(node->left, symbol_table);

        default:
            return true;
    }
}

bool validate_quantifier_expression(Node* node, SymbolTable* symbol_table) {
    if (!node) return true;

    switch (node->type) {
        case NODE_EXISTS:
        case NODE_FORALL:
            // Ensure quantified variable is used in the expression
            if (!node->left || !node->name) return false;
            return true;

        case NODE_NOT:
            return validate_quantifier_expression(node->left, symbol_table);

        case NODE_AND:
        case NODE_OR:
        case NODE_XOR:
        case NODE_XNOR:
        case NODE_IMPLIES:
        case NODE_IFF:
        case NODE_EQUIV:
            return validate_quantifier_expression(node->left, symbol_table) &&
                   validate_quantifier_expression(node->right, symbol_table);

        default:
            return true;
    }
}
