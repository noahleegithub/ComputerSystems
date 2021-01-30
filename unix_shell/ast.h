#include "hashmap.h"

#ifndef AST_H
#define AST_H

typedef struct ast {
    char* op;
    struct ast* arg0;
    struct ast* arg1;
    char* val;
} ast;

ast* make_ast_val(const char* val);
ast* make_ast_op(const char* op, ast* arg0, ast* arg1);
void free_ast(ast* ast);

int eval_ast(ast* ast, hashmap* variables);
void print_ast(ast* ast);



#endif
