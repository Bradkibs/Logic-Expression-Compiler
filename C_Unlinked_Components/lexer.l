%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

extern YYSTYPE yylval;

// Store tokens for printing
struct TokenRecord {
    int token_type;
    char* token_text;
    struct TokenRecord* next;
};

struct TokenRecord* token_list = NULL;

// Add a token to the list
void record_token(int token_type, const char* token_text) {
    struct TokenRecord* new_token = (struct TokenRecord*)malloc(sizeof(struct TokenRecord));
    if (!new_token) return;
    
    new_token->token_type = token_type;
    new_token->token_text = strdup(token_text);
    new_token->next = token_list;
    token_list = new_token;
}

// Print and free all tokens in the list
void print_tokens() {
    printf("Tokens found during lexical analysis:\n");
    printf("===================================\n");
    
    // Count tokens first (for reverse printing)
    int count = 0;
    struct TokenRecord* current = token_list;
    while (current) {
        count++;
        current = current->next;
    }
    
    // Create an array for reverse access
    struct TokenRecord** token_array = (struct TokenRecord**)malloc(count * sizeof(struct TokenRecord*));
    if (!token_array) {
        printf("Error: Memory allocation failed\n");
        return;
    }
    
    // Fill the array
    current = token_list;
    for (int i = count - 1; i >= 0 && current; i--) {
        token_array[i] = current;
        current = current->next;
    }
    
    // Print tokens in original order
    for (int i = 0; i < count; i++) {
        const char* token_name = "UNKNOWN";
        
        // Map token types to string names
        switch (token_array[i]->token_type) {
            case AND: token_name = "AND"; break;
            case OR: token_name = "OR"; break;
            case NOT: token_name = "NOT"; break;
            case XOR: token_name = "XOR"; break;
            case XNOR: token_name = "XNOR"; break;
            case IMPLIES: token_name = "IMPLIES"; break;
            case IFF: token_name = "IFF"; break;
            case ASSIGN: token_name = "ASSIGN"; break;
            case EQUIV: token_name = "EQUIV"; break;
            case EXISTS: token_name = "EXISTS"; break;
            case FORALL: token_name = "FORALL"; break;
            case IF: token_name = "IF"; break;
            case IFF_KEYWORD: token_name = "IFF_KEYWORD"; break;
            case LPAREN: token_name = "LPAREN"; break;
            case RPAREN: token_name = "RPAREN"; break;
            case T_TRUE: token_name = "T_TRUE"; break;
            case T_FALSE: token_name = "T_FALSE"; break;
            case IDENTIFIER: token_name = "IDENTIFIER"; break;
            case INVALID_TOKEN: token_name = "INVALID_TOKEN"; break;
        }
        
        printf("  %-15s : '%s'\n", token_name, token_array[i]->token_text);
    }
    
    printf("===================================\n\n");
    
    // Free the token array
    free(token_array);
    
    // Free the token list
    while (token_list) {
        struct TokenRecord* to_free = token_list;
        token_list = token_list->next;
        free(to_free->token_text);
        free(to_free);
    }
    token_list = NULL;
}
%}

%option noyywrap
%option yylineno
%option noinput nounput



%%
"AND"|"and"|"&&"                     { record_token(AND, yytext); return AND; }
"OR"|"or"|"||"                       { record_token(OR, yytext); return OR; }
"NOT"|"not"|"~"                      { record_token(NOT, yytext); return NOT; }
"XOR"|"xor"                          { record_token(XOR, yytext); return XOR; }
"XNOR"|"xnor"                        { record_token(XNOR, yytext); return XNOR; }

"->"|"-->"|"==>"|"=>"|"IMPLIES"|"implies" { record_token(IMPLIES, yytext); return IMPLIES; }
"<->"|"<=>"|"<-->"|"<==>"|"DOUBLEIMPLIES"|"D_IMPLIES" { record_token(IFF, yytext); return IFF; }

"="|"EQUALS"|"equals"                                 { record_token(ASSIGN, yytext); return ASSIGN; }
"==="|"EQUIVALENT"|"equivalent"      { record_token(EQUIV, yytext); return EQUIV; }

"E_Q"                                { record_token(EXISTS, yytext); return EXISTS; }
"U_Q"                                { record_token(FORALL, yytext); return FORALL; }

"IF"|"if"                            { record_token(IF, yytext); return IF; }
"IFF"|"iff"                          { record_token(IFF_KEYWORD, yytext); return IFF_KEYWORD; }

"("                                  { record_token(LPAREN, yytext); return LPAREN; }
")"                                  { record_token(RPAREN, yytext); return RPAREN; }

"true"|"TRUE"    { yylval.bool_val = 1; record_token(T_TRUE, yytext); return T_TRUE; }
"false"|"FALSE"  { yylval.bool_val = 0; record_token(T_FALSE, yytext); return T_FALSE; }


[A-Za-z_][A-Za-z0-9_]*               { yylval.str = strdup(yytext); record_token(IDENTIFIER, yytext); return IDENTIFIER; }

[ \t\n\r]+                           ;  // skips whitespace

.   {
    fprintf(stderr, "Lexer error: Unrecognized character '%s' at line %d\n", yytext, yylineno);
    record_token(INVALID_TOKEN, yytext);
    return INVALID_TOKEN;
}

%%
