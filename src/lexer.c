#include "../include/lexer.h"
#include <ctype.h>
#include <string.h>

// Initialize the lexer with source code
void lexer_init(Lexer *lex, const char *source, Arena *arena) {
    lex->source = source;
    lex->length = strlen(source);
    lex->pos = 0;
    lex->line = 1;
    lex->column = 1;
    
    // Push the base indentation level (0 spaces) onto the stack
    lex->indent_stack[0] = 0;
    lex->indent_top = 0;
    
    lex->arena = arena;
}

// Peek at current character without advancing
static char peek(Lexer *lex) {
    if (lex->pos >= lex->length) return '\0';
    return lex->source[lex->pos];
}

// Peek at the next character (one ahead)
static char peek_next(Lexer *lex) {
    if (lex->pos + 1 >= lex->length) return '\0';
    return lex->source[lex->pos + 1];
}

// Advance and return current character
static char advance(Lexer *lex) {
    char c = lex->source[lex->pos];
    lex->pos++;
    lex->column++;
    if (c == '\n') {
        lex->line++;
        lex->column = 1;
    }
    return c;
}

// Check if we're at the end of source
static int is_at_end(Lexer *lex) {
    return lex->pos >= lex->length;
}

// Check if a scanned word is a keyword or a generic identifier
static TokenType lookup_keyword(const char *word) {
    struct { const char *name; TokenType type; } keywords[] = {
        {"set", TOKEN_SET},
        {"to", TOKEN_TO},
        {"as", TOKEN_AS},
        {"define", TOKEN_DEFINE},
        {"function", TOKEN_FUNCTION},
        {"returns", TOKEN_RETURNS},
        {"structure", TOKEN_STRUCTURE},
        {"of", TOKEN_OF},
        {"declare", TOKEN_DECLARE},
        {"pointer", TOKEN_POINTER},
        {"address", TOKEN_ADDRESS},
        {"value", TOKEN_VALUE},
        {"at", TOKEN_AT},
        {"array", TOKEN_ARRAY},
        {"if", TOKEN_IF},
        {"else", TOKEN_ELSE},
        {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR},
        {"do", TOKEN_DO},
        {"switch", TOKEN_SWITCH},
        {"on", TOKEN_ON},
        {"case", TOKEN_CASE},
        {"default", TOKEN_DEFAULT},
        {"break", TOKEN_BREAK},
        {"return", TOKEN_RETURN},
        {"include", TOKEN_INCLUDE},
        {"expecting", TOKEN_EXPECTING},
        {"nothing", TOKEN_NOTHING},
        {"increment", TOKEN_INCREMENT},
        {"decrement", TOKEN_DECREMENT},
        {"add", TOKEN_ADD},
        {"subtract", TOKEN_SUBTRACT},
        {"multiply", TOKEN_MULTIPLY},
        {"divide", TOKEN_DIVIDE},
        {"by", TOKEN_BY},
        {"from", TOKEN_FROM},
        {"constant", TOKEN_CONSTANT},
        {"size", TOKEN_SIZE},
        {"cast", TOKEN_CAST},
        {"string", TOKEN_STRING_KW},
        {"continue", TOKEN_CONTINUE},
        {"goto", TOKEN_GOTO},
        {"enumeration", TOKEN_ENUMERATION},
        {"alias", TOKEN_ALIAS},
        {"union", TOKEN_UNION_KW},
        {"static", TOKEN_STATIC_KW},
        {"extern", TOKEN_EXTERN_KW},
        {"volatile", TOKEN_VOLATILE_KW},
        {"register", TOKEN_REGISTER_KW},
        {"inline", TOKEN_INLINE_KW},
        {"int", TOKEN_INT},
        {"float", TOKEN_FLOAT},
        {"double", TOKEN_DOUBLE},
        {"char", TOKEN_CHAR},
        {"void", TOKEN_VOID},
        {"long", TOKEN_LONG},
        {"short", TOKEN_SHORT},
        {"unsigned", TOKEN_UNSIGNED},
        {"signed", TOKEN_SIGNED},
        {"bool", TOKEN_BOOL},
        {"true", TOKEN_TRUE},
        {"false", TOKEN_FALSE},
    };

    int count = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < count; i++) {
        if (strcmp(word, keywords[i].name) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token *scan_word(Lexer *lex) {
    int start = lex->pos;
    int start_col = lex->column;
    while (!is_at_end(lex) && (isalnum(peek(lex)) || peek(lex) == '_')) {
        advance(lex);
    }
    int len = lex->pos - start;
    char *word = arena_alloc(lex->arena, len + 1);
    memcpy(word, lex->source + start, len);
    word[len] = '\0';

    Token *tok = arena_alloc(lex->arena, sizeof(Token));
    tok->type = lookup_keyword(word);
    tok->value = word;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

static Token *scan_number(Lexer *lex) {
    int start = lex->pos;
    int start_col = lex->column;
    while (!is_at_end(lex) && isdigit(peek(lex))) {
        advance(lex);
    }
    // Handle decimal point

    if (peek(lex) == '.' && isdigit(peek_next(lex))) {
        advance(lex); // consume '.'
        while (!is_at_end(lex) && isdigit(peek(lex))) {
            advance(lex);
        }
    }

    int len = lex->pos - start;
    char *num = arena_alloc(lex->arena, len + 1);
    memcpy(num, lex->source + start, len);
    num[len] = '\0';

    Token *tok = arena_alloc(lex->arena, sizeof(Token));
    tok->type = TOKEN_NUMBER;
    tok->value = num;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

static Token *scan_string(Lexer *lex) {
    int start_col = lex->column;
    advance(lex);
    int start = lex->pos;
    while (!is_at_end(lex) && peek(lex) != '"') {
        if (peek(lex) == '\\') advance(lex);
        advance(lex);    
    }
    int len = lex->pos - start;
    char *str = arena_alloc(lex->arena, len + 1);
    memcpy(str, lex->source + start, len);
    str[len] = '\0';
    advance(lex);

    Token *tok = arena_alloc(lex->arena, sizeof(Token));
    tok->type = TOKEN_STRING_LIT;
    tok->value = str;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

static Token *scan_char(Lexer *lex) {
    int start_col = lex->column;
    advance(lex); // Skip the opening '
    int start = lex->pos;
    while (!is_at_end(lex) && peek(lex) != '\'') {
        if (peek(lex) == '\\') advance(lex); // Handle escaped chars like '\n', '\t', etc.
        advance(lex);
    }
    int len = lex->pos - start;
    char *str = arena_alloc(lex->arena, len + 1);
    memcpy(str, lex->source + start, len);
    str[len] = '\0';
    advance(lex); // Skip the closing '

    Token *tok = arena_alloc(lex->arena, sizeof(Token));
    tok->type = TOKEN_CHAR_LIT;
    tok->value = str;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

// Helper to create a token and allocate its string value in the arena
static Token *make_token(Lexer *lex, TokenType type, const char *value, int start_col) {
    Token *tok = (Token *)arena_alloc(lex->arena, sizeof(Token));
    tok->type = type;
    
    int len = strlen(value);
    char *val_copy = (char *)arena_alloc(lex->arena, len + 1);
    memcpy(val_copy, value, len + 1);
    tok->value = val_copy;
    
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

// Helper to match a character and advance if it matches
static int match(Lexer *lex, char expected) {
    if (is_at_end(lex)) return 0;
    if (lex->source[lex->pos] != expected) return 0;
    lex->pos++;
    lex->column++;
    return 1;
}

static void handle_indentation(Lexer *lex, Vec *tokens) {
    // Count spaces at start of the line
    int depth = 0;
    while (!is_at_end(lex) && peek(lex) == ' ') {
        advance(lex);
        depth++;
    }
    // If the line is blank, or is just a comment, skip it
    if (is_at_end(lex) || peek(lex) == '\n' || (peek(lex) == '/' && peek_next(lex) == '/')) {
        return;
    }

    int current_depth = lex->indent_stack[lex->indent_top];

    if (depth > current_depth) {
        // Push new depth and emit INDENT
        lex->indent_top++;
        lex->indent_stack[lex->indent_top] = depth;
        vec_push(tokens, make_token(lex, TOKEN_INDENT, "", 1));
    } else if (depth < current_depth) {
        // Pop and emit DEDENT for each level we come back up
        while (lex->indent_top > 0 && lex->indent_stack[lex->indent_top] > depth) {
            lex->indent_top--;
            vec_push(tokens, make_token(lex, TOKEN_DEDENT, "", 1));
        }
    }
} 

void lexer_tokenize(Lexer *lex, Vec *tokens) {
    // Handle indentation at start of file
    handle_indentation(lex, tokens);

    while (!is_at_end(lex)) {
        char c = peek(lex);

        // Skip spaces (within a line, not at start)
        if (c == ' ' || c == '\t') {
            advance(lex);
            continue;
        }

        // Newline - emit NEWLINE token, then handle indentation of next line
        if (c == '\n') {
            vec_push(tokens, make_token(lex, TOKEN_NEWLINE, "\\n", lex->column));
            advance(lex);

            // Skip blank lines
            while (!is_at_end(lex) && peek(lex) == '\n') {
                advance(lex);
            }
            if (!is_at_end(lex)) {
                handle_indentation(lex, tokens);
            }
            continue;
        }

        // Comments (using //)
        if (c == '/' && peek_next(lex) == '/') {
            while (!is_at_end(lex) && peek(lex) != '\n') {
                advance(lex);
            }
            continue;    
        }

        // Words (identifiers and keywords)
        if (isalpha(c) || c == '_') {
            vec_push(tokens, scan_word(lex));
            continue;
        }

        // Numbers
        if (isdigit(c)) {
            vec_push(tokens, scan_number(lex));
            continue;
        }

        // String literals
        if (c == '"') {
            vec_push(tokens, scan_string(lex));
            continue;
        }

        // Char literals
        if (c == '\'') {
            vec_push(tokens, scan_char(lex));
            continue;
        }

        // Operators and punctuation
        int start_col = lex->column;
        char op = advance(lex);

        switch (op) {
            case '(': vec_push(tokens, make_token(lex, TOKEN_LPAREN, "(", start_col)); break;
            case ')': vec_push(tokens, make_token(lex, TOKEN_RPAREN, ")", start_col)); break;
            case '[': vec_push(tokens, make_token(lex, TOKEN_LBRACKET, "[", start_col)); break;
            case ']': vec_push(tokens, make_token(lex, TOKEN_RBRACKET, "]", start_col)); break;
            case '{': vec_push(tokens, make_token(lex, TOKEN_LBRACE, "{", start_col)); break;
            case '}': vec_push(tokens, make_token(lex, TOKEN_RBRACE, "}", start_col)); break;
            case ',': vec_push(tokens, make_token(lex, TOKEN_COMMA, ",", start_col)); break;
            case ':': vec_push(tokens, make_token(lex, TOKEN_COLON, ":", start_col)); break;
            case ';': vec_push(tokens, make_token(lex, TOKEN_SEMICOLON, ";", start_col)); break;
            case '?': vec_push(tokens, make_token(lex, TOKEN_QUESTION, "?", start_col)); break;
            case '~': vec_push(tokens, make_token(lex, TOKEN_TILDE, "~", start_col)); break;
            case '#': vec_push(tokens, make_token(lex, TOKEN_HASH, "#", start_col)); break;

            case '=':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_EQUAL, "==", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_ASSIGN, "=", start_col));
                }
                break;

            case '!':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_NOT_EQUAL, "!=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_NOT, "!", start_col));
                }
                break;

            case '+':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_PLUS_ASSIGN, "+=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_PLUS, "+", start_col));
                }
                break;

            case '-':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_MINUS_ASSIGN, "-=", start_col));
                } else if (match(lex, '>')) {
                    vec_push(tokens, make_token(lex, TOKEN_ARROW, "->", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_MINUS, "-", start_col));
                }
                break;

            case '*':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_STAR_ASSIGN, "*=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_STAR, "*", start_col));
                }
                break;

            case '/':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_SLASH_ASSIGN, "/=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_SLASH, "/", start_col));
                }
                break;

            case '%':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_PERCENT_ASSIGN, "%=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_PERCENT, "%", start_col));
                }
                break;

            case '&':
                if (match(lex, '&')) {
                    vec_push(tokens, make_token(lex, TOKEN_AND, "&&", start_col));
                } else if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_AMP_ASSIGN, "&=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_AMPERSAND, "&", start_col));
                }
                break;

            case '|':
                if (match(lex, '|')) {
                    vec_push(tokens, make_token(lex, TOKEN_OR, "||", start_col));
                } else if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_PIPE_ASSIGN, "|=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_PIPE, "|", start_col));
                }
                break;

            case '^':
                if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_CARET_ASSIGN, "^=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_CARET, "^", start_col));
                }
                break;

            case '<':
                if (match(lex, '<')) {
                    if (match(lex, '=')) {
                        vec_push(tokens, make_token(lex, TOKEN_LSHIFT_ASSIGN, "<<=", start_col));
                    } else {
                        vec_push(tokens, make_token(lex, TOKEN_LSHIFT, "<<", start_col));
                    }
                } else if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_LESS_EQUAL, "<=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_LESS, "<", start_col));
                }
                break;

            case '>':
                if (match(lex, '>')) {
                    if (match(lex, '=')) {
                        vec_push(tokens, make_token(lex, TOKEN_RSHIFT_ASSIGN, ">>=", start_col));
                    } else {
                        vec_push(tokens, make_token(lex, TOKEN_RSHIFT, ">>", start_col));
                    }
                } else if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_GREATER_EQUAL, ">=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_GREATER, ">", start_col));
                }
                break;

            case '.':
                if (peek(lex) == '.' && peek_next(lex) == '.') {
                    advance(lex);
                    advance(lex);
                    vec_push(tokens, make_token(lex, TOKEN_ELLIPSIS, "...", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_DOT, ".", start_col));
                }
                break;

            default: {
                char unknown[2] = {op, '\0'};
                vec_push(tokens, make_token(lex, TOKEN_ERROR, unknown, start_col));
                break;
            }
        }
    }

    // Emit remaining DEDENT tokens to close all open blocks
    while (lex->indent_top > 0) {
        vec_push(tokens, make_token(lex, TOKEN_DEDENT, "", lex->column));
        lex->indent_top--;
    }

    // EOF token
    vec_push(tokens, make_token(lex, TOKEN_EOF, "", lex->column));
}