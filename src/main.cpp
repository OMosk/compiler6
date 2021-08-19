#include "utils.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ir.hpp"
#include "irrunner.hpp"
#include "compiler.hpp"

#include <unistd.h>
#include <limits.h>

#define USE_MMAP

#ifdef USE_MMAP
#include <errno.h>
#include <sys/mman.h>
#endif
#include <dlfcn.h>

int bootstrapTest();

int main(int argc, char **argv) {
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
  initClock();

  printf("files cap = %d\n", files.cap);


  #if 0
  WorkerGroup *wg = startWorkerGroup(0);

  ioWorkerGroup = wg;
  tokenizationParsingWorkerGroup = wg;
  typecheckingWorkerGroup = wg;


  Module mainModule = {};
  mainModule.name = STR("main");

  char cwd[PATH_MAX + 1];
  if (!getcwd(cwd, PATH_MAX + 1)) {
    printf("getcwd failed: %s\n", strerror(errno));
    abort();
  }

  //start process
  addFileToModule(&mainModule, cwd, "./examples/lexer_test.lan");

  runJobs(wg);
  #else

  bootstrapTest();
  #endif

  return 0;
}

int bootstrapTest() {
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

  IRHub hub = {};

  {
    IRFunction *fn = createIRFunction(&hub, STR("empty_fn"));
    irInstRetVoid(fn, fn->init);
    runIR(fn, {});
  }

  IRFunction *add = NULL;
  {
    Array<Type*> args = {};
    append(&args, i32);
    append(&args, i32);
    add = createIRFunction(&hub, STR("add"), args, i32);
    IRBasicBlockIndex main = createIRBlock(add, "main");
    irInstJump(add, add->init, main);

    uint32_t result = irInstIAdd(add, main, 0, 1);
    irInstRet(add, main, result);

    #if 1
    double start = now();
    Array<IRValue> argValues = {};
    append(&argValues, IRValue{.i32 = 1});
    append(&argValues, IRValue{.i32 = 2});
    IRValue resultValue = runIR(add, argValues);
    ASSERT(resultValue.i32 == 3, "Wrong result");
    double end = now();

    printf("Result is %d (elapsed %f sec)\n", resultValue.i32, end-start);
    #endif
  }

  IRFunction *add3 = NULL;
  {
    Array<Type*> args = {};
    append(&args, i32);
    append(&args, i32);
    append(&args, i32);
    add3 = createIRFunction(&hub, STR("add3"), args, i32);
    IRBasicBlockIndex main = createIRBlock(add3, "main");
    irInstJump(add3, add3->init, main);

    Array<uint32_t> argValues = {};
    append(&argValues, 0u);
    append(&argValues, 1u);

    //setArg(main, 0, 0);
    //setArg(main, 1, 1);
    uint32_t sum1 = irInstCall(add3, main, add, argValues);
    argValues = {};
    append(&argValues, sum1);
    append(&argValues, 2u);
    //setArg(main, 0, sum1);
    //setArg(main, 1, 2);
    uint32_t sum2 = irInstCall(add3, main, add, argValues);

    irInstRet(add3, main, sum2);

    #if 1
    {
      double start = now();
      Array<IRValue> argValues = {};
      append(&argValues, IRValue{.i32 = 1});
      append(&argValues, IRValue{.i32 = 2});
      append(&argValues, IRValue{.i32 = 3});
      IRValue resultValue = runIR(add3, argValues);
      ASSERT(resultValue.i32 == 6, "Wrong result");
      double end = now();

      printf("Result is %d (elapsed %f sec)\n", resultValue.i32, end-start);
    }
    #endif
  }

  IRFunction *fnThatUsesPrintf = NULL;
  {
    IRFunction *irPrintf = NULL;
    Array<Type*> args = {};
    append(&args, u64);
    irPrintf = createIRForeignFunction(&hub, STR("printf"), STR("libc"), args, i32, true);

    fnThatUsesPrintf = createIRFunction(&hub, STR("fnThatUsesPrintf"), args, i32);
    {
      Array<uint32_t> argValues = {};
      append(&argValues, 0u);
      uint32_t retValue = irInstCall(fnThatUsesPrintf, fnThatUsesPrintf->init, irPrintf, argValues);
      irInstRet(fnThatUsesPrintf, fnThatUsesPrintf->init, retValue);
    }

  }

  IRFunction *fWithConstants = NULL;
  {
    IRFunction *f = createIRFunction(&hub, STR("constantTest"), {}, i32);
    uint32_t valueIndex = irInstConstant(f, f->init, i32, valueI32(12345));
    irInstConstant(f, f->init, i64, valueI64(-1));
    irInstConstant(f, f->init, u64, valueU64((uint64_t)-1));
    irInstConstant(f, f->init, f32, valueF32(3.14));
    irInstConstant(f, f->init, f64, valueF64(6.28));
    irInstRet(f, f->init, valueIndex);

    fWithConstants = f;
  }

  {
    IRFunction *f = createIRFunction(&hub, STR("allocaTest"), {}, u64);
    uint32_t address = irInstAlloca(f, f->init, u64);
    uint32_t constant = irInstConstant(f, f->init, u64, valueU64(12345678));
    irInstStore(f, f->init, address, constant);
    uint32_t l = irInstLoad(f, f->init, u64, address);
    irInstRet(f, f->init, l);
    IRValue value = runIR(f, {});
    ASSERT(value.u64 == 12345678, "Alloca/store/load fail");
    printf("Alloca, store, load passed\n");
  }

  printAll(&hub, stdout);

  void *fptr = dlsym(RTLD_DEFAULT, "printf");
  printf("dlsym=%p native=%p\n", fptr, printf);

  {
    printf("before irPrintf\n");
    Array<IRValue> argValues = {};
    append(&argValues, IRValue{.u64 = (uint64_t)(const char *)"Hello world\n"});
    IRValue result = runIR(fnThatUsesPrintf, argValues);
    printf("after irPrintf n = %d\n", result.i32);
  }

  {
    printf("before irPrintf\n");
    Array<IRValue> argValues = {};
    append(&argValues, IRValue{.u64 = (uint64_t)(const char *)"Hello world\n"});
    IRValue result = runIR(fnThatUsesPrintf, argValues);
    printf("after irPrintf n = %d\n", result.i32);
  }

  {
    IRValue result = runIR(fWithConstants, {});
    printf("constant returned %d\n", result.i32);
  }


  return 0;
}
