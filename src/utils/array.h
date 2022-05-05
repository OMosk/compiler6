#pragma once

#include <stdint.h>
#include <assert.h>

#include "allocator.h"

template<typename T>
struct Array {
  T *data;
  uint32_t len;
  uint32_t cap;

  T& operator[](int index) {
    assert(index >= 0);
    assert(index < len);
    return data[index];
  }
};

template<typename T>
void reserve(Array<T> *arr, uint32_t newCap, Allocator *allocator) {
  if (newCap > arr->cap) {
    arr->data = (T *)realloc(arr->data, sizeof(T), alignof(T), arr->len, newCap, allocator);
    arr->cap = newCap;
  }
}

template<typename T>
void resize(Array<T> *arr, uint32_t size, Allocator *a) {
  reserve(arr, (arr->cap * 2 > size ? arr->cap * 2 : size), a);
  arr->len = size;
}

template<typename T>
void ensureAtLeast(Array<T> *arr, uint32_t size, Allocator *a) {
  if (arr->len < size) {
    resize(arr, size, a);
  }
}

template<typename T>
void append(Array<T> *arr, T item, Allocator *a) {
  if (arr->cap == 0) {
    reserve(arr, 8, a);
  }
  if (arr->len == arr->cap) {
    reserve(arr, arr->cap * 2, a);
  }
  arr->data[arr->len] = item;
  arr->len++;
}
