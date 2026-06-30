#include "../include/c_lexer.h"
#include <string.h>
#include <ctype.h>

void c_lexer_init(CLexer *lex, const char *source, Arena *arena) {
    lex->source = source;
    lex->length = strlen(source);
    lex->pos = 0;
    lex->line = 1;
    lex->column = 1;
    lex->arena = arena;
}

// Helpers
static char peek(CLexer *lex) {
    if (lex->pos >= lex->length) return '\0';
    return lex->source[lex->pos];
}

static char peek_next(CLexer *lex) {
    if (lex->pos + 1 >= lex->length) return '\0';
    return lex->source[lex->pos + 1];
}

static char advance(CLexer *lex) {
    char c = lex->source[lex->pos];
    lex->pos++;
    lex->column++;
    if (c == '\n') {
        lex->line++;
        lex->column = 1;
    }
    return c;
}

static int is_at_end(CLexer *lex) {
    return lex->pos >= lex->length;
}

static int match(CLexer *lex, char expected) {
    if (is_at_end(lex)) return 0;
    if (lex->source[lex->pos] != expected) return 0;
    lex->pos++;
    lex->column++;
    return 1;
}

static TokenType lookup_c_keyword(const char *word) {
    struct { const char *name; TokenType type; } keywords[] = {
        // Shared keywords (same word in SpeakC and C17)
        {"if",        TOKEN_IF},
        {"else",      TOKEN_ELSE},
        {"while",     TOKEN_WHILE},
        {"for",       TOKEN_FOR},
        {"do",        TOKEN_DO},
        {"switch",    TOKEN_SWITCH},
        {"case",      TOKEN_CASE},
        {"default",   TOKEN_DEFAULT},
        {"break",     TOKEN_BREAK},
        {"continue",  TOKEN_CONTINUE},
        {"return",    TOKEN_RETURN},
        {"goto",      TOKEN_GOTO},

        // C type keywords (shared)
        {"int",       TOKEN_INT},
        {"float",     TOKEN_FLOAT},
        {"double",    TOKEN_DOUBLE},
        {"char",      TOKEN_CHAR},
        {"void",      TOKEN_VOID},
        {"long",      TOKEN_LONG},
        {"short",     TOKEN_SHORT},
        {"unsigned",  TOKEN_UNSIGNED},
        {"signed",    TOKEN_SIGNED},
        {"_Bool",     TOKEN_BOOL},

        // C-specific keywords (have different names in SpeakC)
        {"struct",    TOKEN_C_STRUCT},
        {"const",     TOKEN_C_CONST},
        {"sizeof",    TOKEN_C_SIZEOF},
        {"NULL",      TOKEN_C_NULL},
        {"typedef",   TOKEN_C_TYPEDEF},
        {"enum",      TOKEN_C_ENUM},
        {"union",     TOKEN_C_UNION},
        {"static",    TOKEN_C_STATIC},
        {"extern",    TOKEN_C_EXTERN},
        {"volatile",  TOKEN_C_VOLATILE},
        {"register",  TOKEN_C_REGISTER},
        {"inline",    TOKEN_C_INLINE},

        // Boolean literals
        {"true",      TOKEN_TRUE},
        {"false",     TOKEN_FALSE},
    };

    int count = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < count; i++) {
        if (strcmp(word, keywords[i].name) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token *make_token(CLexer *lex, TokenType type, const char *value, int start_col) {
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

static Token *scan_word(CLexer *lex) {
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
    tok->type = lookup_c_keyword(word);
    tok->value = word;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

static Token *scan_number(CLexer *lex) {
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

static Token *scan_string(CLexer *lex) {
    int start_col = lex->column;
    advance(lex); // Skip opening '"'
    int start = lex->pos;
    while (!is_at_end(lex) && peek(lex) != '"') {
        if (peek(lex) == '\\') advance(lex);
        advance(lex);
    }
    int len = lex->pos - start;
    char *str = arena_alloc(lex->arena, len + 1);
    memcpy(str, lex->source + start, len);
    str[len] = '\0';
    advance(lex); // Skip closing '"'

    Token *tok = arena_alloc(lex->arena, sizeof(Token));
    tok->type = TOKEN_STRING_LIT;
    tok->value = str;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

static Token *scan_char(CLexer *lex) {
    int start_col = lex->column;
    advance(lex); // Skip opening '\''
    int start = lex->pos;
    while (!is_at_end(lex) && peek(lex) != '\'') {
        if (peek(lex) == '\\') advance(lex);
        advance(lex);
    }
    int len = lex->pos - start;
    char *str = arena_alloc(lex->arena, len + 1);
    memcpy(str, lex->source + start, len);
    str[len] = '\0';
    advance(lex); // Skip closing '\''

    Token *tok = arena_alloc(lex->arena, sizeof(Token));
    tok->type = TOKEN_CHAR_LIT;
    tok->value = str;
    tok->line = lex->line;
    tok->column = start_col;
    return tok;
}

static Token *scan_preprocessor(CLexer *lex) {
    int start_col = lex->column;
    advance(lex); // skip '#'

    // Skip whitespace between # and directive name
    while (!is_at_end(lex) && peek(lex) == ' ') advance(lex);

    // Read the directive name (include, define, etc.)
    int start = lex->pos;
    while (!is_at_end(lex) && isalpha(peek(lex))) advance(lex);
    int len = lex->pos - start;
    char directive[32] = "";
    if (len < 31) {
        memcpy(directive, lex->source + start, len);
        directive[len] = '\0';
    }

    // Skip whitespace after directive name
    while (!is_at_end(lex) && peek(lex) == ' ') advance(lex);

    if (strcmp(directive, "include") == 0) {
        // Capture everything until end of line: <stdio.h> or "myfile.h"
        int val_start = lex->pos;
        while (!is_at_end(lex) && peek(lex) != '\n') advance(lex);
        int val_len = lex->pos - val_start;
        char *value = arena_alloc(lex->arena, val_len + 1);
        memcpy(value, lex->source + val_start, val_len);
        value[val_len] = '\0';

        Token *tok = arena_alloc(lex->arena, sizeof(Token));
        tok->type = TOKEN_INCLUDE;
        tok->value = value;
        tok->line = lex->line;
        tok->column = start_col;
        return tok;
    }

    if (strcmp(directive, "define") == 0) {
        // Capture everything until end of line
        int val_start = lex->pos;
        while (!is_at_end(lex) && peek(lex) != '\n') advance(lex);
        int val_len = lex->pos - val_start;
        char *value = arena_alloc(lex->arena, val_len + 1);
        memcpy(value, lex->source + val_start, val_len);
        value[val_len] = '\0';

        Token *tok = arena_alloc(lex->arena, sizeof(Token));
        tok->type = TOKEN_DEFINE;
        tok->value = value;
        tok->line = lex->line;
        tok->column = start_col;
        return tok;
    }

    // Unknown preprocessor directive — store as error
    return make_token(lex, TOKEN_ERROR, directive, start_col);
}

void c_lexer_tokenize(CLexer *lex, Vec *tokens) {
    while (!is_at_end(lex)) {
        char c = peek(lex);

        // Skip whitespace (spaces, tabs, newlines)
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance(lex);
            continue;
        }

        // Single-line comments (//)
        if (c == '/' && peek_next(lex) == '/') {
            while (!is_at_end(lex) && peek(lex) != '\n') advance(lex);
            continue;
        }

        // Block comments (/* ... */)
        if (c == '/' && peek_next(lex) == '*') {
            advance(lex); // skip /
            advance(lex); // skip *
            while (!is_at_end(lex)) {
                if (peek(lex) == '*' && peek_next(lex) == '/') {
                    advance(lex); // skip /
                    advance(lex); // skip *
                    break;
                }
                advance(lex);
            }
            continue;
        }

        // Preprocessor lines (#include, #define)
        if (c == '#') {
            vec_push(tokens, scan_preprocessor(lex));
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
                if (match(lex, '+')) {
                    vec_push(tokens, make_token(lex, TOKEN_PLUS_PLUS, "++", start_col));
                } else if (match(lex, '=')) {
                    vec_push(tokens, make_token(lex, TOKEN_PLUS_ASSIGN, "+=", start_col));
                } else {
                    vec_push(tokens, make_token(lex, TOKEN_PLUS, "+", start_col));
                }
                break;

            case '-':
                if (match(lex, '-')) {
                    vec_push(tokens, make_token(lex, TOKEN_MINUS_MINUS, "--", start_col));
                } else if (match(lex, '=')) {
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

    // EOF token
    vec_push(tokens, make_token(lex, TOKEN_EOF, "", lex->column));
}