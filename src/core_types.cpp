#include "core_types.h"


FileEntry file(GlobalData *globalData, int fileIndex) {
  pthread_mutex_lock(&globalData->filesMutex);
  auto result = globalData->files[fileIndex];
  pthread_mutex_unlock(&globalData->filesMutex);

  return result;
}
