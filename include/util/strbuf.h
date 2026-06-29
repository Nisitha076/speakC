#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>

typedef struct {
    char *data;
    int length;
    int capacity;
} StrBuf;

void strbuf_init(StrBuf *sb);
void strbuf_append(StrBuf *sb, const char *str);
void strbuf_append_char(StrBuf *sb, char c);
void strbuf_append_fmt(StrBuf *sb, const char *fmt, ...);
char *strbuf_to_string(StrBuf *sb);
void strbuf_free(StrBuf *sb);

#endif