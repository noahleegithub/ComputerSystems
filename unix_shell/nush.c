#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "hashmap.h"
#include "ast_stack.h"
#include "parse.h"
#include "ast.h"
#include "svec.h"
#include "tokenize.h"

int
process_cmd(svec* tokens, hashmap* variables) {
    ast_stack* parsed_trees = parse_toks(tokens);
    int rv = 0;
    while (rv != -1 && ast_stack_peek(parsed_trees)) {
        ast* cmd = ast_stack_peek(parsed_trees);
        
        rv = eval_ast(cmd, variables);
        free_ast(cmd);
        ast_stack_pop(parsed_trees);
    }
    while (ast_stack_peek(parsed_trees)) {
        free_ast(ast_stack_peek(parsed_trees));
        ast_stack_pop(parsed_trees);
    }
    free_ast_stack(parsed_trees); 
    return rv;
}

void
read_input(FILE* in, const char* prompt, const char* endin)
{
    char cmd[256];
    svec* token_buffer = make_svec();
    hashmap* variables = make_hashmap();

    while (1) {
        printf(prompt);
        fflush(stdout);
        
        char* rv = fgets(cmd, 256, in);
        
        if (!rv) {
            printf(endin);
            break;
        } 
        if (strcmp(cmd, "\n") == 0) {
            continue;
        }
        svec* tokens = tokenize(cmd);
        token_buffer = copy_over(token_buffer, tokens);
        free_svec(tokens);
        if (strcmp(svec_get(token_buffer, token_buffer->size - 1), "\\") == 0) {
            svec_remove(token_buffer, token_buffer->size - 1);
            continue;
        }
        else {
            if (process_cmd(token_buffer, variables) == -1) {
                break;
            }
      
            free_svec(token_buffer);
            token_buffer = make_svec();
        }
    }
    free_hashmap(variables);
    free_svec(token_buffer);
}

int
main(int argc, char* argv[])
{
    if (argc > 2) {
        printf("Usage: %s\n", argv[0]);
        return 1;
    }
    else if (argc == 2) {
        int fd = open(argv[1], O_RDONLY);
        
        if (fd == -1) {
            // Error: fopen didn't open argv[1]
            printf("Couldn't open file specified by %s\n", argv[1]);
            return 1;
        }
        
        dup2(fd, 0);
        close(fd);

        read_input(stdin, 0, 0);
    }
    else {
        read_input(stdin, "nush$ ", "\n");
    }
    
    return 0;
}
