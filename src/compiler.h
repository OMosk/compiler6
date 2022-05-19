#pragma once
#include "core_types.h"

enum CompilerJobType {
  COMPILER_JOB_TYPE_READ_FILE,
  COMPILER_JOB_TYPE_PARSE,

  COMPILER_JOB_TYPE_EXIT,
};

struct CompilerJob {
  CompilerJobType type;

  //READ_FILE
  Str fileNameToRead;

  //PARSE
  FileEntry fileEntry;

  //EXIT
  int status;

  CompilerJob *next;
};

struct Compiler {
  GlobalData globalData;
  ThreadData threadsData[64];
  int threads;

  void *memory;
  size_t memorySize;

  Allocator mainAllocator;

  CompilerJob *jobFreelistNext;
  CompilerJob *jobQueueHead;
  CompilerJob *jobQueueTail;
  bool jobQueueShouldContinue;
  bool compilerFinished;
  int exitStatus;
  pthread_mutex_t jobQueueMutex;
  pthread_cond_t jobQueueCond;
};

void initCompiler(Compiler *compiler, int threads, const char *entryPoint);
void deinitCompiler(Compiler *compiler);
CompilerJob *allocOrReuseCompilerJob(Compiler *compiler);
void postCompilerJob(Compiler *compiler, CompilerJob *job);
int waitForCompilerToFinish(Compiler *compiler);
void executeJob(ThreadData *td, CompilerJob *job);

