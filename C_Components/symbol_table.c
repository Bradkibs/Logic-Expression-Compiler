#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

SymbolTable *init_symbol_table()
{
    SymbolTable *table = malloc(sizeof(SymbolTable));
    if (table == NULL)
    {

        return;
    }
    table->count = 0;
    return table;
}

void add_or_update_symbol(SymbolTable *table, const char *name, bool value)
{
    if (table == NULL)
    {

        return;
    }
    // First, check if the symbol already exists
    for (int i = 0; i < table->count; i++)
    {
        if (strcmp(table->symbols[i].name, name) == 0)
        {
            table->symbols[i].value = value;
            table->symbols[i].is_defined = true;
            return;
        }
    }

    if (table->count < MAX_SYMBOLS)
    {
        table->symbols[table->count].name = strdup(name);
        table->symbols[table->count].value = value;
        table->symbols[table->count].is_defined = true;
        table->count++;
    }
}

bool get_symbol_value(SymbolTable *table, const char *name, bool *value)
{
    if (table == NULL)
    {

        return false;
    }
    for (int i = 0; i < table->count; i++)
    {
        if (strcmp(table->symbols[i].name, name) == 0)
        {
            if (table->symbols[i].is_defined)
            {
                *value = table->symbols[i].value;
                return true;
            }
            return false;
        }
    }
    return false; // Symbol not found
}

void free_symbol_table(SymbolTable *table)
{
    if (table == NULL)
    {

        return;
    }
    for (int i = 0; i < table->count; i++)
    {
        free(table->symbols[i].name);
    }
    free(table);
}