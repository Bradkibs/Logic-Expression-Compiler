#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "symbol_table.h"

// Node types for AST
typedef enum
{
    NODE_VAR,
    NODE_ASSIGN,
    NODE_NOT,
    NODE_AND,
    NODE_OR,
    NODE_XOR,
    NODE_XNOR,
    NODE_IMPLIES,
    NODE_IFF,
    NODE_EQUIV,
    NODE_EXISTS,
    NODE_FORALL,
    NODE_BOOL
} NodeType;

// Add more specific error codes as needed
// Structure for representing AST nodes
typedef struct Node
{
    NodeType type;
    char *name;
    struct Node *left;
    struct Node *right;
    int bool_val; // Used for boolean literals and evaluated results
    int is_parenthesized; // Flag to track if the expression is parenthesized
} Node;

// Structure for recording evaluation steps
typedef struct
{
    char *step_description;
} EvaluationStep;

typedef struct
{
    EvaluationStep **steps;
    int step_count;
    int capacity;
} EvaluationSteps;

// Function declarations
Node *create_node(NodeType type, char *name, Node *left, Node *right, int bool_val);
Node *create_variable_node(char *name);
const char *get_node_type_str(NodeType type);
Node *create_assignment_node(char *name, Node *expr);
Node *create_not_node(Node *expr);
Node *create_and_node(Node *l, Node *r);
Node *create_or_node(Node *l, Node *r);
Node *create_xor_node(Node *l, Node *r);
Node *create_xnor_node(Node *l, Node *r);
Node *create_implies_node(Node *l, Node *r);
Node *create_iff_node(Node *l, Node *r);
Node *create_equiv_node(Node *l, Node *r);
Node *create_exists_node(char *var, Node *expr);
Node *create_forall_node(char *var, Node *expr);
Node *create_boolean_node(int value);

void print_ast(Node *node, int indent);
void free_ast(Node *node);
Node *clone_node(const Node *node);
char* node_to_string(Node* node);

// Evaluation steps
EvaluationSteps *init_evaluation_steps();
void add_evaluation_step(EvaluationSteps *steps, const char *description);
void free_evaluation_steps(EvaluationSteps *steps);

// Logical operations
Node *apply_de_morgan(Node *node, EvaluationSteps *steps);
Node *apply_commutative(Node *node, EvaluationSteps *steps);
Node *apply_distributive(Node *node, EvaluationSteps *steps);
Node *apply_implication(Node *node, EvaluationSteps *steps);
Node *apply_iff(Node *node, EvaluationSteps *steps);
Node *apply_logical_laws(Node *node, EvaluationSteps *steps);

// Main evaluation function
EvaluationSteps *evaluate_expression(const char *expression);

// New functions for symbol table integration
Node *evaluate_node_with_symbol_table(Node *node, SymbolTable *symbol_table, EvaluationSteps *steps);
EvaluationSteps *evaluate_multiple_expressions(const char *expressions);

// FFI functions
int get_steps_count(EvaluationSteps *steps);
char *get_step_at(EvaluationSteps *steps, int index);

#endif /* AST_H */