#include "../include/parser.h"
#include "../include/util/error.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for recursive descent expression parsing
static ASTNode *parse_expression(Parser *p);
static ASTNode *parse_or(Parser *p);
static ASTNode *parse_and(Parser *p);
static ASTNode *parse_equality(Parser *p);
static ASTNode *parse_comparison(Parser *p);
static ASTNode *parse_addition(Parser *p);
static ASTNode *parse_multiplication(Parser *p);
static ASTNode *parse_member_access(Parser *p);
static ASTNode *parse_primary(Parser *p);
static ASTNode *parse_function_call(Parser *p, ASTNode *callee);
static ASTNode *parse_array_access(Parser *p, ASTNode *array);

// Forward declarations for statement and block parsing
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_block(Parser *p);
static ASTNode *parse_set_statement(Parser *p);
static ASTNode *parse_declare_statement(Parser *p);
static ASTNode *parse_if_statement(Parser *p);
static ASTNode *parse_while_statement(Parser *p);
static ASTNode *parse_for_statement(Parser *p);
static ASTNode *parse_do_while_statement(Parser *p);
static ASTNode *parse_switch_statement(Parser *p);
static ASTNode *parse_return_statement(Parser *p);
static ASTNode *parse_break_statement(Parser *p);
static ASTNode *parse_inc_dec_statement(Parser *p);
static ASTNode *parse_compound_assign_statement(Parser *p);
static ASTNode *parse_expression_statement(Parser *p);
static ASTNode *parse_include(Parser *p);
static ASTNode *parse_define(Parser *p);
static ASTNode *parse_top_level(Parser *p);

// Initialize the parser with the token list and arena allocator
void parser_init(Parser *p, Vec *tokens, Arena *arena, const char *source, const char *filename) {
    p->tokens = tokens;
    p->pos = 0;
    p->arena = arena;
    p->source = source;
    p->filename = filename;
}

// Get the current token without advancing
static Token *current(Parser *p) {
    return (Token *)vec_get(p->tokens, p->pos);
}

// Advance and return the previous token
static Token *advance_token(Parser *p) {
    Token *tok = current(p);
    if (tok->type != TOKEN_EOF) {
        p->pos++;
    }
    return tok;
}

// Check if current token matches a type
static int check(Parser *p, TokenType type) {
    return current(p)->type == type;
}

// If a current token matches, advance and return 1. Otherwise return 0
static int match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance_token(p);
        return 1;
    }
    return 0;
}

// Expect a specific token type. If not found, report an error
static Token *expect(Parser *p, TokenType type) {
    if (check(p, type)) {
        return advance_token(p);
    }
    Token *tok = current(p);
    char msg[256];
    snprintf(msg, sizeof(msg), "expected %s, got %s (\"%s\")",
             token_type_name(type),
             token_type_name(tok->type),
             tok->value);
    report_error(p->filename, p->source, tok->line, tok->column, msg);
    exit(1);
}

// Skip newlines (they separate statements but aren't always meaningful)
static void skip_newlines(Parser *p) {
    while (check(p, TOKEN_NEWLINE)) {
        advance_token(p);
    }
}

