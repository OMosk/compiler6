#include "compiler.hpp"

#include "utils.hpp"
#include "lexer.hpp"
#include "parser.hpp"

WorkerGroup *ioWorkerGroup;
WorkerGroup *tokenizationParsingWorkerGroup;
WorkerGroup *typecheckingWorkerGroup;

void runJobs(WorkerGroup *group) {
  pthread_mutex_lock(&group->mutex);
  for (;;) {
    if (group->head.next) {

      Job *job = group->head.next;
      group->head.next = job->next;
      if (group->head.next == NULL) { // Queue is empty
        group->tail = &group->head;
      }
      pthread_mutex_unlock(&group->mutex);

      handleJob(job);

      pthread_mutex_lock(&group->mutex);
      job->next = group->freelist.next;
      group->freelist.next = job;
      //Leave mutex locked for next iteration
      //pthread_mutex_unlock(&group->mutex);

    } else {
      pthread_cond_wait(&group->cv, &group->mutex);
    }
  }
}

void *workerGroupThreadProc(void *arg) {
  runJobs((WorkerGroup *) arg);
  return NULL;
}


WorkerGroup *startWorkerGroup(int n) {
  WorkerGroup *result = ALLOC(WorkerGroup);
  *result = {};
  result->tail = &result->head;

  pthread_mutex_init(&result->mutex, NULL);
  pthread_cond_init(&result->cv, NULL);

  for (int i = 0; i < n; ++i) {
    pthread_t thread;
    //TODO error handling
    pthread_create(&thread, NULL, workerGroupThreadProc, result);
    pthread_detach(thread);
  }

  return result;
}

void handleReadFileJob(ReadFileJob *job) {
  printf("Got io job for %s\n", job->filename);
  FileReadResult result = readFile(job->filename);
  if (!result.ok) {
    if (job->source) {
      showError(*job->source, "failed to read file: %s", job->filename);
    } else {
      //TODO: colored output if necessary?
      printf("failed to read file: %s\n", job->filename);
    }
    exit(1);
  }

  Job parsingJob = {};

  parsingJob.kind = Job::JOB_PARSE_FILE;
  parsingJob.parsing.filename = job->filename;
  parsingJob.parsing.content = result.content;
  parsingJob.parsing.module = job->module;
  parsingJob.parsing.source = job->source;

  postJob(tokenizationParsingWorkerGroup, parsingJob);
}

void handleParsingJob(ParsingJob *job) {
  printf("Got parsing job for %s\n", job->filename);

  Tokens tokens = tokenize(job->content, job->filename, job->filename);
  if (!tokens.ok) {
    //TODO: Error reporting data race
    exit(1);
  }

  Error error = {};
  ASTFile *astFile = parseFile(tokens, &error);
  if (!astFile) {
    //TODO: Error reporting data race
    showError(error.site, error.message);
    showCodeLocation(error.site);
    exit(1);
  }

  Job prepass = {};
  prepass.kind = Job::JOB_TYPECHECKING_FILE_PREPASS;
  prepass.typecheckingPrepass.file = astFile;
  prepass.typecheckingPrepass.module = job->module;

  postJob(typecheckingWorkerGroup, prepass);
}

void handleFilePrepassJob(TypecheckingFilePrepassJob *job) {
  printf("Got typecheck prepass job for %s\n", files[job->file->fileIndex].absolute_path);
}

void handleJob(Job *job) {
  switch (job->kind) {

  case Job::JOB_READ_FILE: {
    handleReadFileJob(&job->read);
  } break;

  case Job::JOB_PARSE_FILE: {
    handleParsingJob(&job->parsing);
  } break;

  case Job::JOB_TYPECHECKING_FILE_PREPASS: {
    handleFilePrepassJob(&job->typecheckingPrepass);
  } break;

  default: {
    NOT_IMPLEMENTED;
  }

  }
}


void postJob(WorkerGroup *group, Job copy) {
  pthread_mutex_lock(&group->mutex);
  Job *job = group->freelist.next;
  if (job) {
    group->freelist.next = job->next;
  } else {
    job = ALLOC(Job);
  }

  *job = copy;
  job->next = NULL;

  group->tail->next = job;
  group->tail = job;

  pthread_cond_signal(&group->cv);
  pthread_mutex_unlock(&group->mutex);
}

#define DECLARE_TYPE(NAME, FLAGS, SIZE, ALIGNMENT) \
  static Type NAME ## _ = (Type){.flags = (FLAGS), .size = (SIZE), .alignment = (ALIGNMENT)};\
  Type *NAME = &NAME ## _
//Type *u8 = &(Type){.flags = TYPE_FLAG_INT};

DECLARE_TYPE(u8,  TYPE_PRIMITIVE|TYPE_FLAG_INT, 1, 1);
DECLARE_TYPE(u16, TYPE_PRIMITIVE|TYPE_FLAG_INT, 2, 2);
DECLARE_TYPE(u32, TYPE_PRIMITIVE|TYPE_FLAG_INT, 4, 4);
DECLARE_TYPE(u64, TYPE_PRIMITIVE|TYPE_FLAG_INT, 8, 8);

DECLARE_TYPE(i8,  TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 1, 1);
DECLARE_TYPE(i16, TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 2, 2);
DECLARE_TYPE(i32, TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 4, 4);
DECLARE_TYPE(i64, TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 8, 8);

DECLARE_TYPE(f32, TYPE_PRIMITIVE|TYPE_FLAG_FLOATING_POINT, 4, 4);
DECLARE_TYPE(f64, TYPE_PRIMITIVE|TYPE_FLAG_FLOATING_POINT, 8, 8);

#undef DECLARE_TYPE


Names builtin;

void initBuiltinTypes() {
  //registerName(&builtin, STR("u8"), )
}

Array<Type *> pointeeTypes;
Array<Type *> pointerTypes;

Type *getPointerType_(Type *pointee) {
  for (int i = 0; i < pointeeTypes.len; ++i) {
    if (pointeeTypes[i] == pointee) {
      return pointerTypes[i];
    }
  }

  Type *result = ALLOC(Type);

  *result = {};
  result->flags = TYPE_PRIMITIVE | TYPE_FLAG_POINTER;
  result->size = 8;
  result->alignment = 8;
  result->pointerToType = pointee;
  append(&pointeeTypes, pointee);
  append(&pointerTypes, result);
  return result;
}

pthread_mutex_t pointerTypesMutex = PTHREAD_MUTEX_INITIALIZER;

Type *getPointerType(Type *pointee) {

  pthread_mutex_lock(&pointerTypesMutex);

  Type *result = getPointerType_(pointee);

  pthread_mutex_unlock(&pointerTypesMutex);

  return result;
}


void addFileToModule(Module *module, const char *curDirectory,  const char *filename, Site *source) {
  //TODO: @ThreadSafety
  Str fullPath = concatAndNormalizePath(curDirectory, filename);
  bool found = false;

  //TODO: @HashTable
  for (int i = 0; i < module->absoluteFilenames.len; ++i) {
    if (module->absoluteFilenames[i].eq(fullPath)) {
      found = true;
      break;
    }
  }

  if (!found) {
    Job job = {};
    job.kind = Job::JOB_READ_FILE;
    job.read.filename = fullPath.data;
    job.read.module = module;
    job.read.source = source;

    postJob(ioWorkerGroup, job);
  }
}
