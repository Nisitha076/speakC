#include "../../include/util/arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static ArenaBlock *arena_new_block(int capacity) {
    ArenaBlock *block = malloc(sizeof(ArenaBlock));
    if (!block) {
        fprintf(stderr, "arena: out of memory \n");
        exit(1);
    }
    block->data = malloc(capacity);
    if (!block->data) {
        fprintf(stderr, "arena: out of memory\n");
        free(block);
        exit(1);
    }
    block->used = 0;
    block->capacity = capacity;
    block->next = NULL;
    return block;
}

void arena_init(Arena *a) {
    a->first = arena_new_block(ARENA_BLOCK_SIZE);
    a->current = a->first;
}

void *arena_alloc(Arena *a, size_t size) {
    // Align 8 bytes
    size = (size + 7) & ~(size_t)7;

    if (a->current->used + (int) size > a->current->capacity) {
        int cap = ARENA_BLOCK_SIZE;
        if ((int)size > cap) {
            cap = (int)size;
        }
        ArenaBlock *new_block = arena_new_block(cap);
        a->current->next = new_block;
        a->current = new_block;
    }

    void *ptr = a->current->data + a->current->used;
    a->current->used += (int)size;
    return ptr;
}

void arena_free(Arena *a) {
    ArenaBlock *block = a->first;
    while (block) {
        ArenaBlock *next = block->next;
        free(block->data);
        free(block);
        block = next;
    }
    a->first = NULL;
    a->current = NULL;
}