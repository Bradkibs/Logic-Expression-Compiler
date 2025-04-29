#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

// External declarations from lexer/parser
extern int yyparse(void);
extern Node* parsed_expression;  // Global variable to store the parsed result
extern void yy_scan_string(const char*);
extern void yy_delete_buffer(void*);
extern void* yy_create_buffer(FILE*, int);
extern void yy_switch_to_buffer(void*);
extern int yydebug;  // External declaration of parser debug flag
// Error handling function for the parser
void yyerror(const char* s) {
    fprintf(stderr, "Parser error: %s\n", s);
}
Node* create_node(NodeType type, char* name, Node* left, Node* right) {
    Node* node = malloc(sizeof(Node));
    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->left = left;
    node->right = right;
    return node;
}

Node* create_variable_node(char* name) { return create_node(NODE_VAR, name, NULL, NULL); }
Node* create_assignment_node(char* name, Node* expr) { return create_node(NODE_ASSIGN, name, expr, NULL); }
Node* create_not_node(Node* expr) { return create_node(NODE_NOT, NULL, expr, NULL); }
Node* create_and_node(Node* l, Node* r) { return create_node(NODE_AND, NULL, l, r); }
Node* create_or_node(Node* l, Node* r) { return create_node(NODE_OR, NULL, l, r); }
Node* create_xor_node(Node* l, Node* r) { return create_node(NODE_XOR, NULL, l, r); }
Node* create_xnor_node(Node* l, Node* r) { return create_node(NODE_XNOR, NULL, l, r); }
Node* create_implies_node(Node* l, Node* r) { return create_node(NODE_IMPLIES, NULL, l, r); }
Node* create_iff_node(Node* l, Node* r) { return create_node(NODE_IFF, NULL, l, r); }
Node* create_equiv_node(Node* l, Node* r) { return create_node(NODE_EQUIV, NULL, l, r); }
Node* create_exists_node(char* var, Node* expr) { return create_node(NODE_EXISTS, var, expr, NULL); }
Node* create_forall_node(char* var, Node* expr) { return create_node(NODE_FORALL, var, expr, NULL); }

void print_ast(Node* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("NodeType: %d", node->type);
    if (node->name) printf(", Name: %s", node->name);
    printf("\n");
    print_ast(node->left, indent + 1);
    print_ast(node->right, indent + 1);
}

void free_ast(Node* node) {
    if (!node) return;
    free(node->name);
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

/* Implementation of evaluation functions */

EvaluationSteps* init_evaluation_steps() {
    EvaluationSteps* steps = malloc(sizeof(EvaluationSteps));
    if (!steps) return NULL;
    
    steps->steps = malloc(sizeof(EvaluationStep*) * 10); // Initial capacity of 10
    if (!steps->steps) {
        free(steps);
        return NULL;
    }
    
    steps->step_count = 0;
    steps->capacity = 10;
    
    return steps;
}

void add_evaluation_step(EvaluationSteps* steps, const char* description) {
    if (!steps || !description) return;
    
    // Resize array if needed
    if (steps->step_count >= steps->capacity) {
        int new_capacity = steps->capacity * 2;
        EvaluationStep** new_steps = realloc(steps->steps, 
                                    sizeof(EvaluationStep*) * new_capacity);
        if (!new_steps) return;
        
        steps->steps = new_steps;
        steps->capacity = new_capacity;
    }
    
    // Create and add the new step
    EvaluationStep* step = malloc(sizeof(EvaluationStep));
    if (!step) return;
    
    step->step_description = strdup(description);
    if (!step->step_description) {
        free(step);
        return;
    }
    
    steps->steps[steps->step_count++] = step;
}

void free_evaluation_steps(EvaluationSteps* steps) {
    if (!steps) return;
    
    for (int i = 0; i < steps->step_count; i++) {
        free(steps->steps[i]->step_description);
        free(steps->steps[i]);
    }
    
    free(steps->steps);
    free(steps);
}

// Helper function to parse a string expression
static Node* parse_string(const char* input) {
    yydebug = 1;  // Enable debug output
    void* buffer = yy_create_buffer(NULL, strlen(input));
    yy_scan_string(input);
    yy_switch_to_buffer(buffer);
    
    parsed_expression = NULL;  // Reset global result
    int parse_result = yyparse();
    
    yy_delete_buffer(buffer);
    
    if (parse_result != 0 || parsed_expression == NULL) {
        return NULL;
    }
    
    return parsed_expression;
}

// Helper function for node type to string conversion
static const char* get_node_type_str(NodeType type) {
    switch (type) {
        case NODE_VAR: return "Variable";
        case NODE_ASSIGN: return "Assignment";
        case NODE_NOT: return "NOT";
        case NODE_AND: return "AND";
        case NODE_OR: return "OR";
        case NODE_XOR: return "XOR";
        case NODE_XNOR: return "XNOR";
        case NODE_IMPLIES: return "IMPLIES";
        case NODE_IFF: return "IFF";
        case NODE_EQUIV: return "EQUIV";
        case NODE_EXISTS: return "EXISTS";
        case NODE_FORALL: return "FORALL";
        default: return "Unknown";
    }
}

// Helper function to evaluate a node and record steps
static void evaluate_node_and_record(Node* node, EvaluationSteps* steps) {
    if (!node || !steps) return;
    
    char description[256];
    
    // Record the current node evaluation
    if (node->name) {
        snprintf(description, sizeof(description), "Processing %s: %s", 
                get_node_type_str(node->type), node->name);
    } else {
        snprintf(description, sizeof(description), "Processing operator: %s", 
                get_node_type_str(node->type));
    }
    add_evaluation_step(steps, description);
    
    // Recursively evaluate children
    if (node->left) evaluate_node_and_record(node->left, steps);
    if (node->right) evaluate_node_and_record(node->right, steps);
}

EvaluationSteps* evaluate_expression(const char* expression) {
    if (!expression) return NULL;
    
    // Initialize evaluation steps
    EvaluationSteps* steps = init_evaluation_steps();
    if (!steps) return NULL;
    
    // Record the initial expression
    char description[256];
    snprintf(description, sizeof(description), "Evaluating expression: %s", expression);
    add_evaluation_step(steps, description);
    
    // Try to parse the expression
    add_evaluation_step(steps, "Parsing expression...");
    
    // Parse the expression into an AST
    Node* ast = parse_string(expression);
    
    if (!ast) {
        add_evaluation_step(steps, "Failed to parse expression");
        return steps;
    }
    
    add_evaluation_step(steps, "Successfully parsed expression into AST");
    
    // Evaluate the AST
    evaluate_node_and_record(ast, steps);
    
    // Add completion step
    add_evaluation_step(steps, "Evaluation complete");
    
    // Clean up
    free_ast(ast);
    
    return steps;
}
