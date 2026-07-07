#include "../include/c_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void c_parser_init(CParser *p, Vec *tokens, Arena *arena, const char *filename) {
    p->tokens = tokens;
    p->pos = 0;
    p->arena = arena;
    p->filename = filename;
}

// Get current token without advancing
static Token *current(CParser *p) {
    return (Token *)vec_get(p->tokens, p->pos);
}

// Get current token and advance
static Token *advance_parser(CParser *p) {
    Token *tok = current(p);
    if (tok->type != TOKEN_EOF) p->pos++;
    return tok;
}

// Check if current token matches a type (without consuming)
static int check(CParser *p, TokenType type) {
    return current(p)->type == type;
}

// If current token matches, consume and return 1. Otherwise return 0.
static int match(CParser *p, TokenType type) {
    if (check(p, type)) {
        advance_parser(p);
        return 1;
    }
    return 0;
}

// Expect a specific token type. Print error if wrong.
static Token *expect(CParser *p, TokenType type) {
    if (check(p, type)) {
        return advance_parser(p);
    }
    Token *got = current(p);
    fprintf(stderr, "%s:%d:%d: error: expected %s, got %s (\"%s\")\n",
            p->filename, got->line, got->column,
            token_type_name(type),
            token_type_name(got->type),
            got->value);
    return got;
}

static ASTNode *parse_c_expression(CParser *p);
static ASTNode *parse_c_statement(CParser *p);
static ASTNode *parse_c_block(CParser *p);
static ASTNode *parse_c_function(CParser *p, ASTNode *return_type, Token *name);
static ASTNode *parse_c_do_while(CParser *p);
static ASTNode *parse_c_switch(CParser *p);

// Check if a token is a type keyword
static int is_type_token(TokenType type) {
    return type == TOKEN_INT || type == TOKEN_FLOAT ||
           type == TOKEN_DOUBLE || type == TOKEN_CHAR ||
           type == TOKEN_VOID || type == TOKEN_LONG ||
           type == TOKEN_SHORT || type == TOKEN_UNSIGNED ||
           type == TOKEN_SIGNED || type == TOKEN_BOOL ||
           type == TOKEN_C_STRUCT || type == TOKEN_C_CONST ||
           type == TOKEN_C_ENUM || type == TOKEN_C_UNION;
}

// Parse a C type and return a NODE_TYPE tree
static ASTNode *parse_c_type(CParser *p) {
    int line = current(p)->line;
    int is_const = 0;

    // Check for "const" prefix
    if (check(p, TOKEN_C_CONST)) {
        advance_parser(p);
        is_const = 1;
    }

    // Check for "struct", "enum", "union" prefix
    ASTNode *base_type = NULL;
    if (check(p, TOKEN_C_STRUCT)) {
        advance_parser(p);
        Token *name = expect(p, TOKEN_IDENTIFIER);
        base_type = ast_new(p->arena, NODE_TYPE, name->value, line);
    } else if (check(p, TOKEN_C_ENUM)) {
        advance_parser(p);
        Token *name = expect(p, TOKEN_IDENTIFIER);
        base_type = ast_new(p->arena, NODE_TYPE, name->value, line);
    } else if (check(p, TOKEN_C_UNION)) {
        advance_parser(p);
        Token *name = expect(p, TOKEN_IDENTIFIER);
        base_type = ast_new(p->arena, NODE_TYPE, name->value, line);
    } else {
        // Regular type: int, float, char, unsigned long, etc.
        // Handle multi-word types like "unsigned int", "long long"
        Token *tok = advance_parser(p);
        char type_str[64] = "";
        strncpy(type_str, tok->value, sizeof(type_str) - 1);

        // Keep consuming type tokens for multi-word types
        while (is_type_token(current(p)->type) &&
               current(p)->type != TOKEN_C_STRUCT &&
               current(p)->type != TOKEN_C_CONST) {
            tok = advance_parser(p);
            strncat(type_str, " ", sizeof(type_str) - strlen(type_str) - 1);
            strncat(type_str, tok->value, sizeof(type_str) - strlen(type_str) - 1);
        }

        base_type = ast_new(p->arena, NODE_TYPE, type_str, line);
    }

    // Wrap with const if needed
    if (is_const) {
        ASTNode *const_type = ast_new(p->arena, NODE_TYPE, "const", line);
        ast_add_child(p->arena, const_type, base_type);
        base_type = const_type;
    }

    // Handle pointer stars: int *, int **, etc.
    while (check(p, TOKEN_STAR)) {
        advance_parser(p);
        ASTNode *ptr_type = ast_new(p->arena, NODE_TYPE, "pointer", line);
        ast_add_child(p->arena, ptr_type, base_type);
        base_type = ptr_type;
    }

    return base_type;
}

