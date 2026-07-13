#include "../include/speakc_emitter.h"
#include <string.h>

void speakc_emitter_init(SpeakCEmitter *e) {
    strbuf_init(&e->output);
    e->indent_level = 0;
}

void speakc_emitter_free(SpeakCEmitter *e) {
    strbuf_free(&e->output);
}

char *speakc_emitter_get_output(SpeakCEmitter *e) {
    return strbuf_to_string(&e->output);
}

// Indentation helper — same as your C17 emitter but with 4 spaces
static void emit_indent(SpeakCEmitter *e) {
    for (int i = 0; i < e->indent_level; i++) {
        strbuf_append(&e->output, "    ");
    }
}

// Forward declarations
static void emit_expression(SpeakCEmitter *e, ASTNode *node);
static void emit_statement(SpeakCEmitter *e, ASTNode *node);
static void emit_block(SpeakCEmitter *e, ASTNode *block);

static void emit_type(SpeakCEmitter *e, ASTNode *node) {
    if (node->type != NODE_TYPE) return;

    const char *val = node->value;

    if (strcmp(val, "pointer") == 0) {
        // "int *" → "pointer to int"
        strbuf_append(&e->output, "pointer to ");
        emit_type(e, node->children[0]);
    } else if (strcmp(val, "array") == 0) {
        // "int [10]" → "array of 10 int"
        strbuf_append_fmt(&e->output, "array of %s ", node->children[0]->value);
        emit_type(e, node->children[1]);
    } else if (strcmp(val, "const") == 0) {
        // "const int" → "constant int"
        strbuf_append(&e->output, "constant ");
        emit_type(e, node->children[0]);
    } else {
        // Base type: int, float, char, etc.
        strbuf_append(&e->output, val);
    }
}

static void emit_expression(SpeakCEmitter *e, ASTNode *node) {
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
            // C: point.x or ptr->x
            // SpeakC: x of point or x of value at ptr
            //
            // AST for ptr->x: MEMBER_ACCESS(DEREF(ptr), x)
            // SpeakC output: x of value at ptr
            //
            // AST for point.x: MEMBER_ACCESS(point, x)
            // SpeakC output: x of point

            // Print member name first (reversed from C)
            strbuf_append(&e->output, node->children[1]->value);
            strbuf_append(&e->output, " of ");

            // Print object (might be a deref for pointer access)
            if (node->children[0]->type == NODE_DEREF) {
                strbuf_append(&e->output, "value at ");
                emit_expression(e, node->children[0]->children[0]);
            } else {
                emit_expression(e, node->children[0]);
            }
            break;

        case NODE_ADDRESS_OF:
            // C: &x → SpeakC: address of x
            strbuf_append(&e->output, "address of ");
            emit_expression(e, node->children[0]);
            break;

        case NODE_DEREF:
            // C: *p → SpeakC: value at p
            strbuf_append(&e->output, "value at ");
            emit_expression(e, node->children[0]);
            break;

        case NODE_SIZEOF:
            // C: sizeof(int) → SpeakC: size of int
            strbuf_append(&e->output, "size of ");
            emit_type(e, node->children[0]);
            break;

        case NODE_CAST:
            // C: (float)x → SpeakC: cast x to float
            strbuf_append(&e->output, "cast ");
            emit_expression(e, node->children[0]);
            strbuf_append(&e->output, " to ");
            emit_type(e, node->children[1]);
            break;

        case NODE_FUNC_CALL:
            // Same in both: printf("hello", x)
            strbuf_append(&e->output, node->value);
            strbuf_append(&e->output, "(");
            for (int i = 0; i < node->child_count; i++) {
                if (i > 0) strbuf_append(&e->output, ", ");
                emit_expression(e, node->children[i]);
            }
            strbuf_append(&e->output, ")");
            break;

        case NODE_ARRAY_ACCESS:
            // Same in both: arr[i]
            emit_expression(e, node->children[0]);
            strbuf_append(&e->output, "[");
            emit_expression(e, node->children[1]);
            strbuf_append(&e->output, "]");
            break;

        case NODE_UNARY_OP:
            strbuf_append(&e->output, node->value);
            emit_expression(e, node->children[0]);
            break;

        default:
            strbuf_append(&e->output, "/* unknown expr */");
            break;
    }
}