// parse_type handles:
//   "int", "float", "char", etc.
//   "pointer to int"
//   "pointer to pointer to int"
//   "array of 10 int"
//   "constant int"
static ASTNode *parse_type(Parser *p) {
    int line = current(p)->line;

    // Handle "constant"
    if (match(p, TOKEN_CONSTANT)) {
        ASTNode *inner = parse_type(p);
        ASTNode *node = ast_new(p->arena, NODE_TYPE, "const", line);
        ast_add_child(p->arena, node, inner);
        return node;
    }

    // Handle "pointer to"
    if (match(p, TOKEN_POINTER)) {
        expect(p, TOKEN_TO);
        ASTNode *inner = parse_type(p); // recursive!
        ASTNode *node = ast_new(p->arena, NODE_TYPE, "pointer", line);
        ast_add_child(p->arena, node, inner);
        return node;
    }

    // Handle "array of N type"
    if (match(p, TOKEN_ARRAY)) {
        expect(p, TOKEN_OF);
        Token *size = expect(p, TOKEN_NUMBER);
        ASTNode *inner = parse_type(p);  // recursive!
        ASTNode *node = ast_new(p->arena, NODE_TYPE, "array", line);
        ASTNode *size_node = ast_new(p->arena, NODE_NUMBER_LIT, size->value, line);
        ast_add_child(p->arena, node, size_node);
        ast_add_child(p->arena, node, inner);
        return node;
    }

    // Base types: int, float, char, double, void, etc.
    if (check(p, TOKEN_INT) || check(p, TOKEN_FLOAT) ||
        check(p, TOKEN_CHAR) || check(p, TOKEN_DOUBLE) ||
        check(p, TOKEN_VOID) || check(p, TOKEN_LONG) ||
        check(p, TOKEN_SHORT) || check(p, TOKEN_UNSIGNED) ||
        check(p, TOKEN_SIGNED) || check(p, TOKEN_STRING_KW)) {
        Token *tok = advance_token(p);
        return ast_new(p->arena, NODE_TYPE, tok->value, line);
    }

    // Could also be a struct type name (identifier)
    if (check(p, TOKEN_IDENTIFIER)) {
        Token *tok = advance_token(p);
        return ast_new(p->arena, NODE_TYPE, tok->value, line);
    }

    Token *tok = current(p);
    report_error(p->filename, p->source, tok->line, tok->column, "expected a type");
    exit(1);
}

static ASTNode *parse_primary(Parser *p) {
    int line = current(p)->line;

    // Number literal
    if (check(p, TOKEN_NUMBER)) {
        Token *tok = advance_token(p);
        return ast_new(p->arena, NODE_NUMBER_LIT, tok->value, line);
    }

    // String literal
    if (check(p, TOKEN_STRING_LIT)) {
        Token *tok = advance_token(p);
        return ast_new(p->arena, NODE_STRING_LIT, tok->value, line);
    }

    // Char literal
    if (check(p, TOKEN_CHAR_LIT)) {
        Token *tok = advance_token(p);
        return ast_new(p->arena, NODE_CHAR_LIT, tok->value, line);
    }

    // "nothing" (NULL)
    if (match(p, TOKEN_NOTHING)) {
        return ast_new(p->arena, NODE_IDENTIFIER, "NULL", line);
    }

    // "address of x"
    if (match(p, TOKEN_ADDRESS)) {
        expect(p, TOKEN_OF);
        ASTNode *operand = parse_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_ADDRESS_OF, NULL, line);
        ast_add_child(p->arena, node, operand);
        return node;
    }

    // "Value at p"
    if (match(p, TOKEN_VALUE)) {
        expect(p, TOKEN_AT);
        ASTNode *operand = parse_primary(p);
        ASTNode *node = ast_new(p->arena, NODE_DEREF, NULL, line);
        ast_add_child(p->arena, node, operand);
        return node;
    }

    // "size of type"
    if (match(p, TOKEN_SIZE)) {
        expect(p, TOKEN_OF);
        ASTNode *type = parse_type(p);
        ASTNode *node = ast_new(p->arena, NODE_SIZEOF, NULL, line);
        ast_add_child(p->arena, node, type);
        return node;
    }

    // "cast x to float"
    if (match(p, TOKEN_CAST)) {
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_TO);
        ASTNode *type = parse_type(p);
        ASTNode *node = ast_new(p->arena, NODE_CAST, NULL, line);
        ast_add_child(p->arena, node, expr);
        ast_add_child(p->arena, node, type);
        return node;
    }

    // Identifier (variable name, function call, or array access)
    if (check(p, TOKEN_IDENTIFIER)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_new(p->arena, NODE_IDENTIFIER, tok->value, line);

        // Check for function call: identifier followed by (
        if (check(p, TOKEN_LPAREN)) {
            return parse_function_call(p, node);
        }

        // Check for array access: identifier followed by [
        if (check(p, TOKEN_LBRACKET)) {
            return parse_array_access(p, node);
        }

        return node;
    }

    // Parenthesized expression
    if (match(p, TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN);
        return expr;
    }

    Token *tok = current(p);
    char msg[256];
    snprintf(msg, sizeof(msg), "unexpected token %s (\"%s\")",
             token_type_name(tok->type), tok->value);
    report_error(p->filename, p->source, tok->line, tok->column, msg);
    exit(1);
}

