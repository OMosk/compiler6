#include "tests/lexer.cpp"
#include "tests/ast.cpp"

const char *toString(ASTNodeType type) {
  switch (type) {
  #define XX(NAME, STRUCT) case NAME ## _: return #NAME;
  AST_NODES_LIST
  #undef XX
  default: return "unknown";
  } 
}

AST *astSafeCastImpl(AST *node, ASTNodeType wantedType) {
  if (node->type == wantedType) {
    return node;
  } else {
    return NULL;
  }
}
