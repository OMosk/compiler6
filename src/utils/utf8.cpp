#include "utf8.h"

#include <stdlib.h>

int32_t utf8advance(char **it, char *end) {
  if (*it >= end) return 0;
  if (**(unsigned char**)it > 127) {
    //TODO: handle multibyte
    abort();
  }
  int32_t result = **it;
  ++(*it);
  return result;
}
