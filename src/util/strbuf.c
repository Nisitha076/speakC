#include "../../include/util/strbuf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


#define INITIAL_CAPACITY 64
// Allocates an initial memory buffer of 64 bytes
void strbuf_init(StrBuf *sb) {
    sb->data = malloc(INITIAL_CAPACITY);
    if (!sb->data) {
        fprintf(stderr, "strbuf_grow: out of memory\n");
        exit(1);
    }
    sb->data[0] = '\0';
    sb->length = 0;
    sb->capacity = INITIAL_CAPACITY;
}
// Doubles the capacity of memory buffer
static void strbuf_grow(StrBuf *sb, int needed) {
    while (sb->capacity < needed) {
        sb->capacity *=2;
    }
    sb->data = realloc(sb->data, sb->capacity);
    if (!sb->data) {
        fprintf(stderr, "strbuf_grow: out of memory\n");
        exit(1);
    }
}
// Appeneds Strings to end of buffer
void strbuf_append(StrBuf *sb, const char *str) {
    int len = strlen(str);
    int new_length = sb->length + len + 1;
    if (new_length > sb-> capacity) {
        strbuf_grow(sb, new_length);        
    }
     memcpy(sb->data + sb->length, str, len + 1);
     sb->length += len;
}
// Appeneds single character
void strbuf_append_char(StrBuf *sb, char c) {
    int new_length = sb->length + 2;
    if (new_length > sb->capacity) {
        strbuf_grow(sb, new_length);
    }
    sb->data[sb->length] = c;
    sb->length++;
    sb->data[sb->length] = '\0';
}
// Formats a string and appends it
void strbuf_append_fmt(StrBuf *sb, const char *fmt, ...) {
    char temp[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);
    strbuf_append(sb, temp);
}
// Returns a fresh copy of the accumulated string
char *strbuf_to_string(StrBuf *sb) {
    char *result = malloc(sb->length + 1);
    if (!result) {
        fprintf(stderr, "strbuf_to_string: out of memory\n");
        exit(1);
    }
    memcpy(result, sb->data, sb->length + 1);
    return result;
}
// Deallocates the internal buffer
void strbuf_free(StrBuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
}