// Parse a primary expression (literals, identifiers, unary ops)
static ASTNode *parse_c_primary(CParser *p) {
    Token *tok = current(p);
    int line = tok->line;

    // Number literal
    if (check(p, TOKEN_NUMBER)) {
        advance_parser(p);
        return ast_new(p->arena, NODE_NUMBER_LIT, tok->value, line);
    }

    // String literal
    if (check(p, TOKEN_STRING_LIT)) {
        advance_parser(p);
        return ast_new(p->arena, NODE_STRING_LIT, tok->value, line);
    }

    // Char literal
    if (check(p, TOKEN_CHAR_LIT)) {
        advance_parser(p);
        return ast_new(p->arena, NODE_CHAR_LIT, tok->value, line);
    }

    // NULL
    if (check(p, TOKEN_C_NULL)) {
        advance_parser(p);
        return ast_new(p->arena, NODE_IDENTIFIER, "nothing", line);
    }

    // true / false
    if (check(p, TOKEN_TRUE)) {
        advance_parser(p);
        return ast_new(p->arena, NODE_IDENTIFIER, "true", line);
    }
    if (check(p, TOKEN_FALSE)) {
        advance_parser(p);
        return ast_new(p->arena, NODE_IDENTIFIER, "false", line);
    }

    // sizeof(type)
    if (check(p, TOKEN_C_SIZEOF)) {
        advance_parser(p);
        expect(p, TOKEN_LPAREN);
        ASTNode *type = parse_c_type(p);
        expect(p, TOKEN_RPAREN);
        ASTNode *node = ast_new(p->arena, NODE_SIZEOF, NULL, line);
        ast_add_child(p->arena, node, type);
        return node;
    }

    // Unary & (address of)
    if (check(p, TOKEN_AMPERSAND)) {
        advance_parser(p);
        ASTNode *operand = parse_c_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_ADDRESS_OF, NULL, line);
        ast_add_child(p->arena, node, operand);
        return node;
    }

    // Unary * (dereference)
    if (check(p, TOKEN_STAR)) {
        advance_parser(p);
        ASTNode *operand = parse_c_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_DEREF, NULL, line);
        ast_add_child(p->arena, node, operand);
        return node;
    }

    // Unary ! (logical not)
    if (check(p, TOKEN_NOT)) {
        advance_parser(p);
        ASTNode *operand = parse_c_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_UNARY_OP, "!", line);
        ast_add_child(p->arena, node, operand);
        return node;
    }

    // Unary - (negative)
    if (check(p, TOKEN_MINUS)) {
        advance_parser(p);
        ASTNode *operand = parse_c_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_UNARY_OP, "-", line);
        ast_add_child(p->arena, node, operand);
        return node;
    }

    // Cast: (type)expression
    // This is tricky — (int)x looks like a parenthesized expression
    // Hint: if the token after ( is a type keyword, it's a cast
    if (check(p, TOKEN_LPAREN) && is_type_token(((Token *)vec_get(p->tokens, p->pos + 1))->type)) {
        advance_parser(p); // skip (
        ASTNode *type = parse_c_type(p);
        expect(p, TOKEN_RPAREN);
        ASTNode *expr = parse_c_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_CAST, NULL, line);
        ast_add_child(p->arena, node, expr);
        ast_add_child(p->arena, node, type);
        return node;
    }

    // Parenthesized expression: (expr)
    if (check(p, TOKEN_LPAREN)) {
        advance_parser(p);
        ASTNode *expr = parse_c_expression(p);
        expect(p, TOKEN_RPAREN);
        return expr;
    }

    // Identifier (might be followed by function call, array access, or member access)
    if (check(p, TOKEN_IDENTIFIER)) {
        advance_parser(p);
        ASTNode *node = ast_new(p->arena, NODE_IDENTIFIER, tok->value, line);

        // Handle postfix operators
        while (1) {
            // Function call: name(args)
            if (check(p, TOKEN_LPAREN)) {
                advance_parser(p);
                ASTNode *call = ast_new(p->arena, NODE_FUNC_CALL, tok->value, line);
                while (!check(p, TOKEN_RPAREN) && !check(p, TOKEN_EOF)) {
                    ASTNode *arg = parse_c_expression(p);
                    ast_add_child(p->arena, call, arg);
                    if (!check(p, TOKEN_RPAREN)) expect(p, TOKEN_COMMA);
                }
                expect(p, TOKEN_RPAREN);
                node = call;
                continue;
            }

            // Array access: name[index]
            if (check(p, TOKEN_LBRACKET)) {
                advance_parser(p);
                ASTNode *index = parse_c_expression(p);
                expect(p, TOKEN_RBRACKET);
                ASTNode *access = ast_new(p->arena, NODE_ARRAY_ACCESS, NULL, line);
                ast_add_child(p->arena, access, node);
                ast_add_child(p->arena, access, index);
                node = access;
                continue;
            }

            // Member access: obj.member
            if (check(p, TOKEN_DOT)) {
                advance_parser(p);
                Token *member = expect(p, TOKEN_IDENTIFIER);
                ASTNode *access = ast_new(p->arena, NODE_MEMBER_ACCESS, NULL, line);
                ast_add_child(p->arena, access, node);
                ast_add_child(p->arena, access,
                    ast_new(p->arena, NODE_IDENTIFIER, member->value, line));
                node = access;
                continue;
            }

            // Pointer member access: ptr->member
            if (check(p, TOKEN_ARROW)) {
                advance_parser(p);
                Token *member = expect(p, TOKEN_IDENTIFIER);
                // ptr->x becomes: member_access(deref(ptr), x)
                ASTNode *deref = ast_new(p->arena, NODE_DEREF, NULL, line);
                ast_add_child(p->arena, deref, node);
                ASTNode *access = ast_new(p->arena, NODE_MEMBER_ACCESS, NULL, line);
                ast_add_child(p->arena, access, deref);
                ast_add_child(p->arena, access,
                    ast_new(p->arena, NODE_IDENTIFIER, member->value, line));
                node = access;
                continue;
            }

            // Post-increment: i++
            if (check(p, TOKEN_PLUS_PLUS)) {
                advance_parser(p);
                ASTNode *inc = ast_new(p->arena, NODE_INCREMENT, NULL, line);
                ast_add_child(p->arena, inc, node);
                node = inc;
                continue;
            }

            // Post-decrement: i--
            if (check(p, TOKEN_MINUS_MINUS)) {
                advance_parser(p);
                ASTNode *dec = ast_new(p->arena, NODE_DECREMENT, NULL, line);
                ast_add_child(p->arena, dec, node);
                node = dec;
                continue;
            }

            break; // No more postfix operators
        }

        return node;
    }

    // Unknown token
    fprintf(stderr, "%s:%d:%d: error: unexpected token %s (\"%s\")\n",
            p->filename, tok->line, tok->column,
            token_type_name(tok->type), tok->value);
    advance_parser(p);
    return ast_new(p->arena, NODE_IDENTIFIER, "error", line);
}
static int get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_OR:             return 1;
        case TOKEN_AND:            return 2;
        case TOKEN_PIPE:           return 3;
        case TOKEN_CARET:          return 4;
        case TOKEN_AMPERSAND:      return 5;
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:      return 6;
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER_EQUAL:  return 7;
        case TOKEN_LSHIFT:
        case TOKEN_RSHIFT:         return 8;
        case TOKEN_PLUS:
        case TOKEN_MINUS:          return 9;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:        return 10;
        default:                   return 0;
    }
}

