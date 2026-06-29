#include "../include/emitter.h"
#include <string.h>

// Initialize the emitter
void emitter_init(Emitter *e) {
    strbuf_init(&e->output);
    e->indent_level = 0;
    vec_init(&e->includes);
}

// Free the emitter resources
void emitter_free(Emitter *e) {
    strbuf_free(&e->output);
    vec_free(&e->includes);
}

// Convert output string buffer to a C string (caller must free it)
char *emitter_get_output(Emitter *e) {
    return strbuf_to_string(&e->output);
}

// Step 2: Implement Indentation Helpers

// Append indentation spaces based on the current level
static void emit_indent(Emitter *e) {
    for (int i = 0; i < e->indent_level; i++) {
        strbuf_append(&e->output, "    ");  // 4 spaces per level
    }
}


// Forward declarations for emitter static functions
static void emit_type(Emitter *e, ASTNode *node);
static void emit_array_suffix(Emitter *e, ASTNode *type_node);
static void emit_expression(Emitter *e, ASTNode *node);
static void emit_statement(Emitter *e, ASTNode *node);
static void emit_block(Emitter *e, ASTNode *block);
static void emit_var_init(Emitter *e, ASTNode *node);
static void emit_var_assign(Emitter *e, ASTNode *node);
static void emit_var_decl(Emitter *e, ASTNode *node);
static void emit_if(Emitter *e, ASTNode *node);
static void emit_while(Emitter *e, ASTNode *node);
static void emit_for(Emitter *e, ASTNode *node);
static void emit_do_while(Emitter *e, ASTNode *node);
static void emit_switch(Emitter *e, ASTNode *node);
static void emit_return(Emitter *e, ASTNode *node);
static void emit_break(Emitter *e, ASTNode *node);
static void emit_continue(Emitter *e, ASTNode *node);
static void emit_increment(Emitter *e, ASTNode *node);
static void emit_decrement(Emitter *e, ASTNode *node);
static void emit_compound_assign(Emitter *e, ASTNode *node);
static void emit_function_def(Emitter *e, ASTNode *node);
static void emit_struct_def(Emitter *e, ASTNode *node);
static void emit_include(Emitter *e, ASTNode *node);
static void emit_macro(Emitter *e, ASTNode *node);

static void emit_type(Emitter *e, ASTNode *node) {
    if (node->type != NODE_TYPE) return;

    const char *val = node->value;

    if (strcmp(val, "pointer") == 0) {
        // "pointer to int" → "int *"
        emit_type(e, node->children[0]);  // emit the inner type first
        strbuf_append(&e->output, " *");
    } else if (strcmp(val, "array") == 0) {
        // "array of 10 int" → "int [10]"
        emit_type(e, node->children[1]);  // the base type
    } else if (strcmp(val, "const") == 0) {
        strbuf_append(&e->output, "const ");
        emit_type(e, node->children[0]);
    } else if (strcmp(val, "string") == 0) {
        strbuf_append(&e->output, "char");
    } else {
        // Base type: int, float, char, etc.
        strbuf_append(&e->output, val);
    }
}

static void emit_array_suffix(Emitter *e, ASTNode *type_node) {
    if (type_node->type == NODE_TYPE && strcmp(type_node->value, "array") == 0) {
        strbuf_append_fmt(&e->output, "[%s]", type_node->children[0]->value);
    } else if (type_node->type == NODE_TYPE && strcmp(type_node->value, "string") == 0) {
        strbuf_append(&e->output, "[]");
    }
}

