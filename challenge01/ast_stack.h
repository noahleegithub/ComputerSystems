#include "ast.h"

#ifndef AST_STACK_H
#define AST_STACK_H

typedef struct ast_stack {
    int size;
    ast** data;
    int capacity;
} ast_stack;

ast_stack* make_ast_stack();
void free_ast_stack(ast_stack* astk);

void ast_stack_push(ast_stack* astk, ast* item);
ast* ast_stack_peek(ast_stack* astk);
ast* ast_stack_pop(ast_stack* astk);
ast_stack* ast_stack_rev(ast_stack* astk);

#endif