// Parse function calls: callee(...)
static ASTNode *parse_function_call(Parser *p, ASTNode *callee) {
    int line = callee->line;
    ASTNode *call_node = ast_new(p->arena, NODE_FUNC_CALL, callee->value, line);
    expect(p, TOKEN_LPAREN);
    if (!check(p, TOKEN_RPAREN)) {
        do {
            ast_add_child(p->arena, call_node, parse_expression(p));
        } while (match(p, TOKEN_COMMA));
    }
    expect(p, TOKEN_RPAREN);
    return call_node;
}

// Parse array indexing access: array[index]
static ASTNode *parse_array_access(Parser *p, ASTNode *array) {
    int line = array->line;
    ASTNode *access_node = ast_new(p->arena, NODE_ARRAY_ACCESS, NULL, line);
    expect(p, TOKEN_LBRACKET);
    ASTNode *index = parse_expression(p);
    expect(p, TOKEN_RBRACKET);
    ast_add_child(p->arena, access_node, array);
    ast_add_child(p->arena, access_node, index);
    return access_node;
}

// Expression grammar rules (lowest to highest precedence):

static ASTNode *parse_expression(Parser *p) {
    return parse_or(p);
}

static ASTNode *parse_or(Parser *p) {
    ASTNode *left = parse_and(p);
    while (match(p, TOKEN_OR)) {
        ASTNode *right = parse_and(p);
        ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "||", left->line);
        ast_add_child(p->arena, node, left);
        ast_add_child(p->arena, node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_and(Parser *p) {
    ASTNode *left = parse_equality(p);
    while (match(p, TOKEN_AND)) {
        ASTNode *right = parse_equality(p);
        ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "&&", left->line);
        ast_add_child(p->arena, node, left);
        ast_add_child(p->arena, node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_equality(Parser *p) {
    ASTNode *left = parse_comparison(p);
    while (1) {
        if (match(p, TOKEN_EQUAL)) {
            ASTNode *right = parse_comparison(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "==", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_NOT_EQUAL)) {
            ASTNode *right = parse_comparison(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "!=", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else {
            break;
        }
    }
    return left;
}

static ASTNode *parse_comparison(Parser *p) {
    ASTNode *left = parse_addition(p);
    while (1) {
        if (match(p, TOKEN_GREATER)) {
            ASTNode *right = parse_addition(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, ">", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_LESS)) {
            ASTNode *right = parse_addition(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "<", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_GREATER_EQUAL)) {
            ASTNode *right = parse_addition(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, ">=", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_LESS_EQUAL)) {
            ASTNode *right = parse_addition(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "<=", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else {
            break;
        }
    }
    return left;
}

static ASTNode *parse_addition(Parser *p) {
    ASTNode *left = parse_multiplication(p);
    while (1) {
        if (match(p, TOKEN_PLUS)) {
            ASTNode *right = parse_multiplication(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "+", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_MINUS)) {
            ASTNode *right = parse_multiplication(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "-", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else {
            break;
        }
    }
    return left;
}

static ASTNode *parse_multiplication(Parser *p) {
    ASTNode *left = parse_member_access(p);
    while (1) {
        if (match(p, TOKEN_STAR)) {
            ASTNode *right = parse_member_access(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "*", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_SLASH)) {
            ASTNode *right = parse_member_access(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "/", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else if (match(p, TOKEN_PERCENT)) {
            ASTNode *right = parse_member_access(p);
            ASTNode *node = ast_new(p->arena, NODE_BINARY_OP, "%", left->line);
            ast_add_child(p->arena, node, left);
            ast_add_child(p->arena, node, right);
            left = node;
        } else {
            break;
        }
    }
    return left;
}

static ASTNode *parse_member_access(Parser *p) {
    ASTNode *left = parse_primary(p);

    // "x of y" — but remember, "of" is RIGHT-associative
    // So "x of y of z" means x of (y of z)
    if (check(p, TOKEN_OF)) {
        advance_token(p); // consume "of"
        ASTNode *right = parse_member_access(p); // right-recursive!
        ASTNode *node = ast_new(p->arena, NODE_MEMBER_ACCESS, NULL, left->line);
        ast_add_child(p->arena, node, right); // object first
        ast_add_child(p->arena, node, left);  // member second
        return node;
    }

    return left;
}

// Statement and Block Parsing implementation:

static ASTNode *parse_statement(Parser *p) {
    skip_newlines(p);

    // "set x to ..." — assignment or declaration
    if (check(p, TOKEN_SET))
        return parse_set_statement(p);

    // "declare x as ..." — uninitialized declaration
    if (check(p, TOKEN_DECLARE))
        return parse_declare_statement(p);

    // "if ..." — conditional
    if (check(p, TOKEN_IF))
        return parse_if_statement(p);

    // "while ..." — while loop
    if (check(p, TOKEN_WHILE))
        return parse_while_statement(p);

    // "for ..." — for loop
    if (check(p, TOKEN_FOR))
        return parse_for_statement(p);

    // "do:" — do-while loop
    if (check(p, TOKEN_DO))
        return parse_do_while_statement(p);

    // "switch on ..." — switch
    if (check(p, TOKEN_SWITCH))
        return parse_switch_statement(p);

    // "return ..." — return statement
    if (check(p, TOKEN_RETURN))
        return parse_return_statement(p);

    // "break" — break statement
    if (check(p, TOKEN_BREAK))
        return parse_break_statement(p);

    // "increment x" or "decrement x"
    if (check(p, TOKEN_INCREMENT) || check(p, TOKEN_DECREMENT))
        return parse_inc_dec_statement(p);

    // "add N to x", "subtract N from x", etc.
    if (check(p, TOKEN_ADD) || check(p, TOKEN_SUBTRACT) ||
        check(p, TOKEN_MULTIPLY) || check(p, TOKEN_DIVIDE))
        return parse_compound_assign_statement(p);

    // Otherwise, try to parse as an expression statement (e.g., function call)
    return parse_expression_statement(p);
}

static ASTNode *parse_block(Parser *p) {
    int line = current(p)->line;
    skip_newlines(p);
    expect(p, TOKEN_INDENT);

    ASTNode *block = ast_new(p->arena, NODE_BLOCK, NULL, line);

    while (!check(p, TOKEN_DEDENT) && !check(p, TOKEN_EOF)) {
        ASTNode *stmt = parse_statement(p);
        ast_add_child(p->arena, block, stmt);
        skip_newlines(p);
    }

    expect(p, TOKEN_DEDENT);
    return block;
}

static ASTNode *parse_set_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_SET);

    ASTNode *target = parse_expression(p);
    expect(p, TOKEN_TO);
    ASTNode *value = parse_expression(p);

    // Check for "as type" (declaration with initialization)
    if (match(p, TOKEN_AS)) {
        ASTNode *type = parse_type(p);
        ASTNode *node = ast_new(p->arena, NODE_VAR_INIT, NULL, line);
        ast_add_child(p->arena, node, target);   // child 0: name
        ast_add_child(p->arena, node, value);    // child 1: value
        ast_add_child(p->arena, node, type);     // child 2: type
        return node;
    }

    // No "as" — it's a re-assignment
    ASTNode *node = ast_new(p->arena, NODE_VAR_ASSIGN, NULL, line);
    ast_add_child(p->arena, node, target);
    ast_add_child(p->arena, node, value);
    return node;
}

static ASTNode *parse_declare_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_DECLARE);
    ASTNode *target = parse_expression(p);
    expect(p, TOKEN_AS);
    ASTNode *type = parse_type(p);
    ASTNode *node = ast_new(p->arena, NODE_VAR_DECL, NULL, line);
    ast_add_child(p->arena, node, target);
    ast_add_child(p->arena, node, type);
    return node;
}

static ASTNode *parse_if_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_IF);
    ASTNode *condition = parse_expression(p);
    expect(p, TOKEN_COLON);

    ASTNode *node = ast_new(p->arena, NODE_IF, NULL, line);
    ast_add_child(p->arena, node, condition);       // child 0: condition
    ast_add_child(p->arena, node, parse_block(p));  // child 1: body

    skip_newlines(p);

    // Handle "else if" and "else"
    while (check(p, TOKEN_ELSE)) {
        advance_token(p);
        if (check(p, TOKEN_IF)) {
            // else if
            advance_token(p);
            ASTNode *elif_cond = parse_expression(p);
            expect(p, TOKEN_COLON);
            ast_add_child(p->arena, node, elif_cond);
            ast_add_child(p->arena, node, parse_block(p));
            skip_newlines(p);
        } else {
            // else
            expect(p, TOKEN_COLON);
            ast_add_child(p->arena, node, parse_block(p));
            break;
        }
    }

    return node;
}

static ASTNode *parse_while_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_WHILE);
    ASTNode *condition = parse_expression(p);
    expect(p, TOKEN_COLON);
    ASTNode *body = parse_block(p);
    ASTNode *node = ast_new(p->arena, NODE_WHILE, NULL, line);
    ast_add_child(p->arena, node, condition);
    ast_add_child(p->arena, node, body);
    return node;
}

static ASTNode *parse_for_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_FOR);
    
    ASTNode *init = parse_statement(p);
    expect(p, TOKEN_COMMA);
    ASTNode *cond = parse_expression(p);
    expect(p, TOKEN_COMMA);
    ASTNode *step = parse_statement(p);
    expect(p, TOKEN_COLON);
    ASTNode *body = parse_block(p);

    ASTNode *node = ast_new(p->arena, NODE_FOR, NULL, line);
    ast_add_child(p->arena, node, init);
    ast_add_child(p->arena, node, cond);
    ast_add_child(p->arena, node, step);
    ast_add_child(p->arena, node, body);
    return node;
}

static ASTNode *parse_do_while_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_DO);
    expect(p, TOKEN_COLON);
    ASTNode *body = parse_block(p);
    skip_newlines(p);
    expect(p, TOKEN_WHILE);
    ASTNode *condition = parse_expression(p);
    
    ASTNode *node = ast_new(p->arena, NODE_DO_WHILE, NULL, line);
    ast_add_child(p->arena, node, body);
    ast_add_child(p->arena, node, condition);
    return node;
}

