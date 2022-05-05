#include "allocator.h"

#include <string.h>
#include <assert.h>

void initAllocator(Allocator *a, char *start, int64_t size) {
  //memset(start, 0, size);
  a->start = start;
  a->current = start;
  a->end = start + size;
}

void *alloc(size_t size, size_t alignment, int n, Allocator *allocator) {
  if (n == 0) return NULL;

  allocator->current = (char *) alignAddressUpwards((intptr_t)allocator->current, alignment);
  auto result = allocator->current;

  allocator->current += size * n;
  if (allocator->current >= allocator->end) abort();
  allocator->allocations++;

  return result;
}


void *realloc(void *oldData, size_t size, size_t alignment, size_t oldLength, size_t newCap, Allocator *allocator) {
  auto newData = alloc(size, alignment, newCap, allocator);
  memcpy(newData, oldData, oldLength * size);
  return newData;
}

size_t usage(Allocator *a) {
  return (a->current - a->start);
}

void reset(Allocator *a) {
  a->current = a->start;
}


uint64_t alignAddressUpwards(uint64_t ptr, uint64_t alignment) {
  auto a = alignment;
  auto shift = a - ptr % a;
  if (shift == a) {
    shift = 0;
  }
  auto result = ptr + shift;

  assert(result % alignment == 0);

  return result;
}