static void emit_expression(Emitter *e, ASTNode *node) {
    switch (node->type) {
        case NODE_NUMBER_LIT:
        case NODE_IDENTIFIER:
            strbuf_append(&e->output, node->value);
            break;
        
        case NODE_STRING_LIT:
            strbuf_append_fmt(&e->output, "\"%s\"", node->value);
            break;
        
        case NODE_CHAR_LIT:
            strbuf_append_fmt(&e->output, "'%s'", node->value);
            break;
        
        case NODE_BINARY_OP:
            emit_expression(e, node->children[0]);
            strbuf_append_fmt(&e->output, " %s ", node->value);
            emit_expression(e, node->children[1]);
            break;

        case NODE_MEMBER_ACCESS:
            if (node->children[0]->type == NODE_DEREF) {
                emit_expression(e, node->children[0]->children[0]);
                strbuf_append(&e->output, "->");
            } else {
                emit_expression(e, node->children[0]);
                strbuf_append(&e->output, ".");
            }
            strbuf_append(&e->output, node->children[1]->value);
            break;


        case NODE_ADDRESS_OF:
            strbuf_append(&e->output, "&");
            emit_expression(e, node->children[0]);
            break;
        
        case NODE_DEREF:
            strbuf_append(&e->output, "*");
            emit_expression(e, node->children[0]);
            break;
        
        case NODE_SIZEOF:
            strbuf_append(&e->output, "sizeof(");
            emit_type(e, node->children[0]);
            emit_array_suffix(e, node->children[0]);
            strbuf_append(&e->output, ")");
            break;

        case NODE_CAST:
            strbuf_append(&e->output, "(");
            emit_type(e, node->children[1]); // The target type
            emit_array_suffix(e, node->children[1]);
            strbuf_append(&e->output, ")");
            emit_expression(e, node->children[0]); // The value
            break;


        case NODE_FUNC_CALL:
            strbuf_append(&e->output, node->value);
            strbuf_append(&e->output, "(");
            for (int i = 0; i < node->child_count; i++) {
                if (i > 0) strbuf_append(&e->output, ", ");
                emit_expression(e, node->children[i]);
            }
            strbuf_append(&e->output, ")");
            break;

        case NODE_ARRAY_ACCESS:
            emit_expression(e, node->children[0]);
            strbuf_append(&e->output, "[");
            emit_expression(e, node->children[1]);
            strbuf_append(&e->output, "]");
            break;

        default:
            strbuf_append(&e->output, "/* unknown expr */");
            break;
    }
}

// "set x to 5 as int" → "int x = 5;"
static void emit_var_init(Emitter *e, ASTNode *node) {
    emit_indent(e);
    ASTNode *name = node->children[0];
    ASTNode *value = node->children[1];
    ASTNode *type = node->children[2];

    emit_type(e, type);
    strbuf_append_fmt(&e->output, " %s", name->value);
    emit_array_suffix(e, type);
    strbuf_append(&e->output, " = ");
    emit_expression(e, value);
    strbuf_append(&e->output, ";\n");
}

// "set x to 10" → "x = 10;"
static void emit_var_assign(Emitter *e, ASTNode *node) {
    emit_indent(e);
    emit_expression(e, node->children[0]);  // target
    strbuf_append(&e->output, " = ");
    emit_expression(e, node->children[1]);  // value
    strbuf_append(&e->output, ";\n");
}

// "declare x as int" → "int x;"
static void emit_var_decl(Emitter *e, ASTNode *node) {
    emit_indent(e);
    ASTNode *name = node->children[0];
    ASTNode *type = node->children[1];
    emit_type(e, type);
    strbuf_append_fmt(&e->output, " %s", name->value);
    emit_array_suffix(e, type);
    strbuf_append(&e->output, ";\n");
}

static void emit_if(Emitter *e, ASTNode *node) {
    // Children are in pairs: condition, body, condition, body, ... [optional else body]
    emit_indent(e);
    strbuf_append(&e->output, "if (");
    emit_expression(e, node->children[0]);  // condition
    strbuf_append(&e->output, ") {\n");

    e->indent_level++;
    emit_block(e, node->children[1]);  // body
    e->indent_level--;

    emit_indent(e);
    strbuf_append(&e->output, "}");

    // Handle else-if and else clauses
    int i = 2;
    while (i < node->child_count) {
        if (i + 1 < node->child_count && node->children[i]->type != NODE_BLOCK) {
            // else if
            strbuf_append(&e->output, " else if (");
            emit_expression(e, node->children[i]);
            strbuf_append(&e->output, ") {\n");
            e->indent_level++;
            emit_block(e, node->children[i + 1]);
            e->indent_level--;
            emit_indent(e);
            strbuf_append(&e->output, "}");
            i += 2;
        } else {
            // else
            strbuf_append(&e->output, " else {\n");
            e->indent_level++;
            emit_block(e, node->children[i]);
            e->indent_level--;
            emit_indent(e);
            strbuf_append(&e->output, "}");
            i++;
        }
    }
    strbuf_append(&e->output, "\n");
}

static void emit_while(Emitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "while (");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, ") {\n");
    
    e->indent_level++;
    emit_block(e, node->children[1]);
    e->indent_level--;
    
    emit_indent(e);
    strbuf_append(&e->output, "}\n");
}

