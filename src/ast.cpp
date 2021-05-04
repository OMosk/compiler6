#include "ast.hpp"

atomic_uint nodeCounter;
int allocationsCount[ASTMax];

const char *astTypeToStr(int kind) {
  switch (kind) {
  #define XX(TYPE) case TYPE: return #TYPE;
  AST_TYPES
  #undef XX
  }
  return "unknown";
}
