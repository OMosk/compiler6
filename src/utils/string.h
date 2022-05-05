#pragma once

#include <stddef.h>
#include <string.h>

#include "allocator.h"

struct Str {
  char *data;
  size_t len;
};

#define STR(LITERAL) (Str {(char *)LITERAL, sizeof(LITERAL)-1})

Str StrDup(Str s);
Str SPrintf(Allocator *a, const char *fmt, ...);
