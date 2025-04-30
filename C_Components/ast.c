#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

// External declarations from lexer/parser
extern int yyparse(void);
extern Node *parsed_expression;
extern void yy_scan_string(const char *);
extern void yy_delete_buffer(void *);
extern void *yy_create_buffer(FILE *, int);
extern void yy_switch_to_buffer(void *);
extern int yydebug;

// Parser error handling
void yyerror(const char *s)
{
    fprintf(stderr, "Parser error: %s\n", s);
}

// General node creation function
Node *create_node(NodeType type, char *name, Node *left, Node *right, int bool_val)
{
    Node *node = malloc(sizeof(Node));
    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->left = left;
    node->right = right;
    node->bool_val = bool_val;
    return node;
}

// Node creation wrappers
Node *create_variable_node(char *name) { return create_node(NODE_VAR, name, NULL, NULL, 0); }
Node *create_assignment_node(char *name, Node *expr) { return create_node(NODE_ASSIGN, name, expr, NULL, 0); }
Node *create_not_node(Node *expr) { return create_node(NODE_NOT, NULL, expr, NULL, 0); }
Node *create_and_node(Node *l, Node *r) { return create_node(NODE_AND, NULL, l, r, 0); }
Node *create_or_node(Node *l, Node *r) { return create_node(NODE_OR, NULL, l, r, 0); }
Node *create_xor_node(Node *l, Node *r) { return create_node(NODE_XOR, NULL, l, r, 0); }
Node *create_xnor_node(Node *l, Node *r) { return create_node(NODE_XNOR, NULL, l, r, 0); }
Node *create_implies_node(Node *l, Node *r) { return create_node(NODE_IMPLIES, NULL, l, r, 0); }
Node *create_iff_node(Node *l, Node *r) { return create_node(NODE_IFF, NULL, l, r, 0); }
Node *create_equiv_node(Node *l, Node *r) { return create_node(NODE_EQUIV, NULL, l, r, 0); }
Node *create_exists_node(char *var, Node *expr) { return create_node(NODE_EXISTS, var, expr, NULL, 0); }
Node *create_forall_node(char *var, Node *expr) { return create_node(NODE_FORALL, var, expr, NULL, 0); }
Node *create_boolean_node(int value) { return create_node(NODE_BOOL, NULL, NULL, NULL, value); }

// AST printing
void print_ast(Node *node, int indent)
{
    if (!node)
        return;
    for (int i = 0; i < indent; i++)
        printf("  ");
    printf("NodeType: %d", node->type);
    if (node->name)
        printf(", Name: %s", node->name);
    if (node->type == NODE_BOOL)
        printf(", Value: %s", node->bool_val ? "true" : "false");
    printf("\n");
    print_ast(node->left, indent + 1);
    print_ast(node->right, indent + 1);
}

