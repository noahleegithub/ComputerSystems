#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "tokenize.h"
#include "svec.h"

int
is_shell_op(char c) 
{
    // We won't check for && and || right now, it'll be dealt with by
    // read_operator.
    return c == '<' || c == '>' || c == '|' || c == '&' || c == ';'
        || c == '\\'|| c == '=' || c == '$';
}

char*
read_quoted_str(const char* line, int ii) 
{
    int nn = 1;
    while (line[ii + nn] != '\"') {
        if (line[ii + nn] == 0) {
            abort(); // We reached the end of the line, but no closing quote.
        }
        ++nn;
    }

    char* str = malloc(nn + 2);
    memcpy(str, line + ii, nn + 1);
    str[nn + 1] = '\0';
    return str;
}

char*
read_str(const char* line, int ii)
{
    int nn = 0;
    while(line[ii + nn] && !isspace(line[ii + nn]) && line[ii + nn] != '"' &&
            !is_shell_op(line[ii + nn]) && line[ii + nn] != '(' &&
                line[ii + nn] != ')') {
        ++nn;
    }

    char* str = malloc(nn + 1);
    memcpy(str, line + ii, nn);
    str[nn] = '\0';
    return str;
}



char*
read_operator(const char* line, int ii) 
{
    int nn = 0;
    while (is_shell_op(line[ii + nn])) {
        ++nn;
    }
    
    char* op = malloc(nn + 1);
    memcpy(op, line + ii, nn);
    op[nn] = '\0';
    return op;
}

svec*
tokenize(const char* line)
{

    svec* sv = make_svec();
    int nn = strlen(line);
    int ii = 0;

    while (ii < nn) {
        if (isspace(line[ii])) {
            ++ii;
        }
        else if (line[ii] == '\"') {
            char* str = read_quoted_str(line, ii);
            svec_push_back(sv, str);
            ii += strlen(str);
            free(str);
        }
        else if (line[ii] == '(' || line[ii] == ')') {
            char paren[2] = "_";
            paren[0] = line[ii];
            svec_push_back(sv, paren);
            ++ii;
        }
        else if (is_shell_op(line[ii])) {
            char* op = read_operator(line, ii);
            svec_push_back(sv, op);
            ii += strlen(op);
            free(op);
        }
        else { // line[ii] == first char of command name/arg
            char* str = read_str(line, ii);
            svec_push_back(sv, str);
            ii += strlen(str);
            free(str);
        }
    }
 
    return sv;
}