static ASTNode *parse_switch_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_SWITCH);
    expect(p, TOKEN_ON);
    ASTNode *expr = parse_expression(p);
    expect(p, TOKEN_COLON);
    skip_newlines(p);
    expect(p, TOKEN_INDENT);
    ASTNode *switch_node = ast_new(p->arena, NODE_SWITCH, NULL, line);
    ast_add_child(p->arena, switch_node, expr);
    
    ASTNode *cases_block = ast_new(p->arena, NODE_BLOCK, NULL, line);
    while (!check(p, TOKEN_DEDENT) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_CASE)) {
            int case_line = current(p)->line;
            advance_token(p); // consume case
            ASTNode *val = parse_expression(p);
            expect(p, TOKEN_COLON);
            ASTNode *case_node = ast_new(p->arena, NODE_CASE, NULL, case_line);
            ast_add_child(p->arena, case_node, val);
            ast_add_child(p->arena, cases_block, case_node);
        } else if (check(p, TOKEN_DEFAULT)) {
            int def_line = current(p)->line;
            advance_token(p); // consume default
            expect(p, TOKEN_COLON);
            ASTNode *def_node = ast_new(p->arena, NODE_DEFAULT, NULL, def_line);
            ast_add_child(p->arena, cases_block, def_node);
        } else {
            ASTNode *stmt = parse_statement(p);
            ast_add_child(p->arena, cases_block, stmt);
        }
        skip_newlines(p);
    }
    expect(p, TOKEN_DEDENT);
    ast_add_child(p->arena, switch_node, cases_block);
    return switch_node;
}

