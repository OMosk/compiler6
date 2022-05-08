#include "../all.h"

struct TestSetupData {
  Lexer lexer;
  ThreadData threadData;
  GlobalData globalData;
};

TestSetupData* setupTestData(Str content) {
  auto result = static_cast<TestSetupData *>(malloc(sizeof(TestSetupData)));
  *result = {};
  result->threadData.globalData = &result->globalData;
  size_t size = 10 * 1024;
  initAllocator(&result->threadData.allocator, (char *)malloc(size), size);

  auto fileEntry = FileEntry{
    .absolutePath = STR("/main.c6"),
    .relativePath = STR("main.c6"),
    .content = content,
    .index = 0,
  };

  append(&result->globalData.files, fileEntry, &result->threadData.allocator);

  result->lexer.fileIndex = 0;
  result->lexer.source = content;

  return result;
}

TEST(ParsingIdentifier) (T *t) {
  auto setup = setupTestData(STR("MyStruct"));

  ParsingError error = {};
  AST *ast = parseIdentifier(&setup->threadData, &setup->lexer, 0, &error);
  ASTIdentifier *ident = AST_CAST(ASTIdentifier, ast);
  if (ident == NULL) {
    t->Printf("ident is NULL");
    t->Fail();
    return;
  }

  if (!StrEqual(ident->name, STR("MyStruct"))) {
    t->Printf("identifier names does not match");
    t->Fail();
    return;
  }
}

