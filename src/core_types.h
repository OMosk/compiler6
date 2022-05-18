#pragma once

#include <inttypes.h>

#include <pthread.h>
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

struct Compiler;
struct GlobalData {
  Compiler *compiler;

  Array<FileEntry> files;
  pthread_mutex_t filesMutex;

  char currentWorkingDirectory[PATH_MAX + 1];//4KB + 1
};

void initGlobalData(GlobalData *globalData);

FileEntry file(GlobalData *globalData, int fileIndex);

struct ThreadData {
  GlobalData *globalData;
  Allocator allocator;
};

void initThreadData(ThreadData *td, GlobalData *globalData, void *memory, size_t size);
