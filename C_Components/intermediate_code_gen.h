#ifndef INTERMEDIATE_CODE_GEN_H
#define INTERMEDIATE_CODE_GEN_H

#include "ast.h"

// Three-address code representation
typedef enum {
    TAC_ASSIGN,
    TAC_NOT,
    TAC_AND,
    TAC_OR,
    TAC_XOR,
    TAC_XNOR,
    TAC_IMPLIES,
    TAC_IFF,
    TAC_EQUIV
} TACOpType;

typedef struct TACInstruction {
    TACOpType op;
    char* result;
    char* arg1;
    char* arg2;
} TACInstruction;

typedef struct {
    TACInstruction** instructions;
    int count;
    int capacity;
} IntermediateCode;

// Function prototypes
IntermediateCode* generate_three_address_code(Node* ast);
void free_intermediate_code(IntermediateCode* code);
void print_intermediate_code(IntermediateCode* code);

#endif // INTERMEDIATE_CODE_GEN_H
