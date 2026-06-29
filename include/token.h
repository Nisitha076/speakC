#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    // Keywords
    TOKEN_SET,           // "set"
    TOKEN_TO,            // "to"
    TOKEN_AS,            // "as"
    TOKEN_DEFINE,        // "define"
    TOKEN_FUNCTION,      // "function"
    TOKEN_RETURNS,       // "returns"
    TOKEN_STRUCTURE,     // "structure"
    TOKEN_OF,            // "of"
    TOKEN_DECLARE,       // "declare"
    TOKEN_POINTER,       // "pointer"
    TOKEN_ADDRESS,       // "address"
    TOKEN_VALUE,         // "value"
    TOKEN_AT,            // "at"
    TOKEN_ARRAY,         // "array"
    TOKEN_IF,            // "if"
    TOKEN_ELSE,          // "else"
    TOKEN_WHILE,         // "while"
    TOKEN_FOR,           // "for"
    TOKEN_DO,            // "do"
    TOKEN_SWITCH,        // "switch"
    TOKEN_ON,            // "on"
    TOKEN_CASE,          // "case"
    TOKEN_DEFAULT,       // "default"
    TOKEN_BREAK,         // "break"
    TOKEN_RETURN,        // "return"
    TOKEN_INCLUDE,       // "include"
    TOKEN_EXPECTING,     // "expecting"
    TOKEN_NOTHING,       // "nothing"
    TOKEN_INCREMENT,     // "increment"
    TOKEN_DECREMENT,     // "decrement"
    TOKEN_ADD,           // "add"
    TOKEN_SUBTRACT,      // "subtract"
    TOKEN_MULTIPLY,      // "multiply"
    TOKEN_DIVIDE,        // "divide"
    TOKEN_BY,            // "by"
    TOKEN_FROM,          // "from"
    TOKEN_CONSTANT,      // "constant"
    TOKEN_SIZE,          // "size"
    TOKEN_CAST,          // "cast"
    TOKEN_STRING_KW,     // "string" (the keyword, not a literal)
    TOKEN_CONTINUE,      // "continue"
    TOKEN_GOTO,          // "goto"
    TOKEN_ENUMERATION,   // "enumeration" (SpeakC keyword for C's enum)
    TOKEN_ALIAS,         // "alias" (SpeakC keyword for C's typedef)
    TOKEN_UNION_KW,      // "union"
    TOKEN_STATIC_KW,     // "static"
    TOKEN_EXTERN_KW,     // "extern"
    TOKEN_VOLATILE_KW,   // "volatile"
    TOKEN_REGISTER_KW,   // "register"
    TOKEN_INLINE_KW,     // "inline"

    // Types (C types used as keywords in SpeakC)
    TOKEN_INT,           // "int"
    TOKEN_FLOAT,         // "float"
    TOKEN_DOUBLE,        // "double"
    TOKEN_CHAR,          // "char"
    TOKEN_VOID,          // "void"
    TOKEN_LONG,          // "long"
    TOKEN_SHORT,         // "short"
    TOKEN_UNSIGNED,      // "unsigned"
    TOKEN_SIGNED,        // "signed"
    TOKEN_BOOL,          // "_Bool" / "bool"
    TOKEN_TRUE,          // "true"
    TOKEN_FALSE,         // "false"

    // Literals
    TOKEN_NUMBER,        // 42, 3.14
    TOKEN_STRING_LIT,    // "hello"
    TOKEN_CHAR_LIT,      // 'A'
    TOKEN_IDENTIFIER,    // variable/function names

    // Operators
    TOKEN_ASSIGN,        // =
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_STAR,          // *
    TOKEN_SLASH,         // /
    TOKEN_PERCENT,       // %
    TOKEN_EQUAL,         // ==
    TOKEN_NOT_EQUAL,     // !=
    TOKEN_LESS,          // <
    TOKEN_GREATER,       // >
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_AND,           // &&
    TOKEN_OR,            // ||
    TOKEN_NOT,           // !
    TOKEN_AMPERSAND,     // &
    TOKEN_PIPE,          // |
    TOKEN_TILDE,         // ~
    TOKEN_CARET,         // ^
    TOKEN_LSHIFT,        // <<
    TOKEN_RSHIFT,        // >>
    TOKEN_QUESTION,      // ?

    // Compound Assignment Operators
    TOKEN_PLUS_ASSIGN,    // +=
    TOKEN_MINUS_ASSIGN,   // -=
    TOKEN_STAR_ASSIGN,    // *=
    TOKEN_SLASH_ASSIGN,   // /=
    TOKEN_PERCENT_ASSIGN, // %=
    TOKEN_AMP_ASSIGN,     // &=
    TOKEN_PIPE_ASSIGN,    // |=
    TOKEN_CARET_ASSIGN,   // ^=
    TOKEN_LSHIFT_ASSIGN,  // <<=
    TOKEN_RSHIFT_ASSIGN,  // >>=

    // Punctuation
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACKET,      // [
    TOKEN_RBRACKET,      // ]
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_COMMA,         // ,
    TOKEN_COLON,         // :
    TOKEN_SEMICOLON,     // ;  (needed for C17 backward lexer)
    TOKEN_DOT,           // .
    TOKEN_ARROW,         // -> (needed for C17 backward lexer)
    TOKEN_HASH,          // #  (needed for C17 backward lexer)
    TOKEN_ELLIPSIS,      // ... (variadic functions)

    // Indentation (virtual tokens)
    TOKEN_INDENT,        // Indentation increased
    TOKEN_DEDENT,        // Indentation decreased
    TOKEN_NEWLINE,       // End of a line

    // Special
    TOKEN_EOF,           // End of file
    TOKEN_ERROR,         // Lexer error
} TokenType;

typedef struct {
    TokenType type;
    char *value;     // The raw text of the token
    int line;        // Source line (1-indexed)
    int column;      // Source column (1-indexed)
} Token;

// Helper: get a human-readable name for a token type (for debugging)
const char *token_type_name(TokenType type);

#endif