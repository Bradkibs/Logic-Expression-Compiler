#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"

// Semantic analysis error codes
typedef enum {
    SEMANTIC_OK,
    SEMANTIC_UNDEFINED_VARIABLE,
    SEMANTIC_TYPE_MISMATCH,
    SEMANTIC_INVALID_QUANTIFIER,
    SEMANTIC_AMBIGUOUS_EXPRESSION
} SemanticErrorCode;

// Semantic analysis result structure
typedef struct {
    SemanticErrorCode error_code;
    char* error_message;
} SemanticAnalysisResult;

// Function prototypes for semantic analysis
SemanticAnalysisResult perform_semantic_analysis(Node* ast, SymbolTable* symbol_table);
bool validate_variable_usage(Node* node, SymbolTable* symbol_table);
bool validate_quantifier_expression(Node* node, SymbolTable* symbol_table);
bool check_ambiguous_expression(Node* node, bool* ambiguous);
char* generate_parenthesized_expression(Node* node);

#endif // SEMANTIC_ANALYZER_H
