#include "../../include/util/error.h"
#include <stdio.h>

void report_error(const char *filename, const char *source, int line, int column, const char *message) {
    // Print the error header in bold red
    fprintf(stderr, "\033[1;31merror:\033[0m %s\n", message);
    fprintf(stderr, "  --> %s:%d:%d\n", filename, line, column);

    // Find the start of the specified line
    int current_line = 1;
    const char *line_start = source;
    while (current_line < line && *line_start != '\0') {
        if (*line_start == '\n') {
            current_line++;
        }
        line_start++;
    }

    // If line not found, just return
    if (*line_start == '\0' && current_line < line) {
        return;
    }

    // Find the end of the line
    const char *line_end = line_start;
    while (*line_end != '\0' && *line_end != '\n') {
        line_end++;
    }

    // Print the line number and the line content
    fprintf(stderr, "   |\n");
    fprintf(stderr, "%2d | %.*s\n", line, (int)(line_end - line_start), line_start);

    // Print the ^ pointer pointing to the column
    fprintf(stderr, "   | ");
    for (int i = 1; i < column && (line_start + i - 1 < line_end); i++) {
        if (line_start[i - 1] == '\t') {
            fprintf(stderr, "\t");
        } else {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\033[1;31m^\033[0m\n");
    fprintf(stderr, "   |\n");
}
