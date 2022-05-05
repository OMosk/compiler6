#pragma once

#include <stdlib.h>
#include <stdint.h>

struct Allocator {
  char *start;
  char *current;
  char *end;
  size_t allocations;
};

void initAllocator(Allocator *a, char *start, int64_t size);

void *alloc(size_t size, size_t alignment, int n, Allocator *allocator);
void *realloc(void *oldData, size_t size, size_t alignment, size_t oldLength, size_t newCap, Allocator *allocator);
size_t usage(Allocator *a);
void reset(Allocator *a);
uint64_t alignAddressUpwards(uint64_t ptr, uint64_t alignment);

#define ALLOC(TYPE, ALLOCATOR) ((TYPE *)alloc(sizeof(TYPE), alignof(TYPE), 1, (ALLOCATOR)))
#define ALLOC_ARRAY(TYPE, N, ALLOCATOR) ((TYPE *)alloc(sizeof(TYPE), alignof(TYPE), N, (ALLOCATOR)))
#define REALLOC(TYPE, OLDDATA, LEN, NEWCAP, ALLOCATOR) \
  ((TYPE *)realloc(OLDDATA, sizeof(TYPE), alignof(TYPE), LEN, NEWCAP, (ALLOCATOR)))


