#pragma once

#include "core_types.h"

#define AST_NODES_LIST                                                         \
  XX(ASTIdentifier, { Str name; })                                             \
  XX(ASTStringLiteral, { Str value; })

enum ASTNodeType {
#define XX(NAME, STRUCT) NAME##_,
  AST_NODES_LIST
#undef XX
};

const char *toString(ASTNodeType type);

struct AST {
  ASTNodeType type;
  uint32_t fileIndex, offset0, offset1;
  uint32_t flags;
  uint32_t debugId;
};

#define XX(NAME, STRUCT) struct NAME : public AST STRUCT;
AST_NODES_LIST
#undef XX

AST *setupASTNode(AST *node, ASTNodeType type, size_t size);
#define AST_ALLOC(NODE_TYPE, ALLOCATOR)                                        \
  (static_cast<NODE_TYPE *>(setupASTNode(ALLOC(NODE_TYPE, ALLOCATOR),          \
                                         NODE_TYPE##_, sizeof(NODE_TYPE))))

AST *astSafeCastImpl(AST *node, ASTNodeType wantedType);
#define AST_CAST(TARGET_TYPE, NODE)                                            \
  static_cast<TARGET_TYPE *>(astSafeCastImpl(NODE, TARGET_TYPE##_))
