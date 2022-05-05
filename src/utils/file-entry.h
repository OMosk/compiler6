#pragma once

#include <inttypes.h>

#include "string.h"

struct FileEntry {
  Str absolutePath;
  Str relativePath;
  Str content;
  uint32_t index;
};

