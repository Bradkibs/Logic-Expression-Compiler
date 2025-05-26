#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 10

SymbolTable *init_symbol_table()
{
    // Allocate memory for symbol table
    SymbolTable *table = malloc(sizeof(SymbolTable));
    if (table == NULL) {
        // Handle memory allocation failure
        return NULL;
    }

    // Initialize table properties with an initial capacity
    table->capacity = INITIAL_CAPACITY;
    table->size = 0;
    table->symbols = malloc(table->capacity * sizeof(Symbol));
    
    if (table->symbols == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

int add_or_update_symbol(SymbolTable *table, const char *name, int value)
{
    if (!table || !name) {
        return ERROR_SYMBOL_NOT_DEFINED;
    }
    
    // Check if table->symbols exists
    if (!table->symbols) {
        table->capacity = INITIAL_CAPACITY;
        table->symbols = malloc(table->capacity * sizeof(Symbol));
        if (!table->symbols) {
            return ERROR_SYMBOL_TABLE_FULL;
        }
        table->size = 0;
    }
    
    // Check if symbol already exists
    for (int i = 0; i < table->size; i++)
    {
        if (strcmp(table->symbols[i].name, name) == 0)
        {
            table->symbols[i].value = value;
            return 0; // Success
        }
    }

    // Resize if needed
    if (table->size >= table->capacity) {
        int new_capacity = table->capacity * 2;
        Symbol *new_symbols = realloc(table->symbols, new_capacity * sizeof(Symbol));
        if (!new_symbols) {
            return ERROR_SYMBOL_TABLE_FULL;
        }
        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }

    // Add new symbol
    strncpy(table->symbols[table->size].name, name, MAX_SYMBOL_NAME_LENGTH - 1);
    table->symbols[table->size].name[MAX_SYMBOL_NAME_LENGTH - 1] = '\0';
    table->symbols[table->size].value = value;
    table->size++;

    return 0; // Success
}

int get_symbol_value(SymbolTable *table, const char *name)
{
    if (!table || !table->symbols || !name) {
        return ERROR_SYMBOL_NOT_FOUND;
    }
    
    // Handle special case for TRUE/FALSE constants
    if (strcmp(name, "TRUE") == 0) {
        return 1;
    } else if (strcmp(name, "FALSE") == 0) {
        return 0;
    }
    
    for (int i = 0; i < table->size; i++)
    {
        if (strcmp(table->symbols[i].name, name) == 0)
        {
            return table->symbols[i].value;
        }
    }

    return ERROR_SYMBOL_NOT_FOUND;
}

void free_symbol_table(SymbolTable *table)
{
    if (table) {
        free(table->symbols);
        free(table);
    }
}