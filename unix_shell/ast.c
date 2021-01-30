/* Idea of ASTs taken from Nat Tuck's CS3650 Calculator Code lecture
 * But modified to work with char* values
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bsd/string.h>

#include "evaluate.h"
#include "ast.h"

ast* 
make_ast_val(const char* val) 
{
    ast* val_ast = calloc(1, sizeof(ast));
    val_ast->val = strdup(val);
    return val_ast;
}

ast* 
make_ast_op(const char* op, ast* arg0, ast* arg1)
{
    ast* op_ast = calloc(1, sizeof(ast));
    op_ast->op = strdup(op);
    op_ast->arg0 = arg0;
    op_ast->arg1 = arg1;
    return op_ast;
}

void 
free_ast(ast* ast) 
{
    if (ast) {
        free(ast->op);
        free(ast->val);
        free_ast(ast->arg0);
        free_ast(ast->arg1);
        free(ast);
    }
}

int
eval_ast(ast* ast, hashmap* variables)
{
   return evaluate(ast, variables);
}

char* 
ast_tostr(ast* ast)
{
    if (!ast) {
        char* str = malloc(4 * sizeof(char));
        strlcpy(str, "nul", 4);
        return str;
    }
    if (!ast->op) {
        char* str = malloc((strlen(ast->val) + 1) * sizeof(char));
        strlcpy(str, ast->val, strlen(ast->val) + 1);
        return str;
    }
    else {
        char* str0 = ast_tostr(ast->arg0);
        char* str1 = ast_tostr(ast->arg1);
        
        char* out = malloc((strlen(str0) + strlen(str1) + strlen(ast->op) + 5) 
                * sizeof(char));
        sprintf(out, "(%s %s %s)", str0, ast->op, str1);

        free(str0);
        free(str1);
        return out;
    }
}

void 
print_ast(ast* ast)
{
    char* str = ast_tostr(ast);
    printf("%s\n", str);
    free(str);
}
