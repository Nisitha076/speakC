#ifndef C_PARSER_H
#define C_PARSER_H

#include "token.h"
#include "ast.h"
#include "util/vec.h"
#include "util/arena.h"

typedef struct {
    Vec *tokens;           // Token list from the C17 lexer
    int pos;               // Current position in token list
    Arena *arena;          // For allocating AST nodes
    const char *filename;  // Source file path (for error messages)
} CParser;

void c_parser_init(CParser *p, Vec *tokens, Arena *arena, const char *filename);
ASTNode *c_parser_parse(CParser *p);  // Returns the root AST node (NODE_PROGRAM)

#endif