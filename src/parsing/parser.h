#pragma once

#include "../core_types.h"
#include "../ast.h"
#include "tokenization.h"

struct ParsingError {
  uint32_t offset;
  Str message;
};

#define FORWARD_DECLARE_PARSER(NAME) \
  AST *NAME(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags, ParsingError *error)

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
