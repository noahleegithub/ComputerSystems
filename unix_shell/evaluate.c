/* Shell operators are written following the pseudocode from Nat Tuck's Shell
 * Hints lecture.
 */

#include <stdlib.h>
#include <stdio.h>
#include <bsd/string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "hashmap.h"
#include "tokenize.h"
#include "evaluate.h"
#include "ast.h"

int
exec_command(char* argv[]) 
{
    int cpid;
    if ((cpid = fork())) {
        if (cpid < 0) {
            return -1;
        }
        int st;
        waitpid(cpid, &st, 0);
        return st;
    }
    else {     
        return execvp(argv[0], argv);
    }
}

int
eval_base(char* val, hashmap* variables) {
    svec* tokens = tokenize(val);
  
    char** argv = calloc(tokens->size + 1, sizeof(char*));
    int jj = 0;
    for (int ii = 0; ii < tokens->size; ++ii) {
        // Load variable mappings from hashmap
        if (strcmp(tokens->data[ii], "$") == 0) {
            if (hashmap_has(variables, tokens->data[ii + 1])) {
                argv[jj] = strdup(hashmap_get(variables, tokens->data[ii + 1]));
                ++ii;
                ++jj;
            }
            else {
                free(argv);
                free_svec(tokens);
                return -1;
            }
        }
        // Get rid of extraneous double quotes around string
        else if (tokens->data[ii][0] == '"') {
            int len = strlen(tokens->data[ii]);
            argv[jj] = strdup(tokens->data[ii] + 1);
            argv[jj][len - 2] = '\0';
            ++jj;
        }
        else {
            argv[jj] = strdup(tokens->data[ii]);
            ++jj;
        }
    }

    int rv = 0;
    if (!argv[0]) {
        rv = -1;
    } 
    else if (strcmp(argv[0], "cd") == 0) {
        if (!argv[1]) {
            rv = -1;
        }
        else {
            rv = chdir(argv[1]);
        }
    }
    else if (strcmp(argv[0], "exit") == 0) {
        // To exit gracefully by freeing all alloced memory, return -1.
        rv = -1;
    }
    else {
        rv = exec_command(argv);
    }  

    for (int ii = 0; argv[ii] != 0; ++ii) {
        free(argv[ii]);
    }
    free(argv);
    free_svec(tokens);
    return rv;
}

int
semicolon(ast* ast, hashmap* variables) 
{
    int rv1 = evaluate(ast->arg0, variables);
    if (rv1 == -1) {
        return -1;
    }
    int rv2 = evaluate(ast->arg1, variables);
    return rv2;
}

int 
and(ast* ast, hashmap* variables)
{
    int rv1 = evaluate(ast->arg0, variables);
    if (rv1 == -1) {
        return -1;
    }
    else if (rv1 != 0) {
        return rv1;
    }
    return evaluate(ast->arg1, variables);
}

int
or(ast* ast, hashmap* variables) 
{
    int rv1 = evaluate(ast->arg0, variables);
    if (rv1 == -1) {
        return -1;
    }
    else if (rv1 == 0) {
        return rv1;
    }
    return evaluate(ast->arg1, variables);
}

int 
background(ast* ast, hashmap* variables)
{
    int cpid;
    if ((cpid = fork())) {
        if (cpid < 0) {
            return -1;
        }
        
        return evaluate(ast->arg1, variables);
    }
    else {
        close(0);
        evaluate(ast->arg0, variables);
        return -1;
    }
}

int
redirect_in(ast* ast, hashmap* variables)
{
    if (!ast->arg1) {
        return -1;
    }
    char* filepath = ast->arg1->val;

    int cpid;
    if ((cpid = fork())) {
        if (cpid < 0) {
            return -1;
        }
        int st;
        waitpid(cpid, &st, 0);
        return st;
    }
    else { 
        
        int fd = open(filepath, O_RDONLY);
        if (fd == -1) {
            printf("Couldn't open file specified by %s\n", filepath);
            return -1;
        }

        dup2(fd, 0);
        close(fd);
        evaluate(ast->arg0, variables);
        return -1;
    }
}

int
redirect_out(ast* ast, hashmap* variables)
{
    if (!ast->arg1) {
        return -1;
    }
    char* filepath = ast->arg1->val;

    int cpid;
    if ((cpid = fork())) {
        if (cpid < 0) {
            return -1;
        }
        int st;
        waitpid(cpid, &st, 0);
        return st;
    }
    else { 
        close(0);
        int fd = open(filepath, O_CREAT | O_TRUNC | O_RDWR, 0666);
        if (fd == -1) {
            printf("Couldn't open file specified by %s\n", filepath);
            return -1;
        }

        dup2(fd, 1);
        close(fd);
        evaluate(ast->arg0, variables);
        return -1;
    }
}

int
pipe_cmd(ast* ast, hashmap* variables)
{
    int c0pid;
    if ((c0pid = fork())) {
        if (c0pid < 0) {
            return -1;
        }
        
        int st;
        waitpid(c0pid, &st, 0);
        return st;
    }
    else {
        
        int pipefds[2];
        if (pipe(pipefds) == -1) {
            return -1;
        }
        dup2(pipefds[0], 0);
        
        int c1pid;
        int c2pid;
        if ((c1pid = fork())) {
            // Fall through
        }
        else {
            dup2(pipefds[1], 1);
            close(pipefds[0]);
            close(pipefds[1]);
            evaluate(ast->arg0, variables);
            return -1;
        }

        if ((c2pid = fork())) {
            // Fall through
        }
        else {
            dup2(pipefds[0], 0);
            close(pipefds[1]);
            close(pipefds[0]);
            evaluate(ast->arg1, variables);
            return -1;
        }

        close(pipefds[0]);
        close(pipefds[1]);
        int st;
        waitpid(c1pid, 0, 0);
        waitpid(c2pid, &st, 0);
        return -1;
    }
}

int
assignment(ast* ast, hashmap* variables) 
{
    if (ast->arg0 && ast->arg1) {
        hashmap_put(variables, ast->arg0->val, ast->arg1->val);
        return 0;
    }
    return -1;
}

int
evaluate(ast* ast, hashmap* variables)
{
    if (!ast) {
        return 0;
    }
    else if (ast->op == 0) {
        return eval_base(ast->val, variables);
    }
    else {
        if (strcmp(ast->op, ";") == 0) {
            return semicolon(ast, variables);
        }
        else if (strcmp(ast->op, "&") == 0) {
            return background(ast, variables);
        }
        else if (strcmp(ast->op, "||") == 0) {
            return or(ast, variables);
        }
        else if (strcmp(ast->op, "&&") == 0) {
            return and(ast, variables);
        }
        else if (strcmp(ast->op, "|") == 0) {
            return pipe_cmd(ast, variables);
        }
        else if (strcmp(ast->op, "<") == 0) {
            return redirect_in(ast, variables);
        }
        else if (strcmp(ast->op, ">") == 0) {
            return redirect_out(ast, variables);
        }
        else if (strcmp(ast->op, "=") == 0) {
            return assignment(ast, variables);
        }
        else {
            return -1; // For invalid operator
        }
    }
}
