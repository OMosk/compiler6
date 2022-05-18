#include "core_types.h"

#include <stdio.h>

#include <unistd.h>

void initGlobalData(GlobalData *globalData) {
  *globalData = {};
  auto error = pthread_mutex_init(&globalData->filesMutex, NULL);
  if (error) {
    fprintf(stderr, "%s:%d pthread_mutex_init failed: %s\n",
      __FILE__, __LINE__, strerror(error));
    abort();
  }
  auto cwd = getcwd(globalData->currentWorkingDirectory,
    sizeof(globalData->currentWorkingDirectory));
  if (!cwd) {
    fprintf(stderr, "%s:%d getcwd failed: %s\n",
      __FILE__, __LINE__, strerror(error));
    abort();
  }
}

void initThreadData(ThreadData *td, GlobalData *globalData, void *memory, size_t size) {
  *td = {};
  td->globalData = globalData;
  initAllocator(&td->allocator, (char *) memory, size);
}

FileEntry file(GlobalData *globalData, int fileIndex) {
  pthread_mutex_lock(&globalData->filesMutex);
  auto result = globalData->files[fileIndex];
  pthread_mutex_unlock(&globalData->filesMutex);

  return result;
}