static const char *get_operator_string(TokenType type) {
    switch (type) {
        case TOKEN_PLUS:           return "+";
        case TOKEN_MINUS:          return "-";
        case TOKEN_STAR:           return "*";
        case TOKEN_SLASH:          return "/";
        case TOKEN_PERCENT:        return "%";
        case TOKEN_EQUAL:          return "==";
        case TOKEN_NOT_EQUAL:      return "!=";
        case TOKEN_LESS:           return "<";
        case TOKEN_GREATER:        return ">";
        case TOKEN_LESS_EQUAL:     return "<=";
        case TOKEN_GREATER_EQUAL:  return ">=";
        case TOKEN_AND:            return "&&";
        case TOKEN_OR:             return "||";
        case TOKEN_AMPERSAND:      return "&";
        case TOKEN_PIPE:           return "|";
        case TOKEN_CARET:          return "^";
        case TOKEN_LSHIFT:         return "<<";
        case TOKEN_RSHIFT:         return ">>";
        default:                   return "?";
    }
}

static ASTNode *parse_c_expression_prec(CParser *p, int min_prec) {
    ASTNode *left = parse_c_primary(p);

    while (1) {
        int prec = get_precedence(current(p)->type);
        if (prec <= min_prec) break;

        Token *op = advance_parser(p);
        ASTNode *right = parse_c_expression_prec(p, prec);

        ASTNode *bin = ast_new(p->arena, NODE_BINARY_OP,
                               get_operator_string(op->type), op->line);
        ast_add_child(p->arena, bin, left);
        ast_add_child(p->arena, bin, right);
        left = bin;
    }

    return left;
}

