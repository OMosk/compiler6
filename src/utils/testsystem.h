#pragma once

#include <stdio.h>

struct T;
typedef void (*TestFunction)(T *t);
struct T {
  bool failed;

  FILE *printFile;
  char *printBuffer;
  size_t printBufferSize;

  void Fail();

  __attribute__((__format__ (__printf__, 2, 3)))
  int Printf(const char *fmt, ...);
};

void registerTestFunction(TestFunction f, const char *name);
int runTests(int argc, char **argv);

#define TEST(TEST_CASE_NAME) \
void TEST_CASE_NAME(T *t);\
__attribute__((constructor)) void TEST_CASE_NAME ## _register_function() { \
  registerTestFunction(TEST_CASE_NAME, #TEST_CASE_NAME); \
} \
void TEST_CASE_NAME
