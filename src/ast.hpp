#pragma once

#include <stdatomic.h>
#include "utils.hpp"

extern atomic_uint nodeCounter;

#define AST_TYPES \
  XX(ASTToken_) \
  XX(ASTSeq_) \
  XX(ASTAlt_) \
  XX(ASTLiteral_) \
  XX(ASTIdentifier_) \
  XX(ASTSubscript_) \
  XX(ASTCall_) \
  XX(ASTMemberAccess_) \
  XX(ASTCast_) \
  XX(ASTUnaryOp_) \
  XX(ASTBinaryOp_) \
  XX(ASTNamedConstant_) \
  XX(ASTSuperGlobal_) \
  XX(ASTFile_) \
  XX(ASTProcArgument_) \
  XX(ASTBlock_) \
  XX(ASTReturn_) \
  XX(ASTProc_) \
  XX(ASTVariable_) \
  XX(ASTVariableDefinition_) \
  XX(ASTAssignment_) \
  XX(ASTPointerTo_) \
  XX(ASTModule_) \
  XX(ASTTypeInfo_) \
  XX(ASTIf_) \
  XX(ASTWhile_) \
  XX(ASTContinue_) \
  XX(ASTBreak_) \
  XX(ASTArray_) \
  XX(ASTStruct_) \
  XX(ASTStructMember_) \
  XX(ASTNil_) \
  XX(ASTDefer_) \
  XX(ASTExportScopeDirective_) \
  XX(ASTModuleScopeDirective_) \
  XX(ASTLoad_) \
  XX(ASTAggregateInitializer_)

#define XX(X) X,
enum {
  AST_NULL_INITED,

  AST_TYPES

  ASTMax
};
#undef XX
extern int allocationsCount[];

enum {
  FlagsProcForeign = 1 << 0,
  FlagsProcVariadic = 1 << 1,
  FlagsExprInParentheses = 1 << 2,
};

struct ASTNode {
  uint32_t id;
  uint32_t flags;
  Site site;
  uint32_t kind;
};

inline ASTNode * init(void *memory, size_t size, int kind, Site site) {
  memset(memory, 0, size);
  ASTNode *node = (ASTNode *) memory;
  node->id = atomic_fetch_add_explicit(&nodeCounter, 1, memory_order_relaxed);
  node->kind = kind;
  node->site = site;
  return node;
}

#define ASTALLOC(NODE, I) \
  ((NODE *) init(ALLOC(NODE), sizeof(NODE), NODE ## _, SITE(I)))

#define ASTALLOC2(NODE, site) \
  ((NODE *) init(ALLOC(NODE), sizeof(NODE), NODE ## _, site))

struct ASTIdentifier: ASTNode {
  Str name;
};

struct ASTFile: ASTNode {
  Array<ASTNode*> declarations;
  uint32_t fileIndex;
};

struct ASTProcArgument: ASTNode {
  Str name;
  ASTNode *type;
};

struct ASTBlock: ASTNode {
  Array<ASTNode *> statements;
};

struct ASTProc: ASTNode {
  Array<ASTProcArgument *> arguments;
  Array<ASTNode *> returnTypes;
  Str name;
  ASTBlock *body;
};

struct ASTPointerTo: ASTNode {
  ASTNode *pointsTo;
};

struct ASTVariable: ASTNode {
  Str name;
  ASTNode *type;
  ASTNode *initExpr;
};

struct ASTVariableDefinition: ASTNode {
  Array<ASTVariable*> vars;
};

struct ASTUnaryOp: ASTNode {
  ASTNode *operand;
  int16_t op;
};

enum {
  LiteralKindNumber       = 1 << 0,
  LiteralKindIntNumber    = 1 << 1,
  LiteralKindFloatNumber  = 1 << 2,

  LiteralKindString       = 1 << 3,
};

struct ASTLiteral: ASTNode {
  Str raw;
  union {
    double f64;
    int64_t i64;
  };
  uint8_t literalKind;
};

struct ASTNil: ASTNode {
};

struct ASTCast: ASTNode {
  ASTNode *operand;
  ASTNode *type;
};

struct ASTMemberAccess: ASTNode {
  ASTNode *container;
  Str name;
};

struct ASTSubscript: ASTNode {
  ASTNode *container;
  ASTNode *index;
};

struct ASTBinaryOp: ASTNode {
  ASTNode *lhs, *rhs;
  int16_t op;
};

struct ASTCall: ASTNode {
  ASTNode *callee;
  Array<ASTNode *> args;
};

struct ASTIf: ASTNode {
  ASTNode *cond, *thenStmt, *elseStmt;
};

struct ASTAssignment: ASTNode {
  Array<ASTNode *> lhs;
  Array<ASTNode *> rhs;
};

struct ASTDefer: ASTNode {
  ASTNode *stmt;
};

struct ASTWhile: ASTNode {
  ASTNode *cond;
  ASTNode *body;
};

const char *astTypeToStr(int kind);
