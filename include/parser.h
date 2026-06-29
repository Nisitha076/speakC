#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include "util/vec.h"
#include "util/arena.h"

typedef struct {
    Vec *tokens; // Token list from the lexer
    int pos; // Current posistion in token list
    Arena *arena; // For allocating AST nodes
} Parser;

void parser_init(Parser *p, Vec *tokens, Arena *arena);
ASTNode *parser_parse(Parser *p); // Returns the root AST node 

#endif