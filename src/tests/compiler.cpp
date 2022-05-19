#include "../all.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

TEST(CompilerSmokeTest) (T *t) {
  Compiler compiler;
  const char *entry = ".unittest.c6";

  auto fd = open(entry, O_TRUNC | O_WRONLY);
  auto content = STR("main :: func() { print(\"Hello world\\n\"); }");
  write(fd, content.data, content.len);
  close(fd);


  initCompiler(&compiler, 1, entry);
  auto status = waitForCompilerToFinish(&compiler);
  deinitCompiler(&compiler);

  unlink(entry);
  if (status != 0) FAILF("Unexpected exit status: %d\n", status);
}