// C: int x = 5; → SpeakC: set x to 5 as int
static void emit_var_init(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    ASTNode *name = node->children[0];
    ASTNode *value = node->children[1];
    ASTNode *type = node->children[2];

    strbuf_append(&e->output, "set ");
    strbuf_append(&e->output, name->value);
    strbuf_append(&e->output, " to ");
    emit_expression(e, value);
    strbuf_append(&e->output, " as ");
    emit_type(e, type);
    strbuf_append(&e->output, "\n");
}

// C: x = 10; → SpeakC: set x to 10
static void emit_var_assign(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "set ");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, " to ");
    emit_expression(e, node->children[1]);
    strbuf_append(&e->output, "\n");
}

// C: int x; → SpeakC: declare x as int
static void emit_var_decl(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    ASTNode *name = node->children[0];
    ASTNode *type = node->children[1];

    strbuf_append(&e->output, "declare ");
    strbuf_append(&e->output, name->value);
    strbuf_append(&e->output, " as ");
    emit_type(e, type);
    strbuf_append(&e->output, "\n");
}

// C: if (x > 5) { ... } → SpeakC: if x > 5:\n    ...
static void emit_if(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "if ");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, ":\n");

    e->indent_level++;
    emit_block(e, node->children[1]);
    e->indent_level--;

    // Handle else if / else
    int i = 2;
    while (i < node->child_count) {
        if (i + 1 < node->child_count && node->children[i]->type != NODE_BLOCK) {
            // else if
            emit_indent(e);
            strbuf_append(&e->output, "else if ");
            emit_expression(e, node->children[i]);
            strbuf_append(&e->output, ":\n");
            e->indent_level++;
            emit_block(e, node->children[i + 1]);
            e->indent_level--;
            i += 2;
        } else {
            // else
            emit_indent(e);
            strbuf_append(&e->output, "else:\n");
            e->indent_level++;
            emit_block(e, node->children[i]);
            e->indent_level--;
            i++;
        }
    }
}

// C: while (x > 0) { ... } → SpeakC: while x > 0:\n    ...
static void emit_while(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "while ");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, ":\n");

    e->indent_level++;
    emit_block(e, node->children[1]);
    e->indent_level--;
}

// C: for (int i = 0; i < 10; i++) { ... }
// SpeakC: for set i to 0 as int, i < 10, increment i:\n    ...
static void emit_for(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "for ");

    // Init
    ASTNode *init = node->children[0];
    if (init->type == NODE_VAR_INIT) {
        strbuf_append(&e->output, "set ");
        strbuf_append(&e->output, init->children[0]->value);
        strbuf_append(&e->output, " to ");
        emit_expression(e, init->children[1]);
        strbuf_append(&e->output, " as ");
        emit_type(e, init->children[2]);
    } else if (init->type == NODE_VAR_ASSIGN) {
        strbuf_append(&e->output, "set ");
        emit_expression(e, init->children[0]);
        strbuf_append(&e->output, " to ");
        emit_expression(e, init->children[1]);
    }

    strbuf_append(&e->output, ", ");

    // Condition
    emit_expression(e, node->children[1]);

    strbuf_append(&e->output, ", ");

    // Step
    ASTNode *step = node->children[2];
    if (step->type == NODE_INCREMENT) {
        strbuf_append(&e->output, "increment ");
        emit_expression(e, step->children[0]);
    } else if (step->type == NODE_DECREMENT) {
        strbuf_append(&e->output, "decrement ");
        emit_expression(e, step->children[0]);
    } else if (step->type == NODE_COMPOUND_ASSIGN) {
        emit_expression(e, step->children[0]);
        strbuf_append_fmt(&e->output, " %s ", step->value);
        emit_expression(e, step->children[1]);
    }

    strbuf_append(&e->output, ":\n");

    e->indent_level++;
    emit_block(e, node->children[3]);
    e->indent_level--;
}

