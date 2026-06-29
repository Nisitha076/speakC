#ifndef VEC_H
#define VEC_H

#include <stddef.h>

// Generic dynamic array that stores void pointers
// I push pointers to my data, and cast them back when I retrieve

typedef struct {
  void **items; // array of pointers
  int count; // count of items in the vector
  int capacity; // how many items the vector can hold rn
} Vec;

void vec_init(Vec *v);
void vec_push(Vec *v, void *item);
void *vec_get(Vec *v, int index);
void vec_free(Vec *v);

#endif