static ASTNode *parse_c_expression(CParser *p) {
    return parse_c_expression_prec(p, 0);
}

static ASTNode *parse_c_block(CParser *p) {
    int line = current(p)->line;
    ASTNode *block = ast_new(p->arena, NODE_BLOCK, NULL, line);

    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        ASTNode *stmt = parse_c_statement(p);
        if (stmt) ast_add_child(p->arena, block, stmt);
    }

    return block;
}

static int is_declaration(CParser *p) {
    TokenType t = current(p)->type;
    if (is_type_token(t)) return 1;

    // Check if it's a custom typedef type (e.g. Point pt; or Point *p;)
    if (t == TOKEN_IDENTIFIER && p->pos + 1 < p->tokens->count) {
        TokenType next = ((Token *)vec_get(p->tokens, p->pos + 1))->type;
        if (next == TOKEN_IDENTIFIER) return 1;
        if (next == TOKEN_STAR && p->pos + 2 < p->tokens->count) {
            TokenType next2 = ((Token *)vec_get(p->tokens, p->pos + 2))->type;
            if (next2 == TOKEN_IDENTIFIER || next2 == TOKEN_STAR) return 1;
        }
    }
    return 0;
}

static ASTNode *parse_c_declaration(CParser *p) {
    int line = current(p)->line;

    // Parse type
    ASTNode *type = parse_c_type(p);

    // Parse variable name
    Token *name = expect(p, TOKEN_IDENTIFIER);

    // Check for array: int arr[10];
    if (check(p, TOKEN_LBRACKET)) {
        advance_parser(p);
        Token *size = expect(p, TOKEN_NUMBER);
        expect(p, TOKEN_RBRACKET);

        // Wrap type as array type
        ASTNode *array_type = ast_new(p->arena, NODE_TYPE, "array", line);
        ASTNode *size_node = ast_new(p->arena, NODE_NUMBER_LIT, size->value, line);
        ast_add_child(p->arena, array_type, size_node);
        ast_add_child(p->arena, array_type, type);
        type = array_type;
    }

    // Check for initialization: int x = 5;
    if (match(p, TOKEN_ASSIGN)) {
        ASTNode *value = parse_c_expression(p);
        expect(p, TOKEN_SEMICOLON);

        ASTNode *node = ast_new(p->arena, NODE_VAR_INIT, NULL, line);
        ASTNode *name_node = ast_new(p->arena, NODE_IDENTIFIER, name->value, line);
        ast_add_child(p->arena, node, name_node);
        ast_add_child(p->arena, node, value);
        ast_add_child(p->arena, node, type);
        return node;
    }

    // Function definition: int add(int a, int b) { ... }
    if (check(p, TOKEN_LPAREN)) {
        return parse_c_function(p, type, name);
    }

    // Plain declaration: int x;
    expect(p, TOKEN_SEMICOLON);
    ASTNode *node = ast_new(p->arena, NODE_VAR_DECL, NULL, line);
    ASTNode *name_node = ast_new(p->arena, NODE_IDENTIFIER, name->value, line);
    ast_add_child(p->arena, node, name_node);
    ast_add_child(p->arena, node, type);
    return node;
}

