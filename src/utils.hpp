#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define ARRLEN(x) (sizeof(x)/sizeof(x[0]))
#define SIZEDARR(x) x, ARRLEN(x)

#define NOT_IMPLEMENTED do { printf("%s:%d:%s not implemented\n", __FILE__, __LINE__, __func__); abort(); } while(0)

#define ASSERT(cond, msg) do { if (!(cond)) {puts(msg); abort();}} while (0)

struct Allocator {
  char *start;
  char *current;
  char *end;
  size_t allocations;
};
extern Allocator global;

void initAllocator(Allocator *a, char *start, int64_t size);

void *alloc(size_t size, size_t alignment, int n = 1);
void *realloc(void *oldData, size_t size, size_t alignment, size_t oldLength, size_t newCap);
size_t usage(Allocator *a);

#define ALLOC(TYPE) ((TYPE *)alloc(sizeof(TYPE), alignof(TYPE)))
#define ALLOC_ARRAY(TYPE, N) ((TYPE *)alloc(sizeof(TYPE), alignof(TYPE), N))
#define REALLOC(TYPE, OLDDATA, LEN, NEWCAP) \
  ((TYPE *)realloc(OLDDATA, sizeof(TYPE), alignof(TYPE), LEN, NEWCAP))

struct Str {
  char *data;
  size_t len;
  bool eq(const char *s, size_t len) {
    if (this->len == len && memcmp(this->data, s, len) == 0) return true;
    return false;
  }
};
#define STR(LITERAL) (Str {(char *)LITERAL, sizeof(LITERAL)-1})

Str StrDup(Str s);

Str SPrintf(const char *fmt, ...);

template<typename T>
struct Array {
  T *data;
  uint32_t len;
  uint32_t cap;

  T& operator[](int index) {
    ASSERT(index >= 0, "Array index > 0: failed\n");
    ASSERT(index < len, "Array index < len: failed\n");
    return data[index];
  }
};

template<typename T>
void reserve(Array<T> *arr, uint32_t newCap) {
  if (newCap > arr->cap) {
    arr->data = (T *)realloc(arr->data, sizeof(T), alignof(T), arr->len, newCap);
    arr->cap = newCap;
  }
}
template<typename T>
void resize(Array<T> *arr, uint32_t size) {
  reserve(arr, size);
  arr->len = size;
}

template<typename T>
void append(Array<T> *arr, T item) {
  if (arr->len == 0) {
    reserve(arr, 8);
  }
  if (arr->len == arr->cap) {
    reserve(arr, arr->cap * 2);
  }
  arr->data[arr->len] = item;
  arr->len++;
}

template<typename T, int N>
struct StaticArray {
  T data[N];
  uint32_t len;
  const uint32_t cap = N;

  T& operator[](int index) {
    ASSERT(index >= 0, "StaticArray index > 0: failed\n");
    ASSERT(index < len, "StaticArray index < len: failed\n");
    return data[index];
  }
};

template<typename T, int N>
void append(StaticArray<T, N> *arr, T item) {
  if (arr->len == arr->cap) {
    printf("Reached static array capacity %d\n", arr->cap);
    abort();
  }
  arr->data[arr->len] = item;
  arr->len++;
}

struct FileReadResult {
  Str content;
  bool ok;
};

FileReadResult readFile(const char *filename);

void initClock();
double now();

int32_t utf8advance(char **it, char *end);

struct Site {
  uint32_t line;
  uint16_t col;
  uint16_t file;
};

struct FileEntry {
  const char *absolute_path;
  const char *relative_path;
  Str content;
};

extern StaticArray<FileEntry, 1024> files;

void showError(Site loc, Str str);
void showError(Site loc, const char *fmt, ...);
void showError(int file_index, int line, int col, const char *fmt, ...);
void showCodeLocation(Site loc, bool showUnderline = false);


