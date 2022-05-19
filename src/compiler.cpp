#include "compiler.h"

#include <stdio.h>

#include <errno.h>
#include <sys/mman.h>

void *compilerThreadProc(void *arg);

void initCompiler(Compiler *compiler, int threads, const char *entryPoint) {
  *compiler = {};
  initGlobalData(&compiler->globalData);
  compiler->globalData.compiler = compiler;
  compiler->threads = threads;

  compiler->memorySize = 1ull * 1024ull * 1024ull * 1024ull;
  size_t memoryPerThread = 100ull * 1024ull;

  //NOTE: addr and length should be multiple of 4096 (page size on linux)
  if (compiler->memorySize % 4096) {
    fprintf(stderr, "%s:%d Memory size is not multiple of 4096\n",
      __FILE__, __LINE__);
    abort();
  }
  compiler->memory = mmap((void *)0x12345789000, compiler->memorySize,
      PROT_READ|PROT_WRITE,
      MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,
      0, 0);
  if (compiler->memory == MAP_FAILED) {
    fprintf(stderr, "%s:%d Failed to mmap memory because: %s\n",
      __FILE__, __LINE__,
      strerror(errno));
    abort();
  }

  pthread_mutex_init(&compiler->jobQueueMutex, NULL);
  pthread_cond_init(&compiler->jobQueueCond, NULL);

  initAllocator(&compiler->mainAllocator, (char *)compiler->memory, compiler->memorySize);

  compiler->jobQueueShouldContinue = true;

  for (int i = 0; i < threads; ++i) {
    auto memory = ALLOC_ARRAY(char, memoryPerThread, &compiler->mainAllocator);
    initThreadData(compiler->threadsData + i, &compiler->globalData, memory, memoryPerThread);

    pthread_t t = {};
    pthread_create(&t, NULL, compilerThreadProc, compiler->threadsData + i);
    pthread_detach(t);
  }

  auto job = allocOrReuseCompilerJob(compiler);
  job->type = COMPILER_JOB_TYPE_READ_FILE;
  job->fileNameToRead = CStringToStr(entryPoint);
  postCompilerJob(compiler, job);
}

void deinitCompiler(Compiler *compiler) {
  munmap(compiler->memory, compiler->memorySize);

  pthread_mutex_destroy(&compiler->jobQueueMutex);
  pthread_cond_destroy(&compiler->jobQueueCond);
}

CompilerJob *allocOrReuseCompilerJob(Compiler *compiler) {
  pthread_mutex_lock(&compiler->jobQueueMutex);

  if (!compiler->jobFreelistNext) {
    size_t n = 100;
    auto newJobs = ALLOC_ARRAY(CompilerJob, n, &compiler->mainAllocator);
    for (int i = 0; i < n; ++i) {
      newJobs[i].next = compiler->jobFreelistNext;
      compiler->jobFreelistNext = newJobs + i;
    }
  }

  auto result = compiler->jobFreelistNext;
  compiler->jobFreelistNext = result->next;

  pthread_mutex_unlock(&compiler->jobQueueMutex);

  *result = {};
  return result;
}

void postCompilerJob(Compiler *compiler, CompilerJob *job) {
  pthread_mutex_lock(&compiler->jobQueueMutex);

  if (!compiler->jobQueueHead) {
    compiler->jobQueueHead = job;
  }
  if (compiler->jobQueueTail) {
    compiler->jobQueueTail->next = job;
  }
  compiler->jobQueueTail = job;

  pthread_cond_signal(&compiler->jobQueueCond);

  pthread_mutex_unlock(&compiler->jobQueueMutex);
}


void *compilerThreadProc(void *arg) {
  auto td = static_cast<ThreadData *>(arg);
  auto compiler = td->globalData->compiler;

  pthread_mutex_lock(&compiler->jobQueueMutex);
  for (;;) {
    while (compiler->jobQueueHead == NULL && compiler->jobQueueShouldContinue) {
      pthread_cond_wait(&compiler->jobQueueCond, &compiler->jobQueueMutex);
    }
    if (!compiler->jobQueueShouldContinue) break;

    auto job = compiler->jobQueueHead;
    compiler->jobQueueHead = job->next;

    pthread_mutex_unlock(&compiler->jobQueueMutex);
    executeJob(td, job);
    pthread_mutex_lock(&compiler->jobQueueMutex);

    job->next = compiler->jobFreelistNext;
    compiler->jobFreelistNext = job;
  }
  pthread_mutex_unlock(&compiler->jobQueueMutex);

  return NULL;
}

int waitForCompilerToFinish(Compiler *compiler) {
  pthread_mutex_lock(&compiler->jobQueueMutex);
  while (!compiler->compilerFinished) {
    pthread_cond_wait(&compiler->jobQueueCond, &compiler->jobQueueMutex);
  }
  auto result = compiler->exitStatus;
  pthread_mutex_unlock(&compiler->jobQueueMutex);
  return result;
}

void executeJob(ThreadData *td, CompilerJob *job) {
  switch (job->type) {
  case COMPILER_JOB_TYPE_READ_FILE: {
    //TODO: continue here

    { //TODO Move exit code
      auto newJob = allocOrReuseCompilerJob(td->globalData->compiler);
      newJob->type = COMPILER_JOB_TYPE_EXIT;
      postCompilerJob(td->globalData->compiler, newJob);
    }
  } break;
  case COMPILER_JOB_TYPE_PARSE: {
    //TODO: continue here
  } break;
  case COMPILER_JOB_TYPE_EXIT: {
    td->globalData->compiler->compilerFinished = true;
    td->globalData->compiler->jobQueueShouldContinue = false;
    pthread_cond_broadcast(&td->globalData->compiler->jobQueueCond);
  } break;
  default: abort();
  }
}
