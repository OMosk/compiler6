#include "utils.hpp"

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

Allocator global;

void initAllocator(Allocator *a, char *start, int64_t size) {
  memset(start, 0, size);
  a->start = start;
  a->current = start;
  a->end = start + size;
}


void *alloc(size_t size, size_t alignment, int n, Allocator *allocator) {
  if (n == 0) return NULL;
  auto s = size;
  auto a = alignment;
  auto shift = a - (intptr_t)allocator->current % a;
  if (shift == a) shift = 0;
  auto result = allocator->current + shift;
  if ((intptr_t)result % a != 0) {
    abort();
  }
  allocator->current += shift + s * n;
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

Str StrDup(Str s) {
  Str result = s;
  result.data = ALLOC_ARRAY(char, s.len);
  memcpy(result.data, s.data, s.len);
  return result;
}

Str SPrintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int want_len = vsnprintf(NULL, 0, fmt, args)+1;
  va_end(args);

  va_start(args, fmt);
  Str buffer;
  buffer.data = ALLOC_ARRAY(char, want_len);
  buffer.len = vsnprintf(buffer.data, want_len, fmt, args);
  va_end(args);

  return buffer;
}

FileReadResult readFile(const char *filename) {
  FileReadResult result = {};
  FILE *f = fopen(filename, "r");
  if (!f) {
    result.ok = false;
    return result;
  }
  fseek(f, 0, SEEK_END);
  result.content.len = ftell(f);
  fseek(f, 0, SEEK_SET);

  result.content.data = ALLOC_ARRAY(char, result.content.len);
  memset(result.content.data, 0, result.content.len);
  fread(result.content.data, result.content.len, 1, f);
  fclose(f);

  result.ok = true;

  return result;
}

struct timespec start_clock;
void initClock() {
  clock_gettime(CLOCK_MONOTONIC, &start_clock);
}

double now() {
  struct timespec res;
  clock_gettime(CLOCK_MONOTONIC, &res);
  res.tv_sec -= start_clock.tv_sec;
  res.tv_nsec -= start_clock.tv_nsec;
  if (res.tv_nsec < 0) {
    res.tv_sec -= 1;
    res.tv_nsec += 1000000000;
  }
  time_t sec = res.tv_sec;
  long nsec = res.tv_nsec;
  return (sec * 1.0) + (nsec * 1e-9);
}

int32_t utf8advance(char **it, char *end) {
  if (*it >= end) return EOF;
  if (**(unsigned char**)it > 127) abort();
  int32_t result = **it;
  ++(*it);
  return result;
}

StaticArray<FileEntry, 1024> files;

void showError(Site loc, Str str) {
  va_list args;
  printf("%s:%d:%d Error: ", files.data[loc.file].relative_path, loc.line, loc.col);
  printf("%.*s\n", (int)str.len, str.data);
  va_end(args);
}

void showError(Site loc, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("%s:%d:%d Error: ", files.data[loc.file].relative_path, loc.line, loc.col);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

void showError(int file_index, int line, int col, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("%s:%d:%d Error: ", files.data[file_index].relative_path, line, col);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

void showCodeLocation(Site loc, bool showUnderline) {
  FileEntry entry = files.data[loc.file];
  auto it = entry.content.data;
  auto end = entry.content.data + entry.content.len;
  int line = 1;
  int col = 1;
  int lineLength = 0;

  Str currentLine = {};
  currentLine.data = it;

  while (it < end) {
    auto it_prev = it;
    int ch = utf8advance(&it, end);
    col += 1;
    lineLength += 1;
    if (ch == '\n') {
      currentLine.len = it_prev - currentLine.data;
      if (line == loc.line) {
        break;
      }
      currentLine.data = it;

      line += 1;
      col = 1;
      lineLength = 0;
    }
  }

  int locStrLen = printf("%s:%d:%d", entry.relative_path, loc.line, loc.col);
  printf("|%.*s\n", (int)currentLine.len, currentLine.data);

  //TODO: Rework showUnderline
  /*
  if (showUnderline) {
    for (int i = 0; i < locStrLen; ++i) {
      putchar(' ');
    }
    putchar('|');
    for (int i = 0; i < loc.col - 1; ++i) {
      putchar(' ');
    }
    auto it = loc.start;
    auto end = loc.end;
    auto i = 0;
    while (i < lineLength && it < end) {
      i += 1;
      utf8advance(&it, end);
      putchar('~');
    }
    putchar('\n');
  }
  */

  for (int i = 0; i < locStrLen; ++i) {
    putchar(' ');
  }
  putchar('|');
  for (int i = 0; i < loc.col - 1; ++i) {
    putchar(' ');
  }
  putchar('^');
  putchar('\n');
  for (int i = 0; i < locStrLen; ++i) {
    putchar(' ');
  }
  putchar('|');
  for (int i = 0; i < loc.col - 1; ++i) {
    printf("─");
  }
  printf("┘");
  putchar('\n');
}


uint64_t alignAddressUpwards(uint64_t ptr, uint64_t alignment) {
  auto a = alignment;
  auto shift = a - ptr % a;
  if (shift == a) shift = 0;
  auto result = ptr + shift;
  ASSERT(result % alignment == 0, "Failed to align");

  return result;
}


Str concatAndNormalizePath(const char *curDirectory, const char *filename) {
  Str fullPath = SPrintf("%s/%s", curDirectory, filename);
  Str normalizedFullPath = {};
  normalizedFullPath.len = PATH_MAX + 1;
  normalizedFullPath.data = ALLOC_ARRAY(char, PATH_MAX + 1);

  const char *result = realpath(fullPath.data, normalizedFullPath.data);
  if (!result) {
    printf("realpath failed: %s\n", strerror(errno));
    abort();
  }

  normalizedFullPath.len = strlen(result);

  return normalizedFullPath;
}
