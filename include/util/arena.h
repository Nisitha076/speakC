#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#define ARENA_BLOCK_SIZE (1024 * 1024) // 1 MB blocks

typedef struct ArenaBlock {
    char *data;
    int used;
    int capacity;
    struct ArenaBlock *next;  // Linked list of blocks
} ArenaBlock;

typedef struct {
    ArenaBlock *first;
    ArenaBlock *current;
} Arena;

void arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t size);
void arena_free(Arena *a);

#endif