static ASTNode *parse_return_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_RETURN);
    ASTNode *expr = NULL;
    if (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_EOF) && !check(p, TOKEN_DEDENT)) {
        expr = parse_expression(p);
    }
    ASTNode *node = ast_new(p->arena, NODE_RETURN, NULL, line);
    if (expr) {
        ast_add_child(p->arena, node, expr);
    }
    return node;
}

static ASTNode *parse_break_statement(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_BREAK);
    return ast_new(p->arena, NODE_BREAK, NULL, line);
}

static ASTNode *parse_inc_dec_statement(Parser *p) {
    int line = current(p)->line;
    NodeType type;
    if (match(p, TOKEN_INCREMENT)) {
        type = NODE_INCREMENT;
    } else {
        expect(p, TOKEN_DECREMENT);
        type = NODE_DECREMENT;
    }
    ASTNode *target = parse_expression(p);
    ASTNode *node = ast_new(p->arena, type, NULL, line);
    ast_add_child(p->arena, node, target);
    return node;
}

static ASTNode *parse_compound_assign_statement(Parser *p) {
    int line = current(p)->line;
    ASTNode *node = ast_new(p->arena, NODE_COMPOUND_ASSIGN, NULL, line);
    
    if (match(p, TOKEN_ADD)) {
        node->value = "+=";
        ASTNode *val = parse_expression(p);
        expect(p, TOKEN_TO);
        ASTNode *target = parse_expression(p);
        ast_add_child(p->arena, node, target); 
        ast_add_child(p->arena, node, val);    
    } else if (match(p, TOKEN_SUBTRACT)) {
        node->value = "-=";
        ASTNode *val = parse_expression(p);
        expect(p, TOKEN_FROM);
        ASTNode *target = parse_expression(p);
        ast_add_child(p->arena, node, target);
        ast_add_child(p->arena, node, val);
    } else if (match(p, TOKEN_MULTIPLY)) {
        node->value = "*=";
        ASTNode *target = parse_expression(p);
        expect(p, TOKEN_BY);
        ASTNode *val = parse_expression(p);
        ast_add_child(p->arena, node, target);
        ast_add_child(p->arena, node, val);
    } else if (match(p, TOKEN_DIVIDE)) {
        node->value = "/=";
        ASTNode *target = parse_expression(p);
        expect(p, TOKEN_BY);
        ASTNode *val = parse_expression(p);
        ast_add_child(p->arena, node, target);
        ast_add_child(p->arena, node, val);
    } else {
        Token *tok = current(p);
        report_error(p->filename, p->source, tok->line, tok->column, "expected compound assignment");
        exit(1);
    }
    return node;
}

