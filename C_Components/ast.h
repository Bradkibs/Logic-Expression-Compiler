#ifndef AST_H
#define AST_H

#ifdef __cplusplus
extern "C"
{
#endif
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

    typedef struct Node
    {
        NodeType type;
        char *name; // For variable or quantifier var
        struct Node *left;
        struct Node *right;
        int bool_val;
    } Node;

// Global variable for parsed expression result
#ifdef __cplusplus
    extern "C"
    {
#endif
        extern Node *parsed_expression;
#ifdef __cplusplus
    }
#endif
    // Constructors
    Node *create_variable_node(char *name);
    Node *create_assignment_node(char *name, Node *expr);
    Node *create_not_node(Node *expr);
    Node *create_and_node(Node *left, Node *right);
    Node *create_or_node(Node *left, Node *right);
    Node *create_xor_node(Node *left, Node *right);
    Node *create_xnor_node(Node *left, Node *right);
    Node *create_implies_node(Node *left, Node *right);
    Node *create_iff_node(Node *left, Node *right);
    Node *create_equiv_node(Node *left, Node *right);
    Node *create_exists_node(char *var, Node *expr);
    Node *create_forall_node(char *var, Node *expr);

    void print_ast(Node *root, int indent);
    void free_ast(Node *root);

    // Evaluation step structure for tracking logical evaluation
    typedef struct
    {
        char *step_description;
    } EvaluationStep;

    // Structure to hold multiple evaluation steps
    typedef struct
    {
        EvaluationStep **steps;
        int step_count;
        int capacity;
    } EvaluationSteps;

    // Initialize evaluation steps structure
    extern EvaluationSteps *init_evaluation_steps();

    // Add a step to the evaluation steps
    extern void add_evaluation_step(EvaluationSteps *steps, const char *description);

    // Free evaluation steps memory
    extern void free_evaluation_steps(EvaluationSteps *steps);

    // Evaluate a logical expression and return the evaluation steps
    extern EvaluationSteps *evaluate_expression(const char *expression);

#ifdef __cplusplus
}
#endif

#endif
