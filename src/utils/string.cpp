#include "string.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

Str StrDup(Str s, Allocator *a) {
  Str result = s;
  result.data = ALLOC_ARRAY(char, s.len, a);
  memcpy(result.data, s.data, s.len);
  return result;
}

Str SPrintf(Allocator *a, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int want_len = vsnprintf(NULL, 0, fmt, args)+1;
  va_end(args);

  va_start(args, fmt);
  Str buffer;
  buffer.data = ALLOC_ARRAY(char, want_len, a);
  buffer.len = vsnprintf(buffer.data, want_len, fmt, args);
  va_end(args);

  return buffer;
}
