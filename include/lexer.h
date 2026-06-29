#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "util/vec.h"
#include "util/arena.h"


typedef struct {
    const char *source; // The full source code string
    int length; // Length of source
    int pos; // current position in the source
    int line; // current line number
    int column; // current column (1-indexed)

    // Indentation tracking
    int indent_stack[256]; // Stack of indentation depths
    int indent_top; // Top of indent stack

    Arena *arena; // For allocating token value strings

} Lexer;

// Initialize the lexer with source code

void lexer_init(Lexer *lex, const char *source, Arena *arena);

// Tokenize the entire source into a Vec of Token*
void lexer_tokenize(Lexer *lex, Vec *token);

#endif