// Helpers to emit inline initializations and steps for the C "for" syntax
static void emit_for_init(Emitter *e, ASTNode *node) {
    if (node->type == NODE_VAR_INIT) {
        ASTNode *name = node->children[0];
        ASTNode *value = node->children[1];
        ASTNode *type = node->children[2];
        emit_type(e, type);
        strbuf_append_fmt(&e->output, " %s", name->value);
        emit_array_suffix(e, type);
        strbuf_append(&e->output, " = ");
        emit_expression(e, value);
    } else if (node->type == NODE_VAR_ASSIGN) {
        emit_expression(e, node->children[0]);
        strbuf_append(&e->output, " = ");
        emit_expression(e, node->children[1]);
    } else if (node->type == NODE_VAR_DECL) {
        ASTNode *name = node->children[0];
        ASTNode *type = node->children[1];
        emit_type(e, type);
        strbuf_append_fmt(&e->output, " %s", name->value);
        emit_array_suffix(e, type);
    } else {
        emit_expression(e, node);
    }
}

static void emit_for_step(Emitter *e, ASTNode *node) {
    if (node->type == NODE_INCREMENT) {
        emit_expression(e, node->children[0]);
        strbuf_append(&e->output, "++");
    } else if (node->type == NODE_DECREMENT) {
        emit_expression(e, node->children[0]);
        strbuf_append(&e->output, "--");
    } else if (node->type == NODE_COMPOUND_ASSIGN) {
        emit_expression(e, node->children[0]);
        strbuf_append_fmt(&e->output, " %s ", node->value);
        emit_expression(e, node->children[1]);
    } else if (node->type == NODE_VAR_ASSIGN) {
        emit_expression(e, node->children[0]);
        strbuf_append(&e->output, " = ");
        emit_expression(e, node->children[1]);
    } else {
        emit_expression(e, node);
    }
}

static void emit_for(Emitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "for (");
    emit_for_init(e, node->children[0]);
    strbuf_append(&e->output, "; ");
    emit_expression(e, node->children[1]);
    strbuf_append(&e->output, "; ");
    emit_for_step(e, node->children[2]);
    strbuf_append(&e->output, ") {\n");
    
    e->indent_level++;
    emit_block(e, node->children[3]);
    e->indent_level--;
    
    emit_indent(e);
    strbuf_append(&e->output, "}\n");
}

static void emit_do_while(Emitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "do {\n");
    
    e->indent_level++;
    emit_block(e, node->children[0]);
    e->indent_level--;
    
    emit_indent(e);
    strbuf_append(&e->output, "} while (");
    emit_expression(e, node->children[1]);
    strbuf_append(&e->output, ");\n");
}

static void emit_switch(Emitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "switch (");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, ") {\n");
    
    e->indent_level++;
    ASTNode *block = node->children[1];
    for (int i = 0; i < block->child_count; i++) {
        ASTNode *item = block->children[i];
        if (item->type == NODE_CASE) {
            emit_indent(e);
            strbuf_append(&e->output, "case ");
            emit_expression(e, item->children[0]);
            strbuf_append(&e->output, ":\n");
        } else if (item->type == NODE_DEFAULT) {
            emit_indent(e);
            strbuf_append(&e->output, "default:\n");
        } else {
            e->indent_level++;
            emit_statement(e, item);
            e->indent_level--;
        }
    }
    e->indent_level--;
    
    emit_indent(e);
    strbuf_append(&e->output, "}\n");
}

static void emit_return(Emitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "return");
    if (node->child_count > 0) {
        strbuf_append(&e->output, " ");
        emit_expression(e, node->children[0]);
    }
    strbuf_append(&e->output, ";\n");
}

static void emit_break(Emitter *e, ASTNode *node) {
    (void)node;
    emit_indent(e);
    strbuf_append(&e->output, "break;\n");
}

static void emit_continue(Emitter *e, ASTNode *node) {
    (void)node;
    emit_indent(e);
    strbuf_append(&e->output, "continue;\n");
}

static void emit_increment(Emitter *e, ASTNode *node) {
    emit_indent(e);
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, "++;\n");
}

static void emit_decrement(Emitter *e, ASTNode *node) {
    emit_indent(e);
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, "--;\n");
}

static void emit_compound_assign(Emitter *e, ASTNode *node) {
    emit_indent(e);
    emit_expression(e, node->children[0]);
    strbuf_append_fmt(&e->output, " %s ", node->value);
    emit_expression(e, node->children[1]);
    strbuf_append(&e->output, ";\n");
}

