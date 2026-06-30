#ifndef C_LEXER_H
#define C_LEXER_H

#include "token.h"
#include "util/vec.h"
#include "util/arena.h"


typedef struct {
    const char *source;    // The full C17 source code
    int length;            // Length of source
    int pos;               // Current position
    int line;              // Current line (1-indexed)
    int column;            // Current column (1-indexed)
    Arena *arena;          // For allocating token strings
} CLexer;

// Initialize the C17 lexer
void c_lexer_init(CLexer *lex, const char *source, Arena *arena);


// Tokenize C17 source into a Vec of Token*
void c_lexer_tokenize(CLexer *lex, Vec *tokens);

#endif