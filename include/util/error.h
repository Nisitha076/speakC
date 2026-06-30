#ifndef UTIL_ERROR_H
#define UTIL_ERROR_H

// Print a detailed error report with colors, source lines, and caret pointer.
void report_error(const char *filename, const char *source, int line, int column, const char *message);

#endif
