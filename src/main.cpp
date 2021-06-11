#include "utils.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ir.hpp"
#include "irrunner.hpp"

#include <unistd.h>

#define USE_MMAP

#ifdef USE_MMAP
#include <errno.h>
#include <sys/mman.h>
#endif
#include <dlfcn.h>

int main() {
  initClock();
  size_t memory_size = 100 * 1024 * 1024;

#ifndef USE_MMAP
  void *memory = malloc(memory_size);
#else
  //NOTE: addr and length should be multiple of 4096 (page size on linux)
  void *memory = mmap((void *)0x12345789000, memory_size,
      PROT_READ|PROT_WRITE,
      MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,
      0, 0);
  if (memory == MAP_FAILED) {
    printf("Failed because: %s\n", strerror(errno));
    ASSERT(false, "failed to allocate via fixed place mmap");
  }
  printf("Memory: %p\n", memory);
#endif
  initAllocator(&global, (char *)memory, memory_size);

  printf("files cap = %d\n", files.cap);

  auto start = now();
  FileReadResult file = readFile("./examples/lexer_test.lan");
  if (!file.ok) {
    printf("failed to read file\n");
    return 1;
  }
  printf("readFile took %fsec\n", now() - start);
  printf("file size = %ld\n", file.content.len);

  start = now();
  Tokens tokens = tokenize(file.content, "abs/lexer_test.lan", "lexer_test.lan");
  if (!tokens.ok) {
    printf("failed to tokenize\n");
    return 1;
  }
  printf("tokenize took %fsec\n", now() - start);
  printf("memory usage %ld\n", usage(&global));

  start = now();
  {
    auto f = fopen("tokens.out", "w");
    debugPrintTokens(tokens, f);
    fclose(f);
  }
  printf("debugPrintTokens took %fsec\n", now() - start);

  start = now();
  Error error = {};
  auto f = parseFile(tokens, &error);
  if (!f) {
    printf("Failed to parse file\n");
    showError(error.site, error.message);
    showCodeLocation(error.site);
    return 1;
  }
  printf("parsing took %fsec\n", now() - start);
  printf("memory usage %ld\n", usage(&global));

  start = now();
  {
    auto file = fopen("ast.out", "w");
    debugPrintAST(f, file, 2, 0, 0);
    fclose(file);
  }
  printf("debugPrintAST took %fsec\n", now() - start);

  ir::Hub hub = {};
  ir::Builder builder = {};
  ir::Builder *b = &builder;
  b->h = &hub;

  {
    ir::Function *fn = b->function(STR("empty_fn"));
    b->jump(fn->init, fn->exit);
    b->retVoid(fn->exit);
  }

  ir::Function *add = NULL;
  {
    Array<Type*> args = {};
    append(&args, i32);
    append(&args, i32);
    add = b->function(STR("add"), args, i32);
    ir::BasicBlock *main = b->block(add, "main");
    b->jump(add->init, main);

    uint32_t result = b->iadd(main, add, 0, 1);
    b->jump(main, add->exit);

    b->ret(add->exit, result);

    #if 1
    double start = now();
    Array<Value> argValues = {};
    append(&argValues, Value{.i32 = 1});
    append(&argValues, Value{.i32 = 2});
    Value resultValue = runIR(add, argValues);
    ASSERT(resultValue.i32 == 3, "Wrong result");
    double end = now();

    printf("Result is %d (elapsed %f sec)\n", resultValue.i32, end-start);
    #endif
  }

  ir::Function *add3 = NULL;
  {
    Array<Type*> args = {};
    append(&args, i32);
    append(&args, i32);
    append(&args, i32);
    add3 = b->function(STR("add3"), args, i32);
    ir::BasicBlock *main = b->block(add3, "main");
    b->jump(add3->init, main);

    Array<uint32_t> argValues = {};
    append(&argValues, 0u);
    append(&argValues, 1u);

    //b->setArg(main, 0, 0);
    //b->setArg(main, 1, 1);
    uint32_t sum1 = b->call(main, add3, add, argValues);
    argValues = {};
    append(&argValues, sum1);
    append(&argValues, 2u);
    //b->setArg(main, 0, sum1);
    //b->setArg(main, 1, 2);
    uint32_t sum2 = b->call(main, add3, add, argValues);

    b->jump(main, add3->exit);

    b->ret(add3->exit, sum2);

    #if 1
    {
      double start = now();
      Array<Value> argValues = {};
      append(&argValues, Value{.i32 = 1});
      append(&argValues, Value{.i32 = 2});
      append(&argValues, Value{.i32 = 3});
      Value resultValue = runIR(add3, argValues);
      ASSERT(resultValue.i32 == 6, "Wrong result");
      double end = now();

      printf("Result is %d (elapsed %f sec)\n", resultValue.i32, end-start);
    }
    #endif
  }

  ir::Function *fnThatUsesPrintf = NULL;
  {
    ir::Function *irPrintf = NULL;
    Array<Type*> args = {};
    append(&args, u64);
    irPrintf = b->foreignFunction(STR("printf"), STR("libc"), args, i32, true);

    fnThatUsesPrintf = b->function(STR("fnThatUsesPrintf"), args, i32);
    {
      Array<uint32_t> argValues = {};
      append(&argValues, 0u);
      uint32_t retValue = b->call(fnThatUsesPrintf->init, fnThatUsesPrintf, irPrintf, argValues);
      b->jump(fnThatUsesPrintf->init, fnThatUsesPrintf->exit);
      b->ret(fnThatUsesPrintf->exit, retValue);
    }

  }

  ir::printAll(&hub, stdout);

  void *fptr = dlsym(RTLD_DEFAULT, "printf");
  printf("dlsym=%p native=%p\n", fptr, printf);

  {
    printf("before irPrintf\n");
    Array<Value> argValues = {};
    append(&argValues, Value{.u64 = (uint64_t)(const char *)"Hello world\n"});
    Value result = runIR(fnThatUsesPrintf, argValues);
    printf("after irPrintf n = %d\n", result.i32);
  }

  {
    printf("before irPrintf\n");
    Array<Value> argValues = {};
    append(&argValues, Value{.u64 = (uint64_t)(const char *)"Hello world\n"});
    Value result = runIR(fnThatUsesPrintf, argValues);
    printf("after irPrintf n = %d\n", result.i32);
  }


  /*{
    auto start = now();
    sleep(1);
    auto end = now();
    printf("Sleep %fsec\n", end - start);
  }*/

  return 0;
}
