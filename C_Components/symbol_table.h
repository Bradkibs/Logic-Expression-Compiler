#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#define MAX_SYMBOLS 100
#define MAX_SYMBOL_NAME_LENGTH 50
#define ERROR_SYMBOL_TABLE_FULL -1
#define ERROR_SYMBOL_NOT_DEFINED -2
#define ERROR_SYMBOL_NOT_FOUND -3

typedef struct {
    char name[MAX_SYMBOL_NAME_LENGTH];  // Fixed-length name to simplify memory management
    int value;
} Symbol;

typedef struct {
    Symbol *symbols;  // Dynamic array of symbols
    int size;         // Current number of symbols
    int capacity;     // Total allocated capacity
} SymbolTable;

SymbolTable *init_symbol_table();
int add_or_update_symbol(SymbolTable *table, const char *name, int value);
int get_symbol_value(SymbolTable *table, const char *name);
void free_symbol_table(SymbolTable *table);

#endif /* SYMBOL_TABLE_H */