static ASTNode *parse_c_if(CParser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_IF);
    expect(p, TOKEN_LPAREN);
    ASTNode *condition = parse_c_expression(p);
    expect(p, TOKEN_RPAREN);

    expect(p, TOKEN_LBRACE);
    ASTNode *body = parse_c_block(p);
    expect(p, TOKEN_RBRACE);

    ASTNode *node = ast_new(p->arena, NODE_IF, NULL, line);
    ast_add_child(p->arena, node, condition);
    ast_add_child(p->arena, node, body);

    // Handle else if / else
    while (check(p, TOKEN_ELSE)) {
        advance_parser(p);  // consume "else"

        if (check(p, TOKEN_IF)) {
            // else if
            advance_parser(p);  // consume "if"
            expect(p, TOKEN_LPAREN);
            ASTNode *elif_cond = parse_c_expression(p);
            expect(p, TOKEN_RPAREN);
            expect(p, TOKEN_LBRACE);
            ASTNode *elif_body = parse_c_block(p);
            expect(p, TOKEN_RBRACE);

            ast_add_child(p->arena, node, elif_cond);
            ast_add_child(p->arena, node, elif_body);
        } else {
            // else
            expect(p, TOKEN_LBRACE);
            ASTNode *else_body = parse_c_block(p);
            expect(p, TOKEN_RBRACE);
            ast_add_child(p->arena, node, else_body);
            break;  // else is always last
        }
    }

    return node;
}

static ASTNode *parse_c_while(CParser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_WHILE);
    expect(p, TOKEN_LPAREN);
    ASTNode *condition = parse_c_expression(p);
    expect(p, TOKEN_RPAREN);

    expect(p, TOKEN_LBRACE);
    ASTNode *body = parse_c_block(p);
    expect(p, TOKEN_RBRACE);

    ASTNode *node = ast_new(p->arena, NODE_WHILE, NULL, line);
    ast_add_child(p->arena, node, condition);
    ast_add_child(p->arena, node, body);
    return node;
}

static ASTNode *parse_c_for(CParser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_FOR);
    expect(p, TOKEN_LPAREN);

    // Init: int i = 0 or i = 0
    ASTNode *init;
    if (is_declaration(p)) {
        // parse type + name + = value, but DON'T consume the semicolon inside parse_c_declaration
        // Instead, handle it inline here
        ASTNode *type = parse_c_type(p);
        Token *name = expect(p, TOKEN_IDENTIFIER);
        expect(p, TOKEN_ASSIGN);
        ASTNode *value = parse_c_expression(p);
        expect(p, TOKEN_SEMICOLON);

        init = ast_new(p->arena, NODE_VAR_INIT, NULL, line);
        ast_add_child(p->arena, init, ast_new(p->arena, NODE_IDENTIFIER, name->value, line));
        ast_add_child(p->arena, init, value);
        ast_add_child(p->arena, init, type);
    } else {
        ASTNode *target = parse_c_expression(p);
        expect(p, TOKEN_ASSIGN);
        ASTNode *value = parse_c_expression(p);
        expect(p, TOKEN_SEMICOLON);

        init = ast_new(p->arena, NODE_VAR_ASSIGN, NULL, line);
        ast_add_child(p->arena, init, target);
        ast_add_child(p->arena, init, value);
    }

    // Condition: i < 10
    ASTNode *condition = parse_c_expression(p);
    expect(p, TOKEN_SEMICOLON);

    // Step: i++ or i += 1
    ASTNode *step = parse_c_expression(p);
    // The step might be i++ which parse_c_primary already handled as NODE_INCREMENT

    expect(p, TOKEN_RPAREN);

    expect(p, TOKEN_LBRACE);
    ASTNode *body = parse_c_block(p);
    expect(p, TOKEN_RBRACE);

    ASTNode *node = ast_new(p->arena, NODE_FOR, NULL, line);
    ast_add_child(p->arena, node, init);
    ast_add_child(p->arena, node, condition);
    ast_add_child(p->arena, node, step);
    ast_add_child(p->arena, node, body);
    return node;
}

