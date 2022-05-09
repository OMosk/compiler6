#pragma once

#include "core_types.h"

enum ASTFlags {
  AST_FLAGS_EXPR_IN_PAREN = 1 << 0,
};

enum UnaryOp {
  UNARY_OP_NOOP,
  UNARY_OP_PLUS,
  UNARY_OP_MINUS,
  UNARY_OP_DEREFERENCE,
  UNARY_OP_ADDRESSOF,
  UNARY_OP_LOGICAL_NEGATE,
  UNARY_OP_BITWISE_NEGATE,
};


enum BinaryOp {
  BINARY_OP_NOOP,

  BINARY_OP_PLUS,
  BINARY_OP_MINUS,

  BINARY_OP_MULTIPLY,
  BINARY_OP_DIVISION,
  BINARY_OP_REMAINDER,

  BINARY_OP_LOGICAL_AND,
  BINARY_OP_LOGICAL_OR,

  BINARY_OP_BITWISE_AND,
  BINARY_OP_BITWISE_OR,
  BINARY_OP_BITWISE_XOR,

  BINARY_OP_BITWISE_SHIFT_LEFT,
  BINARY_OP_BITWISE_SHIFT_RIGHT,

  BINARY_OP_LESS,
  BINARY_OP_LESS_OR_EQUAL,
  BINARY_OP_GREATER,
  BINARY_OP_GREATER_OR_EQUAL,
  BINARY_OP_EQUAL,
  BINARY_OP_NOT_EQUAL,
};

int priority(BinaryOp op);

struct ASTStruct;
struct ASTVar;
struct ASTBlock;

#define AST_NODES_LIST                                                         \
  XX(ASTIdentifier, { Str name; })                                             \
  XX(ASTUnaryOp, {                                                             \
    UnaryOp op;                                                                \
    AST *operand;                                                              \
  })                                                                           \
  XX(ASTBinaryOp, {                                                            \
    BinaryOp op;                                                               \
    AST *left;                                                                 \
    AST *right;                                                                \
  })                                                                           \
  XX(ASTCall, {                                                                \
    AST *callee;                                                               \
    Array<AST *> args;                                                         \
  })                                                                           \
  XX(ASTSubscript, {                                                           \
    AST *indexable;                                                            \
    AST *index;                                                                \
  })                                                                           \
  XX(ASTCast, {                                                                \
    AST *operand;                                                              \
    AST *toTypeExpr;                                                           \
  })                                                                           \
  XX(ASTMemberAccess, {                                                        \
    AST *structLike;                                                           \
    ASTIdentifier *field;                                                      \
  })                                                                           \
  XX(ASTNumberLiteral, { Str value; })                                         \
  XX(ASTStringLiteral, { Str value; })                                         \
  XX(ASTFile, { Array<AST *> topLevelDecls; })                                 \
  XX(ASTLoadDirective, { ASTStringLiteral *path; })                            \
  XX(ASTStruct, {                                                              \
    ASTIdentifier *name;                                                       \
    Array<ASTVar *> members;                                                   \
  })                                                                           \
  XX(ASTConst, {                                                               \
    ASTIdentifier *name;                                                       \
    AST *initExpr;                                                             \
    AST *parentScope;                                                          \
  })                                                                           \
  XX(ASTVar, {                                                                 \
    ASTIdentifier *name;                                                       \
    AST *typeExpr;                                                             \
    AST *initExpr;                                                             \
    AST *parentScope;                                                          \
  })                                                                           \
  XX(ASTFunction, {                                                            \
    ASTIdentifier *name;                                                       \
    Array<ASTVar *> args;                                                      \
    Array<ASTVar *> returns;                                                   \
    ASTBlock *body;                                                            \
  })                                                                           \
  XX(ASTBlock, { Array<AST *> statements; })                                   \
  XX(ASTVariableDefinition, {                                                  \
    Array<ASTIdentifier *> names;                                              \
    AST *typeExpr;                                                             \
    Array<AST *> initilizationValues;                                          \
  })                                                                           \
  XX(ASTIfStatement, {                                                         \
    AST *conditionExpr;                                                        \
    AST *thenStatement;                                                        \
    AST *elseStatement;                                                        \
  })                                                                           \
  XX(ASTWhileLoop, {                                                           \
    AST *condition;                                                            \
    AST *body;                                                                 \
  })                                                                           \
  XX(ASTDeferStatement, { AST *statement; })

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

AST *astAssertCastImpl(AST *node, ASTNodeType wantedType);
#define AST_ASSERT_CAST(TARGET_TYPE, NODE)                                     \
  static_cast<TARGET_TYPE *>(astAssertCastImpl(NODE, TARGET_TYPE##_))
