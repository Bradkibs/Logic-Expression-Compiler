#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#define MAX_SYMBOLS 256
#define SUCCESS 0
#define ERROR_NULL_POINTER -1
#define ERROR_MEMORY_ALLOCATION -2
#define ERROR_SYMBOL_TABLE_FULL -3
#define ERROR_SYMBOL_NOT_FOUND -4

typedef struct
{
    char *name;
    bool value;
    bool is_defined;
} Symbol;

typedef struct
{
    Symbol symbols[MAX_SYMBOLS];
    int count;
} SymbolTable;

SymbolTable *init_symbol_table();
void add_or_update_symbol(SymbolTable *table, const char *name, bool value);
bool get_symbol_value(SymbolTable *table, const char *name, bool *value);
void free_symbol_table(SymbolTable *table);

#endif /* SYMBOL_TABLE_H */