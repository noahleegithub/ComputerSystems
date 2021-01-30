#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "svec.h"
#include "ast_stack.h"
#include "ast.h"

int
str_equal(const char* s1, const char* s2) 
{
    return strcmp(s1, s2) == 0;
}

int
operator_precedence(const char* tok) 
{
   if (str_equal(tok, "(") || str_equal(tok, ")")) {
        return 1;
   }
   else if (str_equal(tok, "=")) {
        return 6;
   }
   else if (str_equal(tok, "<") || str_equal(tok, ">")) {
        return 5;
   }
   else if (str_equal(tok, "|")) {
        return 4;
   }
   else if (str_equal(tok, "||") || str_equal(tok, "&&")) {
        return 3;
   }
   else if (str_equal(tok, "&") || str_equal(tok, ";")) {
        return 2;
   }
   else {
        return 0;
   }
}

/* Simple preprocessing to combine adjacent bash commands into a single char*.
 * Leaves bash operators alone.
 */
svec*
combine_tokens(svec* tokens)
{
    svec* new_toks = make_svec();

    for (int ii = 0; ii < tokens->size; ++ii) {
        if (new_toks->size > 0 
                && operator_precedence(svec_peek(new_toks)) == 0
                && operator_precedence(svec_get(tokens, ii)) == 0) {
            char* last = svec_peek(new_toks);
           
            char* combined = malloc((strlen(last) 
                        + strlen(svec_get(tokens, ii)) + 2) * sizeof(char));
            sprintf(combined, "%s %s\0", last, svec_get(tokens, ii));
            svec_pop(new_toks);
            svec_push(new_toks, combined);
            free(combined);
        }
        else {
            svec_push(new_toks, svec_get(tokens, ii));
        }
    }

    return rev_free_svec(new_toks);
}

/* Pops one operator off the stack, pops 2 (or 1) operands off the operand
 * stack, then combines them into one AST and pushes it back onto the operand
 * stack.
 */
void
apply_op(ast_stack* operand_stack, ast_stack* operator_stack)
{
    ast* operator = ast_stack_peek(operator_stack);
    ast_stack_pop(operator_stack);
    
    ast* operand0 = 0;
    ast* operand1 = 0;
    if (operator_precedence(operator->op) == 2) {
        operand1 = ast_stack_peek(operand_stack);
        ast_stack_pop(operand_stack);
        if (ast_stack_peek(operand_stack)) {
            operand0 = ast_stack_peek(operand_stack);
            ast_stack_pop(operand_stack);
        }
        else {
            operand0 = operand1;
            operand1 = 0;
        }
    }
    else {
        operand1 = ast_stack_peek(operand_stack);
        ast_stack_pop(operand_stack);
        operand0 = ast_stack_peek(operand_stack);
        ast_stack_pop(operand_stack);
    }

    operator->arg0 = operand0;
    operator->arg1 = operand1;
    ast_stack_push(operand_stack, operator);
}    

/* Parses an SVec of tokens into a stack of multiple ASTs.
 * Uses two stacks to track operators and operands, and combines them into ASTs.
 */
ast_stack* 
parse_toks(svec* tokens)
{
    svec* combined = combine_tokens(tokens);
   
    ast_stack* operator_stack = make_ast_stack();
    ast_stack* operand_stack = make_ast_stack();

    
    for (int ii = 0; ii < combined->size; ++ii) {
        if (operator_precedence(svec_get(combined, ii)) == 0) {
            // Bash command case
            ast_stack_push(operand_stack, make_ast_val(svec_get(combined, ii)));
        }
        else {
            // Bash operator case
            if (str_equal(svec_get(combined, ii), "(")) {
                ast_stack_push(operator_stack, 
                        make_ast_op(svec_get(combined, ii), 0, 0));
            }
            else if (str_equal(svec_get(combined, ii), ")")) {
                while (ast_stack_peek(operator_stack)
                        && !str_equal(ast_stack_peek(operator_stack)->op,
                            "(")) {
                    apply_op(operand_stack, operator_stack);
                }
                free_ast(ast_stack_peek(operator_stack));
                ast_stack_pop(operator_stack);
            }
            else {
                while (ast_stack_peek(operator_stack) 
                        && operator_precedence(svec_get(combined, ii)) <= 
                        operator_precedence(ast_stack_peek(operator_stack)->op)) {
                    apply_op(operand_stack, operator_stack);
                }
                ast_stack_push(operator_stack, 
                        make_ast_op(svec_get(combined, ii), 0, 0));
            }
         }
    }
    while (ast_stack_peek(operator_stack)) {
        apply_op(operand_stack, operator_stack);
    }

    free_svec(combined);
    free_ast_stack(operator_stack);
    return ast_stack_rev(operand_stack);
}

