#include "testsystem.h"

#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

namespace {

const int testsMax = 1024;
TestFunction testFunctions[testsMax];
const char* testNames[testsMax];
int testsCount = 0;

//TestCase *tests[testsMax];
}


void T::Fail() {
  this->failed = true;
}

void registerTestFunction(TestFunction f, const char *name) {
  assert(testsCount < testsMax);

  testFunctions[testsCount] = f;
  testNames[testsCount] = name;

  testsCount++;
}

int runTests(int argc, char **argv) {
  bool verbose = false;
  if (getenv("VERBOSE")) {
    verbose = true;
  }
  bool haveFailures = false;
  bool haveRuns = false;

  const char *nameSubstr = NULL;
  if (argc > 1) {
    nameSubstr = argv[1];
  }

  for (int i = 0; i < testsCount; ++i) {
    T t = {};
    auto _this = &t;
    auto name = testNames[i];
    auto f = testFunctions[i];

    if (nameSubstr) {
      if (!strstr(name, nameSubstr)) {
        continue;
      }
    }

    _this->Printf("Test %s: Started\n", name);
    f(_this);
    haveRuns = true;

    if (_this->failed) {
      haveFailures = true;
      _this->Printf("Test %s: Failed\n", name);
    } else {
      _this->Printf("Test %s: OK\n", name);
    }

    fflush(_this->printFile);
    if (_this->failed || verbose) {
      fwrite(_this->printBuffer, _this->printBufferSize, 1, stdout);
      fflush(stdout);
    }

    fclose(_this->printFile);
    free(_this->printBuffer);
  }

  if (haveFailures) {
    fprintf(stdout, "Have test failures, exiting with status 1\n");
    return 1;
  } else {
    if (nameSubstr && !haveRuns) {
      fprintf(stdout, "Have name filter but no tests was matched, exiting with status 2\n");
      return 2;
    }

    return 0;
  }
}


int T::Printf(const char *fmt, ...) {
  if (!this->printFile) {
    this->printFile = open_memstream(&this->printBuffer, &this->printBufferSize);
  }

  va_list args;
  va_start(args, fmt);
  int len = vfprintf(this->printFile, fmt, args);
  va_end(args);
  return len;
}

// TestCase::TestCase(const char *testName) {
//   TestCase *testCase = this;
// 
//   assert(testsCount < testsMax);
// 
//   tests[testsCount] = testCase;
//   testNames[testsCount] = testName;
// 
//   testsCount++;
// }
