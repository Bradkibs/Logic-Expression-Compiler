#ifndef MULTI_STATEMENT_H
#define MULTI_STATEMENT_H

#include "ast.h"

// Structure for multiple AST statements
typedef struct {
    Node** statements;
    int count;
    int capacity;
} MultiStatementAST;

// Function declarations for MultiStatementAST operations
MultiStatementAST* init_multi_statement_ast();
void add_statement(MultiStatementAST* ast, Node* statement);
void free_multi_statement_ast(MultiStatementAST* ast);

#endif /* MULTI_STATEMENT_H */