static ASTNode *parse_c_function(CParser *p, ASTNode *return_type, Token *name) {
    int line = name->line;
    expect(p, TOKEN_LPAREN);

    ASTNode *func = ast_new(p->arena, NODE_FUNC_DEF, name->value, line);
    ast_add_child(p->arena, func, return_type);

    // Parse parameters: (int a, float b)
    while (!check(p, TOKEN_RPAREN) && !check(p, TOKEN_EOF)) {
        ASTNode *param_type = parse_c_type(p);
        Token *param_name = expect(p, TOKEN_IDENTIFIER);

        // Check for array parameter: int arr[]
        if (check(p, TOKEN_LBRACKET)) {
            advance_parser(p);
            if (check(p, TOKEN_NUMBER)) {
                Token *size = advance_parser(p);
                ASTNode *arr_type = ast_new(p->arena, NODE_TYPE, "array", line);
                ast_add_child(p->arena, arr_type,
                    ast_new(p->arena, NODE_NUMBER_LIT, size->value, line));
                ast_add_child(p->arena, arr_type, param_type);
                param_type = arr_type;
            }
            expect(p, TOKEN_RBRACKET);
        }

        ast_add_child(p->arena, func,
            ast_new(p->arena, NODE_IDENTIFIER, param_name->value, line));
        ast_add_child(p->arena, func, param_type);

        if (!check(p, TOKEN_RPAREN)) expect(p, TOKEN_COMMA);
    }
    expect(p, TOKEN_RPAREN);

    // Parse body
    expect(p, TOKEN_LBRACE);
    ASTNode *body = parse_c_block(p);
    expect(p, TOKEN_RBRACE);

    ast_add_child(p->arena, func, body);
    return func;
}
static ASTNode *parse_c_struct_def(CParser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_C_STRUCT);
    Token *name = expect(p, TOKEN_IDENTIFIER);
    expect(p, TOKEN_LBRACE);

    ASTNode *node = ast_new(p->arena, NODE_STRUCT_DEF, name->value, line);

    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        ASTNode *member_type = parse_c_type(p);
        Token *member_name = expect(p, TOKEN_IDENTIFIER);

        // Check for array member: int data[100];
        if (check(p, TOKEN_LBRACKET)) {
            advance_parser(p);
            Token *size = expect(p, TOKEN_NUMBER);
            expect(p, TOKEN_RBRACKET);
            ASTNode *arr_type = ast_new(p->arena, NODE_TYPE, "array", line);
            ast_add_child(p->arena, arr_type,
                ast_new(p->arena, NODE_NUMBER_LIT, size->value, line));
            ast_add_child(p->arena, arr_type, member_type);
            member_type = arr_type;
        }

        expect(p, TOKEN_SEMICOLON);

        ASTNode *member = ast_new(p->arena, NODE_VAR_DECL, NULL, line);
        ast_add_child(p->arena, member,
            ast_new(p->arena, NODE_IDENTIFIER, member_name->value, line));
        ast_add_child(p->arena, member, member_type);
        ast_add_child(p->arena, node, member);
    }
    expect(p, TOKEN_RBRACE);
    expect(p, TOKEN_SEMICOLON);

    return node;
}

static ASTNode *parse_c_do_while(CParser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_DO);
    expect(p, TOKEN_LBRACE);
    ASTNode *body = parse_c_block(p);
    expect(p, TOKEN_RBRACE);

    expect(p, TOKEN_WHILE);
    expect(p, TOKEN_LPAREN);
    ASTNode *condition = parse_c_expression(p);
    expect(p, TOKEN_RPAREN);
    expect(p, TOKEN_SEMICOLON);

    ASTNode *node = ast_new(p->arena, NODE_DO_WHILE, NULL, line);
    ast_add_child(p->arena, node, body);
    ast_add_child(p->arena, node, condition);
    return node;
}

