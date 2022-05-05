#pragma once

#include <inttypes.h>

#include <linux/limits.h> //PATH_MAX

#include "utils/allocator.h"
#include "utils/file-entry.h"
#include "utils/array.h"

struct GlobalData {
  Array<FileEntry> files;
  pthread_mutex_t filesMutex;

  char currentWorkingDirectory[PATH_MAX + 1];//4KB + 1
};

struct ThreadData {
  GlobalData *globalData;
  Allocator allocator;
};
