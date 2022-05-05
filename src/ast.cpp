#include "ast.h"

#include <stdatomic.h>

atomic_uint nextASTNodeDebugId;

AST* setupASTNode(AST *node, ASTNodeType type, size_t size) {
  bzero(node, size);
  node->type = type;
  node->debugId = atomic_fetch_add(&nextASTNodeDebugId, 1);
  return node;
}
