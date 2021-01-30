/* Basically an SVec that contains ASTs.
 * One difference from a normal SVec is that only stack operations are supported
 */

#include <stdlib.h>

#include "ast.h"
#include "ast_stack.h"
    
ast_stack* 
make_ast_stack() 
{
    ast_stack* astk = malloc(sizeof(ast_stack));
    astk->size = 0;
    astk->capacity = 4;
    astk->data = malloc(astk->capacity * sizeof(ast*));
    return astk;
}

void
free_ast_stack(ast_stack* astk)
{
    // Not taking ownership of ASTs, so no freeing them.
    free(astk->data);
    free(astk);
}

void 
ast_stack_push(ast_stack* astk, ast* item)
{
    if (astk->size == astk->capacity) {
        astk->capacity = astk->capacity * 2;
        void* tmp = realloc(astk->data, astk->capacity * sizeof(ast*));
        if (tmp == NULL) {
            return;
        }
        astk->data = tmp;
    }
    astk->size = astk->size + 1;
    for (int ii = astk->size - 1; ii > 0; --ii) {
        astk->data[ii] = astk->data[ii - 1];
    }
    astk->data[0] = item;
}

ast* 
ast_stack_peek(ast_stack* astk)
{
    if (astk->size == 0) {
        return 0;
    }
    return astk->data[0];
}

ast* 
ast_stack_pop(ast_stack* astk)
{
    ast* temp = ast_stack_peek(astk);
    for (int ii = 0; ii < astk->size - 1; ++ii) {
        astk->data[ii] = astk->data[ii + 1];
    }
    astk->size = astk->size - 1;
    return temp;
}

ast_stack*
ast_stack_rev(ast_stack* astk)
{
    ast_stack* new_stk = make_ast_stack();
    int ii = astk->size;
    while (ii > 0) {
        ast_stack_push(new_stk, ast_stack_pop(astk));
        --ii;
    }
    free_ast_stack(astk);
    return new_stk;
}


