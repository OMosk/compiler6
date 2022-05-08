#pragma once

#include "../core_types.h"
#include "../ast.h"
#include "tokenization.h"

// enum ParsingFlag {
//   PARSING_FLAG_
// };

struct ParsingError {
  uint32_t offset;
  Str message;
  const char *producerSourceCodeFile;
  int producerSourceCodeLine;
};

#define FORWARD_DECLARE_PARSER(NAME) \
  AST *NAME(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags, ParsingError *error)

FORWARD_DECLARE_PARSER(parseFile);
FORWARD_DECLARE_PARSER(parseTopLevelDeclaration);
FORWARD_DECLARE_PARSER(parseLoadDirective);
FORWARD_DECLARE_PARSER(parseDeclaration);
FORWARD_DECLARE_PARSER(parseAnonymousStruct);
FORWARD_DECLARE_PARSER(parseAnonymousFunction);
FORWARD_DECLARE_PARSER(parseBlock);
FORWARD_DECLARE_PARSER(parseStatement);

FORWARD_DECLARE_PARSER(parseIdentifier);
FORWARD_DECLARE_PARSER(parseStringLiteral);
FORWARD_DECLARE_PARSER(parseNumberLiteral);

FORWARD_DECLARE_PARSER(parseExpr);
FORWARD_DECLARE_PARSER(parseUnaryExpr);
FORWARD_DECLARE_PARSER(parseParenExpr);

FORWARD_DECLARE_PARSER(parseCallContinuation);
FORWARD_DECLARE_PARSER(parseSubscriptContinuation);
FORWARD_DECLARE_PARSER(parseMemberAccessContinuation);
FORWARD_DECLARE_PARSER(parseCastContinuation);
