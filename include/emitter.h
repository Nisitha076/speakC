#ifndef EMITTER_H
#define EMITTER_H

#include "ast.h"
#include "util/strbuf.h"
#include "util/vec.h"

typedef struct {
    StrBuf output; // The C code being build
    int indent_level; // Current indentation depth (for formatting)
    Vec includes; // Track includes that have been emitted
} Emitter;

void emitter_init(Emitter *e);
void emitter_emit(Emitter *e, ASTNode *root);
char *emitter_get_output(Emitter *e);
void emitter_free(Emitter *e);

#endif