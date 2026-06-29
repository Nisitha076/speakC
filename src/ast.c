#include "../include/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to duplicate a string in the arena
static char *arena_strdup(Arena *a, const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *dst = (char *)arena_alloc(a, len + 1);
    if (dst) {
        memcpy(dst, src, len + 1);
    }
    return dst;
}

ASTNode *ast_new(Arena *a, NodeType type, const char *value, int line) {
    ASTNode *node = (ASTNode *)arena_alloc(a, sizeof(ASTNode));
    if (!node) return NULL;

    node->type = type;
    node->value = arena_strdup(a, value);
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->line = line;

    return node;
}

void ast_add_child(Arena *a, ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;

    if (parent->child_count >= parent->child_capacity) {
        int new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        ASTNode **new_children = (ASTNode **)arena_alloc(a, sizeof(ASTNode *) * new_capacity);
        if (parent->children) {
            for (int i = 0; i < parent->child_count; i++) {
                new_children[i] = parent->children[i];
            }
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }

    parent->children[parent->child_count++] = child;
}

void ast_print(ASTNode *node, int depth) {
    if (!node) return;

    // Indent based on depth level
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    printf("%s", node_type_name(node->type));
    if (node->value) {
        printf(": %s", node->value);
    }
    printf("\n");

    for (int i = 0; i < node->child_count; i++) {
        ast_print(node->children[i], depth + 1);
    }
}

const char *node_type_name(NodeType type) {
    switch (type) {
        case NODE_PROGRAM:          return "NODE_PROGRAM";
        case NODE_INCLUDE:          return "NODE_INCLUDE";
        case NODE_MACRO_DEFINE:     return "NODE_MACRO_DEFINE";
        case NODE_VAR_DECL:         return "NODE_VAR_DECL";
        case NODE_VAR_ASSIGN:       return "NODE_VAR_ASSIGN";
        case NODE_VAR_INIT:         return "NODE_VAR_INIT";
        case NODE_FUNC_DEF:         return "NODE_FUNC_DEF";
        case NODE_FUNC_DECL:        return "NODE_FUNC_DECL";
        case NODE_FUNC_CALL:        return "NODE_FUNC_CALL";
        case NODE_STRUCT_DEF:       return "NODE_STRUCT_DEF";
        case NODE_ENUM_DEF:         return "NODE_ENUM_DEF";
        case NODE_UNION_DEF:        return "NODE_UNION_DEF";
        case NODE_TYPEDEF:          return "NODE_TYPEDEF";
        case NODE_RETURN:           return "NODE_RETURN";
        case NODE_IF:               return "NODE_IF";
        case NODE_WHILE:            return "NODE_WHILE";
        case NODE_DO_WHILE:         return "NODE_DO_WHILE";
        case NODE_FOR:              return "NODE_FOR";
        case NODE_SWITCH:           return "NODE_SWITCH";
        case NODE_CASE:             return "NODE_CASE";
        case NODE_DEFAULT:          return "NODE_DEFAULT";
        case NODE_BREAK:            return "NODE_BREAK";
        case NODE_CONTINUE:         return "NODE_CONTINUE";
        case NODE_GOTO:             return "NODE_GOTO";
        case NODE_LABEL:            return "NODE_LABEL";
        case NODE_BLOCK:            return "NODE_BLOCK";
        case NODE_BINARY_OP:        return "NODE_BINARY_OP";
        case NODE_UNARY_OP:         return "NODE_UNARY_OP";
        case NODE_TERNARY:          return "NODE_TERNARY";
        case NODE_MEMBER_ACCESS:    return "NODE_MEMBER_ACCESS";
        case NODE_ARRAY_ACCESS:     return "NODE_ARRAY_ACCESS";
        case NODE_ADDRESS_OF:       return "NODE_ADDRESS_OF";
        case NODE_DEREF:            return "NODE_DEREF";
        case NODE_CAST:             return "NODE_CAST";
        case NODE_SIZEOF:           return "NODE_SIZEOF";
        case NODE_IDENTIFIER:       return "NODE_IDENTIFIER";
        case NODE_NUMBER_LIT:       return "NODE_NUMBER_LIT";
        case NODE_STRING_LIT:       return "NODE_STRING_LIT";
        case NODE_CHAR_LIT:         return "NODE_CHAR_LIT";
        case NODE_INCREMENT:        return "NODE_INCREMENT";
        case NODE_DECREMENT:        return "NODE_DECREMENT";
        case NODE_COMPOUND_ASSIGN:  return "NODE_COMPOUND_ASSIGN";
        case NODE_TYPE:             return "NODE_TYPE";
        default:                    return "NODE_UNKNOWN";
    }
}
