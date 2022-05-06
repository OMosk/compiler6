#include "../all.h"

TEST(ASTSetupWorks) (T *t) {
  Allocator a = {};
  initAllocator(&a, (char *)malloc(1024), 1024);

  ASTIdentifier *ident = AST_ALLOC(ASTIdentifier, &a);
  ASTNodeType wanted = ASTIdentifier_;
  if (ident->type != wanted) {
    t->Printf("Unexpected type: want '%s' got '%s'\n",
      toString(wanted), toString(ident->type));
    t->Fail();
    return;
  }
}

TEST(ASTSafeCastWorks) (T *t) {
  Allocator a = {};
  initAllocator(&a, (char *)malloc(1024), 1024);

  AST *ident = AST_ALLOC(ASTIdentifier, &a);
  if (AST_CAST(ASTIdentifier, ident) == NULL) {
    t->Printf("%s:%d Unexpected null in AST_CAST", __FILE__, __LINE__);
    t->Fail();
  }

  if (AST_CAST(ASTStringLiteral, ident) != NULL) {
    t->Printf("%s:%d Unexpected non-null in AST_CAST", __FILE__, __LINE__);
    t->Fail();
  }
}
