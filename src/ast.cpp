#include "ast.h"

AST* setupASTNode(AST *node, ASTNodeType type, size_t size) {
  bzero(node, size);
  node->type = type;
  return node;
}

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


AST *astAssertCastImpl(AST *node, ASTNodeType wantedType) {
  if (node->type == wantedType) {
    return node;
  } else {
    abort();
  }
}


int priority(BinaryOp op) {
  switch (op) {
  case BINARY_OP_NOOP: abort(); break;

  case BINARY_OP_LOGICAL_OR: return -7;
  case BINARY_OP_LOGICAL_AND: return -6;

  case BINARY_OP_BITWISE_OR: return -5;
  case BINARY_OP_BITWISE_XOR: return -4;
  case BINARY_OP_BITWISE_AND: return -3;

  case BINARY_OP_EQUAL:
  case BINARY_OP_NOT_EQUAL:
    return -2;

  case BINARY_OP_LESS:
  case BINARY_OP_LESS_OR_EQUAL:
  case BINARY_OP_GREATER:
  case BINARY_OP_GREATER_OR_EQUAL:
    return -1;

  case BINARY_OP_BITWISE_SHIFT_LEFT:
  case BINARY_OP_BITWISE_SHIFT_RIGHT:
    return -0;

  case BINARY_OP_PLUS:
  case BINARY_OP_MINUS:
    return 1;

  case BINARY_OP_MULTIPLY:
  case BINARY_OP_DIVISION:
  case BINARY_OP_REMAINDER:
    return 2;
  }
}
