#include "utils.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <unistd.h>

#define USE_MMAP

#ifdef USE_MMAP
#include <errno.h>
#include <sys/mman.h>
#endif

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

  Error error = {};
  auto f = parseFile(tokens, &error);
  if (!f) {
    printf("Failed to parse file\n");
    showError(error.site, error.message);
    showCodeLocation(error.site);
    return 1;
  }

  start = now();
  debugPrintAST(f, stdout, 2, 0, 0);
  printf("debugPrintAST took %fsec\n", now() - start);

  /*{
    auto start = now();
    sleep(1);
    auto end = now();
    printf("Sleep %fsec\n", end - start);
  }*/

  return 0;
}
