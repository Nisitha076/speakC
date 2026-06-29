#include "../../include/util/file_io.h"
#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: could not open file '%s'\n", path);
        return NULL;
    }

    // Get file size
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: could not seek in '%s'\n", path);
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fprintf(stderr, "Error: could not determine size of '%s'\n", path);
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Error: could not seek in '%s'\n", path);
        fclose(f);
        return NULL;
    }

    // Allocate buffer (+1 for null terminator)
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: out of memory reading '%s'\n", path);
        fclose(f);
        return NULL;
    }

    // Read the whole file
    long read = fread(buffer, 1, size, f);
    buffer[read] = '\0';
    fclose(f);

    return buffer;
}

int write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Error: could not write to '%s'\n", path);
        return -1;
    }
    fputs(content, f);
    fclose(f);
    return 0;
}