static ASTNode *parse_expression_statement(Parser *p) {
    return parse_expression(p);
}


// Top-Level Declarations implementation:

static ASTNode *parse_include(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_INCLUDE);
    
    char value_buf[256] = "";
    if (match(p, TOKEN_LESS)) {
        Token *name = expect(p, TOKEN_IDENTIFIER);
        expect(p, TOKEN_GREATER);
        snprintf(value_buf, sizeof(value_buf), "<%s>", name->value);
    } else if (check(p, TOKEN_STRING_LIT)) {
        Token *name = advance_token(p);
        snprintf(value_buf, sizeof(value_buf), "\"%s\"", name->value);
    } else {
        Token *name = expect(p, TOKEN_IDENTIFIER);
        snprintf(value_buf, sizeof(value_buf), "%s", name->value);
    }
    
    return ast_new(p->arena, NODE_INCLUDE, value_buf, line);
}

static ASTNode *parse_define(Parser *p) {
    int line = current(p)->line;
    expect(p, TOKEN_DEFINE);
    
    if (match(p, TOKEN_FUNCTION)) {
        Token *name = expect(p, TOKEN_IDENTIFIER);
        ASTNode *func_node = ast_new(p->arena, NODE_FUNC_DEF, name->value, line);
        
        ASTNode *params[128];
        int param_count = 0;
        if (match(p, TOKEN_LPAREN)) {
            if (!check(p, TOKEN_RPAREN)) {
                do {
                    Token *pname = expect(p, TOKEN_IDENTIFIER);
                    expect(p, TOKEN_AS);
                    ASTNode *ptype = parse_type(p);
                    
                    ASTNode *pname_node = ast_new(p->arena, NODE_IDENTIFIER, pname->value, pname->line);
                    params[param_count++] = pname_node;
                    params[param_count++] = ptype;
                } while (match(p, TOKEN_COMMA));
            }
            expect(p, TOKEN_RPAREN);
        }
        
        ASTNode *ret_type = NULL;
        if (match(p, TOKEN_RETURNS)) {
            if (!match(p, TOKEN_NOTHING)) {
                ret_type = parse_type(p);
            }
        }
        
        expect(p, TOKEN_COLON);
        ASTNode *body = parse_block(p);
        
        ast_add_child(p->arena, func_node, ret_type);
        for (int i = 0; i < param_count; i++) {
            ast_add_child(p->arena, func_node, params[i]);
        }
        ast_add_child(p->arena, func_node, body);
        return func_node;
    }
    
    Token *name = expect(p, TOKEN_IDENTIFIER);
    expect(p, TOKEN_AS);
    
    if (match(p, TOKEN_STRUCTURE)) {
        expect(p, TOKEN_COLON);
        ASTNode *struct_node = ast_new(p->arena, NODE_STRUCT_DEF, name->value, line);
        
        skip_newlines(p);
        expect(p, TOKEN_INDENT);
        while (!check(p, TOKEN_DEDENT) && !check(p, TOKEN_EOF)) {
            skip_newlines(p);
            if (check(p, TOKEN_DECLARE)) {
                ASTNode *member = parse_statement(p); // returns NODE_VAR_DECL
                ast_add_child(p->arena, struct_node, member);
            } else if (!check(p, TOKEN_DEDENT)) {
                advance_token(p);
            }
            skip_newlines(p);
        }
        expect(p, TOKEN_DEDENT);
        return struct_node;
    }
    
    // Macro define: define NAME as VALUE
    ASTNode *val = parse_expression(p);
    ASTNode *macro_node = ast_new(p->arena, NODE_MACRO_DEFINE, name->value, line);
    ast_add_child(p->arena, macro_node, val);
    return macro_node;
}

static ASTNode *parse_top_level(Parser *p) {
    skip_newlines(p);

    if (check(p, TOKEN_INCLUDE))
        return parse_include(p);

    if (check(p, TOKEN_DEFINE))
        return parse_define(p);  

    if (check(p, TOKEN_DECLARE))
        return parse_declare_statement(p);  

    Token *tok = current(p);
    char msg[256];
    snprintf(msg, sizeof(msg), "unexpected top-level token %s (\"%s\")",
             token_type_name(tok->type), tok->value);
    report_error(p->filename, p->source, tok->line, tok->column, msg);
    exit(1);
}

ASTNode *parser_parse(Parser *p) {
    ASTNode *program = ast_new(p->arena, NODE_PROGRAM, NULL, 1);

    skip_newlines(p);
    while (!check(p, TOKEN_EOF)) {
        ASTNode *item = parse_top_level(p);
        ast_add_child(p->arena, program, item);
        skip_newlines(p);
    }

    return program;
}