static ASTNode *parse_c_switch(CParser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_SWITCH);
    expect(p, TOKEN_LPAREN);
    ASTNode *expr = parse_c_expression(p);
    expect(p, TOKEN_RPAREN);
    expect(p, TOKEN_LBRACE);

    ASTNode *switch_node = ast_new(p->arena, NODE_SWITCH, NULL, line);
    ast_add_child(p->arena, switch_node, expr);

    ASTNode *cases_block = ast_new(p->arena, NODE_BLOCK, NULL, line);
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_CASE)) {
            int case_line = current(p)->line;
            advance_parser(p); // consume case
            ASTNode *val = parse_c_expression(p);
            expect(p, TOKEN_COLON);
            ASTNode *case_node = ast_new(p->arena, NODE_CASE, NULL, case_line);
            ast_add_child(p->arena, case_node, val);
            ast_add_child(p->arena, cases_block, case_node);
        } else if (check(p, TOKEN_DEFAULT)) {
            int def_line = current(p)->line;
            advance_parser(p); // consume default
            expect(p, TOKEN_COLON);
            ASTNode *def_node = ast_new(p->arena, NODE_DEFAULT, NULL, def_line);
            ast_add_child(p->arena, cases_block, def_node);
        } else {
            ASTNode *stmt = parse_c_statement(p);
            if (stmt) ast_add_child(p->arena, cases_block, stmt);
        }
    }
    expect(p, TOKEN_RBRACE);
    ast_add_child(p->arena, switch_node, cases_block);
    return switch_node;
}

static ASTNode *parse_c_statement(CParser *p) {
    // Control flow
    if (check(p, TOKEN_IF))       return parse_c_if(p);
    if (check(p, TOKEN_WHILE))    return parse_c_while(p);
    if (check(p, TOKEN_FOR))      return parse_c_for(p);
    if (check(p, TOKEN_DO))       return parse_c_do_while(p);
    if (check(p, TOKEN_SWITCH))   return parse_c_switch(p);

    // Simple statements
    if (check(p, TOKEN_RETURN)) {
        int line = current(p)->line;
        advance_parser(p);
        ASTNode *node = ast_new(p->arena, NODE_RETURN, NULL, line);
        if (!check(p, TOKEN_SEMICOLON)) {
            ast_add_child(p->arena, node, parse_c_expression(p));
        }
        expect(p, TOKEN_SEMICOLON);
        return node;
    }

    if (check(p, TOKEN_BREAK)) {
        int line = current(p)->line;
        advance_parser(p);
        expect(p, TOKEN_SEMICOLON);
        return ast_new(p->arena, NODE_BREAK, NULL, line);
    }

    if (check(p, TOKEN_CONTINUE)) {
        int line = current(p)->line;
        advance_parser(p);
        expect(p, TOKEN_SEMICOLON);
        return ast_new(p->arena, NODE_CONTINUE, NULL, line);
    }

    // Declaration (starts with a type keyword)
    if (is_declaration(p)) {
        return parse_c_declaration(p);
    }

    // Expression statement (assignment, function call, etc.)
    int line = current(p)->line;
    ASTNode *expr = parse_c_expression(p);

    // Check for assignment: x = 5;
    if (check(p, TOKEN_ASSIGN)) {
        advance_parser(p);
        ASTNode *value = parse_c_expression(p);
        expect(p, TOKEN_SEMICOLON);
        ASTNode *node = ast_new(p->arena, NODE_VAR_ASSIGN, NULL, line);
        ast_add_child(p->arena, node, expr);
        ast_add_child(p->arena, node, value);
        return node;
    }

    // Check for compound assignment: x += 5;
    TokenType ct = current(p)->type;
    if (ct == TOKEN_PLUS_ASSIGN || ct == TOKEN_MINUS_ASSIGN ||
        ct == TOKEN_STAR_ASSIGN || ct == TOKEN_SLASH_ASSIGN ||
        ct == TOKEN_PERCENT_ASSIGN) {
        advance_parser(p);

        // Map C compound operator to SpeakC operator
        const char *speakc_op;
        if (ct == TOKEN_PLUS_ASSIGN)    speakc_op = "+=";
        else if (ct == TOKEN_MINUS_ASSIGN) speakc_op = "-=";
        else if (ct == TOKEN_STAR_ASSIGN)  speakc_op = "*=";
        else if (ct == TOKEN_SLASH_ASSIGN) speakc_op = "/=";
        else speakc_op = "%=";

        ASTNode *value = parse_c_expression(p);
        expect(p, TOKEN_SEMICOLON);

        ASTNode *node = ast_new(p->arena, NODE_COMPOUND_ASSIGN, speakc_op, line);
        ast_add_child(p->arena, node, expr);
        ast_add_child(p->arena, node, value);
        return node;
    }

    // Plain expression statement (like a function call): printf("hi");
    expect(p, TOKEN_SEMICOLON);

    // If it's an increment/decrement expression, return it directly
    if (expr->type == NODE_INCREMENT || expr->type == NODE_DECREMENT) {
        return expr;
    }

    return expr;
}