// Free memory of the AST
void free_ast(Node *node)
{
    if (!node)
        return;
    free(node->name);
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

// Deep copy of AST node
Node *clone_node(Node *node)
{
    if (!node)
        return NULL;

    Node *left_copy = clone_node(node->left);
    Node *right_copy = clone_node(node->right);
    return create_node(node->type, node->name, left_copy, right_copy, node->bool_val);
}

// Logical Laws
Node *apply_de_morgan(Node *node, EvaluationSteps *steps)
{
    if (!node || node->type != NODE_NOT)
        return node;

    if (node->left->type == NODE_AND || node->left->type == NODE_OR)
    {
        NodeType new_type = node->left->type == NODE_AND ? NODE_OR : NODE_AND;
        Node *new_left = create_not_node(clone_node(node->left->left));
        Node *new_right = create_not_node(clone_node(node->left->right));
        Node *transformed = create_node(new_type, NULL, new_left, new_right, 0);
        add_evaluation_step(steps, "Applied De Morgan's Law");
        return transformed;
    }
    return node;
}

Node *apply_commutative(Node *node, EvaluationSteps *steps)
{
    if (!node)
        return node;
    if (node->type == NODE_AND || node->type == NODE_OR)
    {
        Node *temp = node->left;
        node->left = node->right;
        node->right = temp;
        add_evaluation_step(steps, "Applied Commutative Law");
    }
    return node;
}

Node *apply_distributive(Node *node, EvaluationSteps *steps)
{
    if (!node || !node->left || !node->right)
        return node;
    if (node->type == NODE_AND && node->right->type == NODE_OR)
    {
        Node *a = node->left;
        Node *b = node->right->left;
        Node *c = node->right->right;
        Node *left = create_and_node(clone_node(a), clone_node(b));
        Node *right = create_and_node(clone_node(a), clone_node(c));
        Node *transformed = create_or_node(left, right);
        add_evaluation_step(steps, "Applied Distributive Law: AND over OR");
        return transformed;
    }
    return node;
}

Node *apply_implication(Node *node, EvaluationSteps *steps)
{
    if (!node || node->type != NODE_IMPLIES)
        return node;
    Node *not_left = create_not_node(clone_node(node->left));
    Node *transformed = create_or_node(not_left, clone_node(node->right));
    add_evaluation_step(steps, "Applied Implication Law: A -> B == ~A OR B");
    return transformed;
}

Node *apply_iff(Node *node, EvaluationSteps *steps)
{
    if (!node || node->type != NODE_IFF)
        return node;
    Node *left_impl = create_implies_node(clone_node(node->left), clone_node(node->right));
    Node *right_impl = create_implies_node(clone_node(node->right), clone_node(node->left));
    Node *transformed = create_and_node(left_impl, right_impl);
    add_evaluation_step(steps, "Applied IFF Law: A <-> B == (A -> B) AND (B -> A)");
    return transformed;
}

// Recursive logical law application
Node *apply_logical_laws(Node *node, EvaluationSteps *steps)
{
    if (!node)
        return NULL;

    // Recurse first (post-order traversal)
    node->left = apply_logical_laws(node->left, steps);
    node->right = apply_logical_laws(node->right, steps);

    switch (node->type)
    {
    case NODE_NOT:
        return apply_de_morgan(node, steps);
    case NODE_IMPLIES:
        return apply_implication(node, steps);
    case NODE_IFF:
        return apply_iff(node, steps);
    case NODE_AND:
    case NODE_OR:
        node = apply_commutative(node, steps);
        node = apply_distributive(node, steps);
        break;
    default:
        break;
    }

    return node;
}

// Evaluation step helpers
EvaluationSteps *init_evaluation_steps()
{
    EvaluationSteps *steps = malloc(sizeof(EvaluationSteps));
    steps->steps = malloc(sizeof(EvaluationStep *) * 10);
    steps->step_count = 0;
    steps->capacity = 10;
    return steps;
}

void add_evaluation_step(EvaluationSteps *steps, const char *description)
{
    if (steps->step_count >= steps->capacity)
    {
        steps->capacity *= 2;
        steps->steps = realloc(steps->steps, sizeof(EvaluationStep *) * steps->capacity);
    }
    EvaluationStep *step = malloc(sizeof(EvaluationStep));
    step->step_description = strdup(description);
    steps->steps[steps->step_count++] = step;
}

void free_evaluation_steps(EvaluationSteps *steps)
{
    for (int i = 0; i < steps->step_count; i++)
    {
        free(steps->steps[i]->step_description);
        free(steps->steps[i]);
    }
    free(steps->steps);
    free(steps);
}

// Node type to string
static const char *get_node_type_str(NodeType type)
{
    switch (type)
    {
    case NODE_VAR:
        return "Variable";
    case NODE_ASSIGN:
        return "Assignment";
    case NODE_NOT:
        return "NOT";
    case NODE_AND:
        return "AND";
    case NODE_OR:
        return "OR";
    case NODE_XOR:
        return "XOR";
    case NODE_XNOR:
        return "XNOR";
    case NODE_IMPLIES:
        return "IMPLIES";
    case NODE_IFF:
        return "IFF";
    case NODE_EQUIV:
        return "EQUIV";
    case NODE_EXISTS:
        return "EXISTS";
    case NODE_FORALL:
        return "FORALL";
    case NODE_BOOL:
        return "Boolean";
    default:
        return "Unknown";
    }
}

// Evaluation recording
static void evaluate_node_and_record(Node *node, EvaluationSteps *steps)
{
    if (!node || !steps)
        return;

    char description[256];
    if (node->type == NODE_BOOL)
        snprintf(description, sizeof(description), "Boolean literal: %s", node->bool_val ? "true" : "false");
    else if (node->name)
        snprintf(description, sizeof(description), "Processing %s: %s", get_node_type_str(node->type), node->name);
    else
        snprintf(description, sizeof(description), "Processing operator: %s", get_node_type_str(node->type));

    add_evaluation_step(steps, description);
    evaluate_node_and_record(node->left, steps);
    evaluate_node_and_record(node->right, steps);
}

// Expression entry point
EvaluationSteps *evaluate_expression(const char *expression)
{
    if (!expression)
        return NULL;

    EvaluationSteps *steps = init_evaluation_steps();
    char description[256];
    snprintf(description, sizeof(description), "Evaluating expression: %s", expression);
    add_evaluation_step(steps, description);

    add_evaluation_step(steps, "Parsing expression...");
    Node *ast = parse_string(expression);
    if (!ast)
    {
        add_evaluation_step(steps, "Failed to parse expression");
        return steps;
    }

    add_evaluation_step(steps, "Successfully parsed expression into AST");

    ast = apply_logical_laws(ast, steps);
    evaluate_node_and_record(ast, steps);
    add_evaluation_step(steps, "Evaluation complete");

    free_ast(ast);
    return steps;
}

// Parse string wrapper
static Node *parse_string(const char *input)
{
    yydebug = 1;
    yy_scan_string(input);
    parsed_expression = NULL;
    int parse_result = yyparse();
    return (parse_result == 0) ? parsed_expression : NULL;
}
