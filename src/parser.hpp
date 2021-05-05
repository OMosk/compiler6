#pragma once
#include "lexer.hpp"
#include "ast.hpp"

struct Error {
  Site site;
  Str message;
  int tokenIndex;
};

void debugPrintAST(ASTNode *root, FILE *out, int shiftWidth, int currentShift, int firstLineShift);
void debugPrintAST(ASTNode *root, FILE *out, int shiftWidth, int currentShift) {
  debugPrintAST(root, out, shiftWidth, currentShift, currentShift);
}

ASTFile *parseFile(Tokens tokens, Error *error);

const char *opToStr(int op);
int opPriority(int op);
