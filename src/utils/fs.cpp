#include "fs.h"

#include <string.h>
#include <stdio.h>

FileReadResult readFile(Allocator *a, const char *filename) {
  FileReadResult result = {};
  FILE *f = fopen(filename, "r");
  if (!f) {
    result.ok = false;
    return result;
  }
  fseek(f, 0, SEEK_END);
  result.content.len = ftell(f);
  fseek(f, 0, SEEK_SET);

  result.content.data = ALLOC_ARRAY(char, result.content.len, a);
  memset(result.content.data, 0, result.content.len);
  fread(result.content.data, result.content.len, 1, f);
  fclose(f);

  result.ok = true;

  return result;
}
