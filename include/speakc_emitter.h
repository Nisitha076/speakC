#ifndef SPEAKC_EMITTER_H
#define SPEAKC_EMITTER_H

#include "ast.h"
#include "util/strbuf.h"

typedef struct {
    StrBuf output;        // The SpeakC code being built
    int indent_level;     // Current indentation depth
} SpeakCEmitter;

void speakc_emitter_init(SpeakCEmitter *e);
void speakc_emitter_emit(SpeakCEmitter *e, ASTNode *root);
char *speakc_emitter_get_output(SpeakCEmitter *e);
void speakc_emitter_free(SpeakCEmitter *e);

#endif