ASTNode *c_parser_parse(CParser *p) {
    ASTNode *program = ast_new(p->arena, NODE_PROGRAM, NULL, 1);

    while (!check(p, TOKEN_EOF)) {
        // Preprocessor directives (already tokenized as TOKEN_INCLUDE / TOKEN_DEFINE)
        if (check(p, TOKEN_INCLUDE)) {
            Token *tok = advance_parser(p);
            ASTNode *inc = ast_new(p->arena, NODE_INCLUDE, tok->value, tok->line);
            ast_add_child(p->arena, program, inc);
            continue;
        }

        if (check(p, TOKEN_DEFINE)) {
            Token *tok = advance_parser(p);
            // Parse #define NAME value from the token value
            ASTNode *macro = ast_new(p->arena, NODE_MACRO_DEFINE, tok->value, tok->line);
            ast_add_child(p->arena, program, macro);
            continue;
        }

        // Struct definition: struct Point { ... };
        if (check(p, TOKEN_C_STRUCT) &&
            p->pos + 2 < p->tokens->count &&
            ((Token *)vec_get(p->tokens, p->pos + 2))->type == TOKEN_LBRACE) {
            ast_add_child(p->arena, program, parse_c_struct_def(p));
            continue;
        }

        // Typedef: typedef struct Point { ... } Point;
        if (check(p, TOKEN_C_TYPEDEF)) {
            advance_parser(p); // consume typedef
            if (check(p, TOKEN_C_STRUCT)) {
                // Parse struct def but handle the alias
                int line = current(p)->line;
                expect(p, TOKEN_C_STRUCT);
                Token *name = expect(p, TOKEN_IDENTIFIER);
                expect(p, TOKEN_LBRACE);

                ASTNode *node = ast_new(p->arena, NODE_STRUCT_DEF, name->value, line);

                while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
                    ASTNode *member_type = parse_c_type(p);
                    Token *member_name = expect(p, TOKEN_IDENTIFIER);

                    // Check for array member: int data[100];
                    if (check(p, TOKEN_LBRACKET)) {
                        advance_parser(p);
                        Token *size = expect(p, TOKEN_NUMBER);
                        expect(p, TOKEN_RBRACKET);
                        ASTNode *arr_type = ast_new(p->arena, NODE_TYPE, "array", line);
                        ast_add_child(p->arena, arr_type,
                            ast_new(p->arena, NODE_NUMBER_LIT, size->value, line));
                        ast_add_child(p->arena, arr_type, member_type);
                        member_type = arr_type;
                    }

                    expect(p, TOKEN_SEMICOLON);

                    ASTNode *member = ast_new(p->arena, NODE_VAR_DECL, NULL, line);
                    ast_add_child(p->arena, member,
                        ast_new(p->arena, NODE_IDENTIFIER, member_name->value, line));
                    ast_add_child(p->arena, member, member_type);
                    ast_add_child(p->arena, node, member);
                }
                expect(p, TOKEN_RBRACE);
                expect(p, TOKEN_IDENTIFIER); // alias name (e.g. Point)
                expect(p, TOKEN_SEMICOLON);
                ast_add_child(p->arena, program, node);
                continue;
            } else {
                // Handle non-struct typedefs by skipping them
                while (!check(p, TOKEN_SEMICOLON) && !check(p, TOKEN_EOF)) advance_parser(p);
                if (check(p, TOKEN_SEMICOLON)) advance_parser(p);
                continue;
            }
        }

        // Everything else: must be a declaration (variable or function)
        if (is_declaration(p)) {
            ASTNode *decl = parse_c_declaration(p);
            ast_add_child(p->arena, program, decl);
            continue;
        }

        // Skip unknown tokens
        fprintf(stderr, "%s:%d: warning: skipping unexpected token %s\n",
                p->filename, current(p)->line, token_type_name(current(p)->type));
        advance_parser(p);
    }

    return program;
}