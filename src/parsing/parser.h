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