static void emit_function_def(Emitter *e, ASTNode *node) {
    ASTNode *ret_type = node->children[0];
    if (ret_type) {
        emit_type(e, ret_type);
    } else {
        strbuf_append(&e->output, "void");
    }

    strbuf_append_fmt(&e->output, " %s(", node->value);

    int param_limit = node->child_count - 1;
    int param_idx = 0;
    for (int i = 1; i < param_limit; i += 2) {
        if (param_idx > 0) {
            strbuf_append(&e->output, ", ");
        }
        ASTNode *pname = node->children[i];
        ASTNode *ptype = node->children[i + 1];
        emit_type(e, ptype);
        strbuf_append_fmt(&e->output, " %s", pname->value);
        emit_array_suffix(e, ptype);
        param_idx++;
    }

    strbuf_append(&e->output, ") {\n");

    e->indent_level++;
    emit_block(e, node->children[node->child_count - 1]);
    e->indent_level--;

    strbuf_append(&e->output, "}\n\n");
}

static void emit_struct_def(Emitter *e, ASTNode *node) {
    strbuf_append_fmt(&e->output, "typedef struct %s {\n", node->value);
    e->indent_level++;

    for (int i = 0; i < node->child_count; i++) {
        ASTNode *member = node->children[i];
        ASTNode *mname = member->children[0];
        ASTNode *mtype = member->children[1];
        
        emit_indent(e);
        emit_type(e, mtype);
        strbuf_append_fmt(&e->output, " %s", mname->value);
        emit_array_suffix(e, mtype);
        strbuf_append(&e->output, ";\n");
    }

    e->indent_level--;
    strbuf_append_fmt(&e->output, "} %s;\n\n", node->value);
}


static void emit_include(Emitter *e, ASTNode *node) {
    const char *val = node->value;
    if (val[0] == '<') {
        int len = strlen(val);
        char name[128] = "";
        strncpy(name, val + 1, len - 2);
        strbuf_append_fmt(&e->output, "#include <%s.h>\n", name);
    } else if (val[0] == '"') {
        int len = strlen(val);
        char name[128] = "";
        strncpy(name, val + 1, len - 2);
        strbuf_append_fmt(&e->output, "#include \"%s.h\"\n", name);
    } else {
        strbuf_append_fmt(&e->output, "#include <%s.h>\n", val);
    }
}

static void emit_macro(Emitter *e, ASTNode *node) {
    strbuf_append_fmt(&e->output, "#define %s ", node->value);
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, "\n");
}

// Statement and block traversals
static void emit_statement(Emitter *e, ASTNode *node) {
    switch (node->type) {
        case NODE_VAR_INIT:     emit_var_init(e, node); break;
        case NODE_VAR_ASSIGN:   emit_var_assign(e, node); break;
        case NODE_VAR_DECL:     emit_var_decl(e, node); break;
        case NODE_IF:           emit_if(e, node); break;
        case NODE_WHILE:        emit_while(e, node); break;
        case NODE_FOR:          emit_for(e, node); break;
        case NODE_DO_WHILE:     emit_do_while(e, node); break;
        case NODE_SWITCH:       emit_switch(e, node); break;
        case NODE_RETURN:       emit_return(e, node); break;
        case NODE_BREAK:        emit_break(e, node); break;
        case NODE_CONTINUE:     emit_continue(e, node); break;
        case NODE_INCREMENT:    emit_increment(e, node); break;
        case NODE_DECREMENT:    emit_decrement(e, node); break;
        case NODE_COMPOUND_ASSIGN: emit_compound_assign(e, node); break;
        case NODE_FUNC_CALL:
            emit_indent(e);
            emit_expression(e, node);
            strbuf_append(&e->output, ";\n");
            break;
        default:
            emit_indent(e);
            strbuf_append(&e->output, "/* unhandled statement */\n");
            break;
    }
}

static void emit_block(Emitter *e, ASTNode *block) {
    for (int i = 0; i < block->child_count; i++) {
        emit_statement(e, block->children[i]);
    }
}

// Main Emitter Entry Point
void emitter_emit(Emitter *e, ASTNode *root) {
    for (int i = 0; i < root->child_count; i++) {
        ASTNode *item = root->children[i];
        switch (item->type) {
            case NODE_INCLUDE:      emit_include(e, item); break;
            case NODE_MACRO_DEFINE: emit_macro(e, item); break;
            case NODE_STRUCT_DEF:   emit_struct_def(e, item); break;
            case NODE_FUNC_DEF:     emit_function_def(e, item); break;
            default:
                strbuf_append(&e->output, "/* unhandled top-level */\n");
                break;
        }
    }
}