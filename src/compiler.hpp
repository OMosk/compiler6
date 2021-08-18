#pragma once

#include <pthread.h>
#include <inttypes.h>
#include "utils.hpp"
#include "ast.hpp"


struct Module; //Compiler

struct ReadFileJob {
  const char *filename;
  Module *module;
  Site *source;
};

struct ParsingJob {
  const char *filename;
  Str content;
  Module *module;
  Site *source;
};

struct TypecheckingFilePrepassJob {
  ASTFile *file;
  Module *module;
};

struct Job {
  Job *next;

  enum Kind { JOB_READ_FILE, JOB_PARSE_FILE, JOB_TYPECHECKING_FILE_PREPASS } kind;

  union {
    ReadFileJob read;
    ParsingJob parsing;
    TypecheckingFilePrepassJob typecheckingPrepass;
  };

};

struct WorkerGroup {
  pthread_mutex_t mutex;
  pthread_cond_t cv;

  Job head;

  Job *tail;

  Job freelist;
};

void runJobs(WorkerGroup *group);
void handleJob(Job *job);

WorkerGroup *startWorkerGroup(int n = 1);
void postJob(WorkerGroup *group, Job job);

extern WorkerGroup *ioWorkerGroup;
extern WorkerGroup *tokenizationParsingWorkerGroup;
extern WorkerGroup *typecheckingWorkerGroup;

struct Module {
  Str name;
  Array<Str> absoluteFilenames;
};
void addFileToModule(Module *module, const char *curDirectory,  const char *filename, Site *source = NULL);


struct NamedEntry {
  Str name;
  //ASTNode *node;
};


struct Names{
  Array<NamedEntry> entries;
};

//void registerName(Names *ns, Str name, ASTNode *node);

//struct ASTModule: ASTNode {
//  Names names;
//};


extern Names builtin;
void initBuiltinTypes();

enum {
  TYPE_PRIMITIVE           = (1 << 0),
  TYPE_FLAG_INT            = (1 << 1),
  TYPE_FLAG_UNSIGNED       = (1 << 2),
  TYPE_FLAG_FLOATING_POINT = (1 << 3),
  TYPE_FLAG_POINTER        = (1 << 4),
};

struct Type {
  uint32_t flags;
  uint16_t size; //bytes
  uint16_t alignment; //bytes

  Type *pointerToType;
};

extern Type *i8;
extern Type *i16;
extern Type *i32;
extern Type *i64;

extern Type *u8;
extern Type *u16;
extern Type *u32;
extern Type *u64;

extern Type *f32;
extern Type *f64;

Type *getPointerType(Type *pointee);
