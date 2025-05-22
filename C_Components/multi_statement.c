#include "multi_statement.h"
#include <stdlib.h>
#include <stdio.h>

// Initialize a multi-statement AST
MultiStatementAST* init_multi_statement_ast() {
    MultiStatementAST* ast = (MultiStatementAST*)malloc(sizeof(MultiStatementAST));
    if (!ast) {
        fprintf(stderr, "Error: Memory allocation failed for MultiStatementAST\n");
        return NULL;
    }
    
    ast->capacity = 10;  // Initial capacity
    ast->count = 0;
    ast->statements = (Node**)malloc(sizeof(Node*) * ast->capacity);
    
    if (!ast->statements) {
        fprintf(stderr, "Error: Memory allocation failed for statements array\n");
        free(ast);
        return NULL;
    }
    
    return ast;
}

// Add a statement to the multi-statement AST
void add_statement(MultiStatementAST* ast, Node* statement) {
    if (!ast || !statement) return;
    
    // Resize if needed
    if (ast->count >= ast->capacity) {
        int new_capacity = ast->capacity * 2;
        Node** new_statements = (Node**)realloc(ast->statements, sizeof(Node*) * new_capacity);
        
        if (!new_statements) {
            fprintf(stderr, "Error: Failed to resize statements array\n");
            return;
        }
        
        ast->statements = new_statements;
        ast->capacity = new_capacity;
    }
    
    // Add the statement
    ast->statements[ast->count++] = statement;
}

// Free the multi-statement AST
void free_multi_statement_ast(MultiStatementAST* ast) {
    if (!ast) return;
    
    // Free each statement
    for (int i = 0; i < ast->count; i++) {
        free_ast(ast->statements[i]);
    }
    
    // Free the statements array and the AST structure
    free(ast->statements);
    free(ast);
}
