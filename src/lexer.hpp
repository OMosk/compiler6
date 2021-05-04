#pragma once
#include "utils.hpp"

enum {
  TokenEOF = -1,
  TokenLineComment = -2,
  TokenIdentifier = -3,
  TokenDeclare = -4,

  TokenEq = -5,
  TokenNEq = -6,

  TokenShl = -7,
  TokenShr = -8,

  TokenLE = -9,
  TokenGE = -10,

  TokenLogicalOr = -11,
  TokenLogicalAnd = -12,
  TokenDefine = -13,
  TokenNumber = -14,
  TokenReturn = -15,
  TokenStringLiteral = -16,
  TokenForeign = -17,
  TokenVariadic = -18,

  TokenIf = -19,
  TokenElse = -20,

  TokenWhile = -21,
  TokenBreak = -22,
  TokenContinue = -23,

  TokenLoad = -24,

  TokenStruct = -25,
  TokenOpaque = -26,
  TokenExtern = -27,
  TokenNil = -28,
  TokenDefer = -29,
  TokenExportScope = -30,
  TokenModuleScope = -31,
  TokenFunction = -32,
  TokenVar = -33,
};

struct Token {
  uint32_t line;
  uint16_t col;
  int16_t tokenType;
  Str value;
};

struct Tokens {
  bool ok;
  int fileIndex;
  Array<Token> arr;
};

Tokens tokenize(Str content, const char *abs_filename, const char *relative_filename);
void debugPrintTokens(Tokens tokens, FILE *output);