TEST(ParsingUnaryExpr) (T *t) {
  auto setup = setupTestData(STR("1"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto literal = AST_CAST(ASTNumberLiteral, ast);
  if (!literal) {
    t->Printf("Expected number literal, got: %s\n", toString(ast->type));
    t->Fail();
    return;
  }
}

TEST(ParsingBinaryExpr) (T *t) {
  auto setup = setupTestData(STR("1 + 2"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);
  if (!binaryOp) {
    t->Printf("Expected binary op, got: %s\n", toString(ast->type));
    t->Printf("Offset: %u\n", setup->lexer.peek().offset0);
    t->Fail();
    return;
  }

  if (binaryOp->left->type != ASTNumberLiteral_) {
    t->Printf("Expected left operand to be a number literal, got: %s\n", toString(binaryOp->left->type));
    t->Fail();
    return;
  }

  if (binaryOp->right->type != ASTNumberLiteral_) {
    t->Printf("Expected right operand to be a number literal, got: %s\n", toString(binaryOp->right->type));
    t->Fail();
    return;
  }
}

TEST(ParsingComplexBinaryExpr) (T *t) {
  auto setup = setupTestData(STR("1 + 2 + 3"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);
  if (!binaryOp) {
    t->Printf("Expected binary op, got: %s\n", toString(ast->type));
    t->Printf("Offset: %u\n", setup->lexer.peek().offset0);
    t->Fail();
    return;
  }

  if (binaryOp->left->type != ASTBinaryOp_) {
    t->Printf("Expected left operand to be a %s, got: %s\n",
      toString(ASTBinaryOp_),
      toString(binaryOp->left->type));
    t->Fail();
    return;
  }

  if (binaryOp->right->type != ASTNumberLiteral_) {
    t->Printf("Expected right operand to be a %s, got: %s\n",
      toString(ASTNumberLiteral_),
      toString(binaryOp->right->type));
    t->Fail();
    return;
  }
}

TEST(ParsingPrioritiesInBinaryExpr) (T *t) {
  auto setup = setupTestData(STR("1 + 2 * 3"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);

  if (binaryOp->op != BINARY_OP_PLUS) {
    t->Printf("Wrong binary op, wanted %d, got: %d\n", BINARY_OP_PLUS, binaryOp->op);
    t->Fail();
  }

  auto right = AST_CAST(ASTBinaryOp, binaryOp->right);
  if (right->op != BINARY_OP_MULTIPLY) {
    t->Printf("Wrong binary op in right subtree, wanted %d, got: %d\n",
      BINARY_OP_MULTIPLY, right->op);
    t->Fail();
  }
}

TEST(ParsingPrioritiesInBinaryExpr2) (T *t) {
  auto setup = setupTestData(STR("1 * 2 + 3"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);

  if (binaryOp->op != BINARY_OP_PLUS) {
    t->Printf("Wrong binary op, wanted %d, got: %d\n",
      BINARY_OP_PLUS, binaryOp->op);
    return t->Fail();
  }

  auto left = AST_CAST(ASTBinaryOp, binaryOp->left);
  if (!left) {
    t->Printf("Non binary op in left subtree: %s\n",
      toString(binaryOp->left->type));
    return t->Fail();
  }

  if (left->op != BINARY_OP_MULTIPLY) {
    t->Printf("Wrong binary op in left subtree, wanted %d, got: %d\n",
      BINARY_OP_MULTIPLY, left->op);
    return t->Fail();
  }
}

TEST(ParsingPrioritiesInBinaryExpr3) (T *t) {
  auto setup = setupTestData(STR("1 * (2 + 3)"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);

  if (binaryOp->op != BINARY_OP_MULTIPLY) {
    t->Printf("Wrong binary op, wanted %d, got: %d\n",
      BINARY_OP_MULTIPLY, binaryOp->op);
    return t->Fail();
  }

  auto right = AST_CAST(ASTBinaryOp, binaryOp->right);
  if (!right) {
    t->Printf("Non binary op in right subtree: %s\n",
      toString(binaryOp->left->type));
    return t->Fail();
  }

  if (right->op != BINARY_OP_PLUS) {
    t->Printf("Wrong binary op in right subtree, wanted %d, got: %d\n",
      BINARY_OP_MULTIPLY, right->op);
    return t->Fail();
  }
}

TEST(ParsingPrioritiesInBinaryExpr4) (T *t) {
  auto setup = setupTestData(STR("(1 + 2) * 3"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);

  if (binaryOp->op != BINARY_OP_MULTIPLY) {
    t->Printf("Wrong binary op, wanted %d, got: %d\n", BINARY_OP_MULTIPLY, binaryOp->op);
    t->Fail();
  }

  auto left = AST_CAST(ASTBinaryOp, binaryOp->left);
  if (left->op != BINARY_OP_PLUS) {
    t->Printf("Wrong binary op in left subtree, wanted %d, got: %d\n",
      BINARY_OP_PLUS, left->op);
    t->Fail();
  }
  if (!(left->flags & AST_FLAGS_EXPR_IN_PAREN)) {
    t->Printf("Binary op in left subtree missing AST_FLAGS_EXPR_IN_PAREN flag");
    t->Fail();
  }
}

TEST(ParsingBinaryExprEvaluationOrder) (T *t) {
  auto setup = setupTestData(STR("5 / 4 * 3"));

  ParsingError error = {};
  AST *ast = parseExpr(&setup->threadData, &setup->lexer, 0, &error);
  auto binaryOp = AST_CAST(ASTBinaryOp, ast);

  if (binaryOp->op != BINARY_OP_MULTIPLY) {
    t->Printf("Wrong binary op, wanted %d, got: %d\n",
      BINARY_OP_MULTIPLY, binaryOp->op);
    return t->Fail();
  }

  auto left = AST_CAST(ASTBinaryOp, binaryOp->left);
  if (!left) {
    t->Printf("Non binary op in left subtree: %s\n",
      toString(binaryOp->left->type));
    return t->Fail();
  }

  if (left->op != BINARY_OP_DIVISION) {
    t->Printf("Wrong binary op in right subtree, wanted %d, got: %d\n",
      BINARY_OP_DIVISION, left->op);
    return t->Fail();
  }
}

TEST(ParsingFunctionNoArgsNoReturn) (T *t) {
  auto setup = setupTestData(STR("f :: func () {}"));

  ParsingError error = {};
  AST *ast = parseDeclaration(&setup->threadData, &setup->lexer, 0, &error);
  if (!ast) FAILF("Failed to parse declaration: at %u %.*s\n",
    error.offset, (int)error.message.len, error.message.data);

  auto f = AST_CAST(ASTFunction, ast);
  if (!f) FAILF("Expected function got: %s\n", toString(ast->type));

  if (!StrEqual(f->name->name, STR("f")))
    FAILF("Unexpected name: %.*s", (int)f->name->name.len, f->name->name.data);

  if (f->args.len != 0) FAILF("Unexpected number of args: %d\n", f->args.len);
  if (f->returns.len != 0) FAILF("Unexpected number of returns: %d\n", f->returns.len);
}

TEST(ParsingFunctionNoArgsSingleReturn) (T *t) {
  auto setup = setupTestData(STR("f :: func () i32 {}"));

  ParsingError error = {};
  AST *ast = parseDeclaration(&setup->threadData, &setup->lexer, 0, &error);
  if (!ast) FAILF("Failed to parse declaration: at %u %.*s\n",
    error.offset, (int)error.message.len, error.message.data);

  auto f = AST_CAST(ASTFunction, ast);
  if (!f) FAILF("Expected function got: %s\n", toString(ast->type));

  if (!StrEqual(f->name->name, STR("f")))
    FAILF("Unexpected name: %.*s", (int)f->name->name.len, f->name->name.data);

  if (f->args.len != 0) FAILF("Unexpected number of args: %d\n", f->args.len);
  if (f->returns.len != 1) FAILF("Unexpected number of returns: %d\n", f->returns.len);

  auto ident = AST_CAST(ASTIdentifier, f->returns[0]->typeExpr);
  if (!ident) FAILF("Unexpected return type node type: %s\n", toString(f->returns[0]->typeExpr->type));

  if (!StrEqual(ident->name, STR("i32"))) FAILF("Unexpected return type value: %.*s\n", (int)ident->name.len, ident->name.data);
}

TEST(ParsingFunctionNoArgsMultipleReturns) (T *t) {
  auto setup = setupTestData(STR("f :: func () (i32, i64) {}"));

  ParsingError error = {};
  AST *ast = parseDeclaration(&setup->threadData, &setup->lexer, 0, &error);
  if (!ast) FAILF("Failed to parse declaration: at %u %.*s from %s:%d\n",
    error.offset, (int)error.message.len, error.message.data,
    error.producerSourceCodeFile, error.producerSourceCodeLine);

  auto f = AST_CAST(ASTFunction, ast);
  if (!f) FAILF("Expected function got: %s\n", toString(ast->type));

  if (!StrEqual(f->name->name, STR("f")))
    FAILF("Unexpected name: %.*s", (int)f->name->name.len, f->name->name.data);

  if (f->args.len != 0) FAILF("Unexpected number of args: %d\n", f->args.len);
  if (f->returns.len != 2) FAILF("Unexpected number of returns: %d\n", f->returns.len);

  {
    auto ident = AST_CAST(ASTIdentifier, f->returns[0]->typeExpr);
    if (!ident)
      FAILF("#0 Unexpected return type node type: %s\n",
            toString(f->returns[0]->typeExpr->type));

    if (!StrEqual(ident->name, STR("i32")))
      FAILF("#0 Unexpected return type value: %.*s\n", (int)ident->name.len,
            ident->name.data);
  }

  {
    auto ident = AST_CAST(ASTIdentifier, f->returns[1]->typeExpr);
    if (!ident)
      FAILF("#1 Unexpected return type node type: %s\n",
            toString(f->returns[0]->typeExpr->type));

    if (!StrEqual(ident->name, STR("i64")))
      FAILF("#1 Unexpected return type value: %.*s\n", (int)ident->name.len,
            ident->name.data);
  }
}

TEST(ParsingFunctionSingleArgsNoReturn) (T *t) {
  auto setup = setupTestData(STR("f :: func (a: i32) {}"));

  ParsingError error = {};
  AST *ast = parseDeclaration(&setup->threadData, &setup->lexer, 0, &error);
  if (!ast) FAILF("Failed to parse declaration: at %u %.*s\n",
    error.offset, (int)error.message.len, error.message.data);

  auto f = AST_CAST(ASTFunction, ast);
  if (!f) FAILF("Expected function got: %s\n", toString(ast->type));

  if (!StrEqual(f->name->name, STR("f")))
    FAILF("Unexpected name: %.*s", (int)f->name->name.len, f->name->name.data);

  if (f->args.len != 1) FAILF("Unexpected number of args: %d\n", f->args.len);
  if (f->returns.len != 0) FAILF("Unexpected number of returns: %d\n", f->returns.len);

  if (!StrEqual(f->args[0]->name->name, STR("a")))
    FAILF("#0 Unexpected argument name: %.*s\n",
          (int)f->args[0]->name->name.len, f->args[0]->name->name.data);
  auto typeIdent = AST_CAST(ASTIdentifier, f->args[0]->typeExpr);
  if (!typeIdent)
    FAILF("Expected type expr to be ident: got %s\n", toString(f->args[0]->typeExpr->type));
  if (!StrEqual(typeIdent->name, STR("i32")))
    FAILF("#0 Unexpected argument type: %.*s\n",
          (int)typeIdent->name.len, typeIdent->name.data);

}

TEST(ParsingFunctionMultipleArgsNoReturn) (T *t) {
  auto setup = setupTestData(STR("f :: func (a: i32, b: i64) {}"));

  ParsingError error = {};
  AST *ast = parseDeclaration(&setup->threadData, &setup->lexer, 0, &error);
  if (!ast) FAILF("Failed to parse declaration: at %u %.*s\n",
    error.offset, (int)error.message.len, error.message.data);

  auto f = AST_CAST(ASTFunction, ast);
  if (!f) FAILF("Expected function got: %s\n", toString(ast->type));

  if (!StrEqual(f->name->name, STR("f")))
    FAILF("Unexpected name: %.*s", (int)f->name->name.len, f->name->name.data);

  if (f->args.len != 2) FAILF("Unexpected number of args: %d\n", f->args.len);
  if (f->returns.len != 0) FAILF("Unexpected number of returns: %d\n", f->returns.len);

  {
    if (!StrEqual(f->args[0]->name->name, STR("a")))
      FAILF("#0 Unexpected argument name: %.*s\n",
            (int)f->args[0]->name->name.len, f->args[0]->name->name.data);
    auto typeIdent = AST_CAST(ASTIdentifier, f->args[0]->typeExpr);
    if (!typeIdent)
      FAILF("#0 Expected type expr to be ident: got %s\n",
            toString(f->args[0]->typeExpr->type));
    if (!StrEqual(typeIdent->name, STR("i32")))
      FAILF("#0 Unexpected argument type: %.*s\n", (int)typeIdent->name.len,
            typeIdent->name.data);
  }
  {
    if (!StrEqual(f->args[1]->name->name, STR("b")))
      FAILF("#1 Unexpected argument name: %.*s\n",
            (int)f->args[1]->name->name.len, f->args[1]->name->name.data);
    auto typeIdent = AST_CAST(ASTIdentifier, f->args[1]->typeExpr);
    if (!typeIdent)
      FAILF("#1 Expected type expr to be ident: got %s\n",
            toString(f->args[1]->typeExpr->type));
    if (!StrEqual(typeIdent->name, STR("i64")))
      FAILF("#1 Unexpected argument type: %.*s\n", (int)typeIdent->name.len,
            typeIdent->name.data);
  }
}


TEST(ParsingFunctionMultipleArgsCombinedNoReturn) (T *t) {
  auto setup = setupTestData(STR("f :: func (a, b: i64) {}"));

  ParsingError error = {};
  AST *ast = parseDeclaration(&setup->threadData, &setup->lexer, 0, &error);
  if (!ast) FAILF("Failed to parse declaration: at %u %.*s\n",
    error.offset, (int)error.message.len, error.message.data);

  auto f = AST_CAST(ASTFunction, ast);
  if (!f) FAILF("Expected function got: %s\n", toString(ast->type));

  if (!StrEqual(f->name->name, STR("f")))
    FAILF("Unexpected name: %.*s", (int)f->name->name.len, f->name->name.data);

  if (f->args.len != 2) FAILF("Unexpected number of args: %d\n", f->args.len);
  if (f->returns.len != 0) FAILF("Unexpected number of returns: %d\n", f->returns.len);

  {
    if (!StrEqual(f->args[0]->name->name, STR("a")))
      FAILF("#0 Unexpected argument name: %.*s\n",
            (int)f->args[0]->name->name.len, f->args[0]->name->name.data);
    auto typeIdent = AST_CAST(ASTIdentifier, f->args[0]->typeExpr);
    if (!typeIdent)
      FAILF("#0 Expected type expr to be ident: got %s\n",
            toString(f->args[0]->typeExpr->type));
    if (!StrEqual(typeIdent->name, STR("i64")))
      FAILF("#0 Unexpected argument type: %.*s\n", (int)typeIdent->name.len,
            typeIdent->name.data);
  }
  {
    if (!StrEqual(f->args[1]->name->name, STR("b")))
      FAILF("#1 Unexpected argument name: %.*s\n",
            (int)f->args[1]->name->name.len, f->args[1]->name->name.data);
    auto typeIdent = AST_CAST(ASTIdentifier, f->args[1]->typeExpr);
    if (!typeIdent)
      FAILF("#1 Expected type expr to be ident: got %s\n",
            toString(f->args[1]->typeExpr->type));
    if (!StrEqual(typeIdent->name, STR("i64")))
      FAILF("#1 Unexpected argument type: %.*s\n", (int)typeIdent->name.len,
            typeIdent->name.data);
  }
}