// C: do { ... } while (x > 0); → SpeakC: do:\n    ...\nwhile x > 0
static void emit_do_while(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "do:\n");

    e->indent_level++;
    emit_block(e, node->children[0]);
    e->indent_level--;

    emit_indent(e);
    strbuf_append(&e->output, "while ");
    emit_expression(e, node->children[1]);
    strbuf_append(&e->output, "\n");
}

// C: switch (x) { case 1: ... } → SpeakC: switch on x:\n    case 1:\n        ...
static void emit_switch(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "switch on ");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, ":\n");

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
}

// C: return 0; → SpeakC: return 0
static void emit_return(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "return");
    if (node->child_count > 0) {
        strbuf_append(&e->output, " ");
        emit_expression(e, node->children[0]);
    }
    strbuf_append(&e->output, "\n");
}

// C: break; → SpeakC: break
static void emit_break(SpeakCEmitter *e, ASTNode *node) {
    (void)node;
    emit_indent(e);
    strbuf_append(&e->output, "break\n");
}

// C: continue; → SpeakC: continue
static void emit_continue(SpeakCEmitter *e, ASTNode *node) {
    (void)node;
    emit_indent(e);
    strbuf_append(&e->output, "continue\n");
}

// C: i++; → SpeakC: increment i
static void emit_increment(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "increment ");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, "\n");
}

// C: i--; → SpeakC: decrement i
static void emit_decrement(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "decrement ");
    emit_expression(e, node->children[0]);
    strbuf_append(&e->output, "\n");
}

// C: i += 2; → SpeakC: add 2 to i
static void emit_compound_assign(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    const char *op = node->value;

    if (strcmp(op, "+=") == 0) {
        strbuf_append(&e->output, "add ");
        emit_expression(e, node->children[1]);
        strbuf_append(&e->output, " to ");
        emit_expression(e, node->children[0]);
    } else if (strcmp(op, "-=") == 0) {
        strbuf_append(&e->output, "subtract ");
        emit_expression(e, node->children[1]);
        strbuf_append(&e->output, " from ");
        emit_expression(e, node->children[0]);
    } else if (strcmp(op, "*=") == 0) {
        strbuf_append(&e->output, "multiply ");
        emit_expression(e, node->children[0]);
        strbuf_append(&e->output, " by ");
        emit_expression(e, node->children[1]);
    } else if (strcmp(op, "/=") == 0) {
        strbuf_append(&e->output, "divide ");
        emit_expression(e, node->children[0]);
        strbuf_append(&e->output, " by ");
        emit_expression(e, node->children[1]);
    } else {
        // Fallback
        emit_expression(e, node->children[0]);
        strbuf_append_fmt(&e->output, " %s ", op);
        emit_expression(e, node->children[1]);
    }
    strbuf_append(&e->output, "\n");
}

// C: int add(int a, int b) { ... }
// SpeakC: define function add(a as int, b as int) returns int:
static void emit_function_def(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append(&e->output, "define function ");
    strbuf_append(&e->output, node->value);

    // Parameters (pairs of name, type starting at index 1)
    int param_limit = node->child_count - 1;  // last child is the body
    int has_params = (param_limit - 1) > 0;

    if (has_params) {
        strbuf_append(&e->output, "(");
        int param_idx = 0;
        for (int i = 1; i < param_limit; i += 2) {
            if (param_idx > 0) strbuf_append(&e->output, ", ");
            strbuf_append(&e->output, node->children[i]->value);  // param name
            strbuf_append(&e->output, " as ");
            emit_type(e, node->children[i + 1]);  // param type
            param_idx++;
        }
        strbuf_append(&e->output, ")");
    }

    // Return type
    ASTNode *ret_type = node->children[0];
    strbuf_append(&e->output, " returns ");
    emit_type(e, ret_type);

    strbuf_append(&e->output, ":\n");

    // Body
    e->indent_level++;
    emit_block(e, node->children[node->child_count - 1]);
    e->indent_level--;

    strbuf_append(&e->output, "\n");
}

