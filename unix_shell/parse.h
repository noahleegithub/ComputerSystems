#include "svec.h"
#include "ast_stack.h"

#ifndef PARSE_H
#define PARSE_H

ast_stack* parse_toks(svec* tokens);

#endif
