#pragma once

#include "string.h"
#include "allocator.h"

struct FileReadResult {
  Str content;
  bool ok;
};

FileReadResult readFile(Allocator *a, const char *filename);
