#include "../../include/util/vec.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_CAPACITY 8

void vec_init(Vec *v) {
    v->items = malloc(sizeof(void *) * INITIAL_CAPACITY); 
    // Above line allocatae space for 8 pointers
    if (!v->items) {
        fprintf(stderr, "vec_init: out of memory\n");
        exit(1);
    } // breaks if it can't get memory to allocate
    v->count = 0;
    v->capacity = INITIAL_CAPACITY;
}

void vec_push(Vec *v, void *item) {
    if (v->count >= v->capacity) {
        v->capacity *= 2; // Double the capacity
        v->items = realloc(v->items, sizeof(void *) * v->capacity);
        if (!v->items) {
            fprintf(stderr, "vec_push: out of memory\n");
            exit(1);
        }
    }
    v->items[v->count] = item; // store the pointers
    v->count++; // Increment count
}

void *vec_get(Vec *v, int index) {
    if (index < 0 || index >= v->count) {
        fprintf(stderr, "vec_get: index %d out of bounds (count: %d)\n", index, v->count);
        exit(1);
    } // If we try to access out of bounds crash safe
    return v->items[index];
}

void vec_free(Vec *v) {
    free(v->items); // free the allocated memory
    v->items = NULL;
    v->count = 0;
    v->capacity = 0;
}
