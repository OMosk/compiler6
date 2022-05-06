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
  initAllocator(&result->threadData.allocator, (char *)malloc(1024), 1024);

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
