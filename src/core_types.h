#pragma once

// @TODO: rename file to use - instead of _ in filename

#include <inttypes.h>

#include <linux/limits.h> //PATH_MAX

#include "utils/allocator.h"
#include "utils/string.h"
#include "utils/array.h"

struct FileEntry {
  Str absolutePath;
  Str relativePath;
  Str content;
  uint32_t index;
};

struct GlobalData {
  Array<FileEntry> files;
  pthread_mutex_t filesMutex;

  char currentWorkingDirectory[PATH_MAX + 1];//4KB + 1
};

struct ThreadData {
  GlobalData *globalData;
  Allocator allocator;
};