// C: struct Point { int x; int y; };
// SpeakC: define Point as structure:
//     declare x as int
//     declare y as int
static void emit_struct_def(SpeakCEmitter *e, ASTNode *node) {
    emit_indent(e);
    strbuf_append_fmt(&e->output, "define %s as structure:\n", node->value);

    e->indent_level++;
    for (int i = 0; i < node->child_count; i++) {
        ASTNode *member = node->children[i];
        ASTNode *mname = member->children[0];
        ASTNode *mtype = member->children[1];

        emit_indent(e);
        strbuf_append(&e->output, "declare ");
        strbuf_append(&e->output, mname->value);
        strbuf_append(&e->output, " as ");
        emit_type(e, mtype);
        strbuf_append(&e->output, "\n");
    }
    e->indent_level--;
    strbuf_append(&e->output, "\n");
}

// C: #include <stdio.h> → SpeakC: include <stdio>
static void emit_include(SpeakCEmitter *e, ASTNode *node) {
    const char *val = node->value;
    emit_indent(e);
    strbuf_append(&e->output, "include ");

    // Strip ".h" from the include value
    // Input might be: <stdio.h> or "myfile.h"
    int len = strlen(val);
    if (len > 2 && val[len - 1] == '>' && val[0] == '<') {
        // System include: <stdio.h> → <stdio>
        char name[128] = "";
        // Copy between < and >, removing .h if present
        int name_len = len - 2;  // strip < and >
        memcpy(name, val + 1, name_len);
        name[name_len] = '\0';
        // Remove .h suffix if present
        char *dot = strrchr(name, '.');
        if (dot && strcmp(dot, ".h") == 0) *dot = '\0';
        strbuf_append_fmt(&e->output, "<%s>", name);
    } else if (len > 2 && val[0] == '"') {
        // Local include: "myfile.h" → "myfile"
        char name[128] = "";
        int name_len = len - 2;
        memcpy(name, val + 1, name_len);
        name[name_len] = '\0';
        char *dot = strrchr(name, '.');
        if (dot && strcmp(dot, ".h") == 0) *dot = '\0';
        strbuf_append_fmt(&e->output, "\"%s\"", name);
    } else {
        strbuf_append(&e->output, val);
    }
    strbuf_append(&e->output, "\n");
}

static void emit_statement(SpeakCEmitter *e, ASTNode *node) {
    switch (node->type) {
        case NODE_VAR_INIT:        emit_var_init(e, node); break;
        case NODE_VAR_ASSIGN:      emit_var_assign(e, node); break;
        case NODE_VAR_DECL:        emit_var_decl(e, node); break;
        case NODE_IF:              emit_if(e, node); break;
        case NODE_WHILE:           emit_while(e, node); break;
        case NODE_FOR:             emit_for(e, node); break;
        case NODE_DO_WHILE:        emit_do_while(e, node); break;
        case NODE_SWITCH:          emit_switch(e, node); break;
        case NODE_RETURN:          emit_return(e, node); break;
        case NODE_BREAK:           emit_break(e, node); break;
        case NODE_CONTINUE:        emit_continue(e, node); break;
        case NODE_INCREMENT:       emit_increment(e, node); break;
        case NODE_DECREMENT:       emit_decrement(e, node); break;
        case NODE_COMPOUND_ASSIGN: emit_compound_assign(e, node); break;
        case NODE_FUNC_CALL:
            emit_indent(e);
            emit_expression(e, node);
            strbuf_append(&e->output, "\n");
            break;
        default:
            emit_indent(e);
            strbuf_append(&e->output, "// unhandled statement\n");
            break;
    }
}

static void emit_block(SpeakCEmitter *e, ASTNode *block) {
    for (int i = 0; i < block->child_count; i++) {
        emit_statement(e, block->children[i]);
    }
}

void speakc_emitter_emit(SpeakCEmitter *e, ASTNode *root) {
    for (int i = 0; i < root->child_count; i++) {
        ASTNode *item = root->children[i];
        switch (item->type) {
            case NODE_INCLUDE:      emit_include(e, item); break;
            case NODE_STRUCT_DEF:   emit_struct_def(e, item); break;
            case NODE_FUNC_DEF:     emit_function_def(e, item); break;
            default:
                strbuf_append(&e->output, "// unhandled top-level\n");
                break;
        }
    }
}

