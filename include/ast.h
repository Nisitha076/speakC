#ifndef AST_H
#define AST_H


#include "util/arena.h"

typedef enum {
    NODE_PROGRAM,          // Root node: list of top-level items
    NODE_INCLUDE,          // #include directive
    NODE_MACRO_DEFINE,     // #define directive
    NODE_VAR_DECL,         // Variable declaration (declare x as int)
    NODE_VAR_ASSIGN,       // Assignment (set x to 5)
    NODE_VAR_INIT,         // Declaration + assignment (set x to 5 as int)
    NODE_FUNC_DEF,         // Function definition
    NODE_FUNC_DECL,        // Function forward declaration
    NODE_FUNC_CALL,        // Function call
    NODE_STRUCT_DEF,       // Struct definition
    NODE_ENUM_DEF,         // Enum definition (define X as enumeration)
    NODE_UNION_DEF,        // Union definition (define X as union)
    NODE_TYPEDEF,          // Type alias (alias MyInt to int)
    NODE_RETURN,           // Return statement
    NODE_IF,               // If/else if/else
    NODE_WHILE,            // While loop
    NODE_DO_WHILE,         // Do-while loop
    NODE_FOR,              // For loop
    NODE_SWITCH,           // Switch statement
    NODE_CASE,             // Case label
    NODE_DEFAULT,          // Default label in switch
    NODE_BREAK,            // Break statement
    NODE_CONTINUE,         // Continue statement
    NODE_GOTO,             // Goto statement (goto label)
    NODE_LABEL,            // Label for goto (label_name:)
    NODE_BLOCK,            // Indented block of statements
    NODE_BINARY_OP,        // Binary operation (a + b)
    NODE_UNARY_OP,         // Unary operation (!x, -x)
    NODE_TERNARY,          // Ternary expression (condition ? then : else)
    NODE_MEMBER_ACCESS,    // Struct member access (x of point)
    NODE_ARRAY_ACCESS,     // Array index (arr[i])
    NODE_ADDRESS_OF,       // address of x
    NODE_DEREF,            // value at p
    NODE_CAST,             // cast x to float
    NODE_SIZEOF,           // size of int
    NODE_IDENTIFIER,       // Variable/function name
    NODE_NUMBER_LIT,       // Numeric literal
    NODE_STRING_LIT,       // String literal
    NODE_CHAR_LIT,         // Char literal
    NODE_INCREMENT,        // increment i
    NODE_DECREMENT,        // decrement i
    NODE_COMPOUND_ASSIGN,  // add 2 to i, multiply i by 2
    NODE_TYPE,             // Type node (int, float, pointer to int, etc.)
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char *value;             // Optional: identifier name, literal value, operator
    struct ASTNode **children;
    int child_count;
    int child_capacity;
    int line;                // Source line for error reporting
} ASTNode;

// Create a new AST node from the arena
ASTNode *ast_new(Arena *a, NodeType type, const char *value, int line);

// Add a child node
void ast_add_child(Arena *a, ASTNode *parent, ASTNode *child);

// Print the AST as a tree (for debugging)
void ast_print(ASTNode *node, int depth);

// Helper: get a human-readable name for a node type
const char *node_type_name(NodeType type);

#endif