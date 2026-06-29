#ifndef FILE_IO_H
#define FILE_IO_H

// Read entire file into a malloc'd string. Returns NULL on failure.
// Caller must free the returned string.
char *read_file(const char *path);

// Write a string to a file. Returns 0 on success, -1 on failure.
int write_file(const char *path, const char *content);

#endif