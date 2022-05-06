#include "parser.h"

#define DEFINE_PARSER(NAME)                                                    \
  AST *NAME##Impl(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags,        \
                  ParsingError *error);                                        \
  AST *NAME(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags,              \
            ParsingError *error) {                                             \
    Token positionBefore = lexer->peek();                                      \
    AST *result = NAME##Impl(ctx, lexer, parsingFlags, error);                 \
    if (!result) {                                                             \
      lexer->reset(positionBefore);                                            \
    } else {                                                                   \
      *error = {};                                                             \
    }                                                                          \
    return result;                                                             \
  }                                                                            \
  AST *NAME##Impl(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags,        \
                  ParsingError *error)

#define MATCH_TOKEN(NAME, TYPE, MESSAGE)                                       \
  Token NAME = lexer->eat();                                                   \
  if (NAME.type != TYPE) {                                                     \
    error->offset = NAME.offset0;                                              \
    error->message = STR(MESSAGE);                                             \
    return NULL;                                                               \
  }

#define RETURN_NULL_WITH_ERROR(OFFSET, MESSAGE)                                \
  do {                                                                         \
    error->offset = (OFFSET);                                                  \
    error->message = STR(MESSAGE);                                             \
    return NULL;                                                               \
  } while (0)

Str slice(Str src, uint32_t offset0, uint32_t offset1) {
  auto result = Str{
      src.data + offset0,
      offset1 - offset0,
  };
  return result;
}

DEFINE_PARSER(parseIdentifier) {
  MATCH_TOKEN(identToken, TOKEN_TYPE_IDENTIFIER, "Expected identifier")

  auto ident = AST_ALLOC(ASTIdentifier, &ctx->allocator);
  ident->fileIndex = lexer->fileIndex;
  ident->offset0 = identToken.offset0;
  ident->offset1 = identToken.offset1;
  ident->name = slice(lexer->source, ident->offset0, ident->offset1);

  return ident;
}

DEFINE_PARSER(parseUnaryExpr) {
  ASTUnaryOp *leftMostUnaryOp = NULL;
  ASTUnaryOp *lastUnaryOp = NULL;

  for (bool shouldContinue = true; shouldContinue;) {
    Token nextToken = lexer->peek();
    UnaryOp op = UNARY_OP_NOOP;
    switch (nextToken.type) {
    case TOKEN_TYPE_PLUS: op = UNARY_OP_PLUS; break;
    case TOKEN_TYPE_MINUS: op = UNARY_OP_MINUS; break;
    case TOKEN_TYPE_MULTIPLY: op = UNARY_OP_DEREFERENCE; break;
    case TOKEN_TYPE_AMPERSAND: op = UNARY_OP_ADDRESSOF; break;
    case TOKEN_TYPE_EXCLAMATION_MARK: op = UNARY_OP_LOGICAL_NEGATE; break;
    case TOKEN_TYPE_TILDE: op = UNARY_OP_BITWISE_NEGATE; break;
    default: shouldContinue = false;
    }

    if (shouldContinue) {
      lexer->eat();
      ASTUnaryOp *unaryOp = AST_ALLOC(ASTUnaryOp, &ctx->allocator);
      unaryOp->fileIndex = lexer->fileIndex;
      unaryOp->offset0 = nextToken.offset0;
      unaryOp->offset1 = nextToken.offset1;
      unaryOp->op = op;
      if (lastUnaryOp) {
        lastUnaryOp->operand = unaryOp;
      }
      lastUnaryOp = unaryOp;
      if (!leftMostUnaryOp) leftMostUnaryOp = unaryOp;
    }
  }

  AST *operand = NULL;
  if (!operand) operand = parseIdentifier(ctx, lexer, parsingFlags, error);
  if (!operand) operand = parseStringLiteral(ctx, lexer, parsingFlags, error);
  if (!operand) operand = parseNumberLiteral(ctx, lexer, parsingFlags, error);
  if (!operand) operand = parseParenExpr(ctx, lexer, parsingFlags, error);
  if (!operand) RETURN_NULL_WITH_ERROR(lexer->peek().offset0, "Expected unary expression operand");

  for (;;) {
    AST *continuation = NULL;

    if (!continuation) {
      continuation = parseCallContinuation(ctx, lexer, parsingFlags, error);
      if (continuation) {
        auto call = AST_CAST(ASTCall, continuation);
        call->callee = operand;
      }
    }

    if (!continuation) {
      continuation =
          parseSubscriptContinuation(ctx, lexer, parsingFlags, error);
      if (continuation) {
        auto call = AST_CAST(ASTSubscript, continuation);
        call->indexable = operand;
      }
    }

    if (!continuation) {
      continuation = parseCastContinuation(ctx, lexer, parsingFlags, error);
      if (continuation) {
        auto call = AST_CAST(ASTCast, continuation);
        call->operand = operand;
      }
    }

    if (!continuation) {
      continuation =
          parseMemberAccessContinuation(ctx, lexer, parsingFlags, error);
      if (continuation) {
        auto call = AST_CAST(ASTMemberAccess, continuation);
        call->structLike = operand;
      }
    }

    if (continuation) {
      continuation->offset0 = operand->offset0;
      operand = continuation;
    } else {
      break;
    }
  }

  if (lastUnaryOp) lastUnaryOp->operand = operand;
  if (leftMostUnaryOp) {
    return leftMostUnaryOp;
  } else {
    return operand;
  }
}

DEFINE_PARSER(parseStringLiteral) {
  MATCH_TOKEN(token, TOKEN_TYPE_STRING_LITERAL, "Expected string literal");
  auto literal = AST_ALLOC(ASTStringLiteral, &ctx->allocator);
  literal->fileIndex = lexer->fileIndex;
  literal->offset0 = token.offset0;
  literal->offset1 = token.offset1;

  auto rawValue = slice(lexer->source, token.offset0, token.offset1);
  auto newValue = Str{
    ALLOC_ARRAY(char, rawValue.len, &ctx->allocator),
    rawValue.len - 2,
  };

  //Skip " at the beginning
  rawValue.data++;
  rawValue.len--;

  //Skip " at the end
  rawValue.len--;

  // TODO: correctly handle utf-8
  for (int from = 0, to = 0; from < rawValue.len; ++from, ++to) {
    if (rawValue.data[from] == '\\') {
      switch (rawValue.data[from + 1]) {
      case 'n':  newValue.data[to] = '\n'; break;
      case 't':  newValue.data[to] = '\t'; break;
      case '\\': newValue.data[to] = '\\'; break;
      case '"':  newValue.data[to] = '"'; break;
      }
      from++; //eat one more character
      continue;
    } else {
      newValue.data[to] = rawValue.data[from];
    }
  }

  literal->value = newValue;

  return literal;
}

DEFINE_PARSER(parseNumberLiteral) {
  MATCH_TOKEN(token, TOKEN_TYPE_NUMBER_LITERAL, "Expected number literal");
  auto literal = AST_ALLOC(ASTNumberLiteral, &ctx->allocator);
  literal->fileIndex = lexer->fileIndex;
  literal->offset0 = token.offset0;
  literal->offset1 = token.offset1;
  literal->value = slice(lexer->source, token.offset0, token.offset1);
  return literal;
}


DEFINE_PARSER(parseParenExpr) {
  MATCH_TOKEN(openingParen, TOKEN_TYPE_LEFT_PAREN, "Expected '('");
  AST *expr = parseExpr(ctx, lexer, parsingFlags, error);
  if (!expr) RETURN_NULL_WITH_ERROR(lexer->peek().offset0, "Expected expression");
  MATCH_TOKEN(closingParen, TOKEN_TYPE_RIGHT_PAREN, "Expected ')'");

  auto binaryOp = AST_CAST(ASTBinaryOp, expr);
  if (binaryOp) {
    binaryOp->flags |= AST_FLAGS_EXPR_IN_PAREN;
    binaryOp->offset0 = openingParen.offset0;
    binaryOp->offset1 = openingParen.offset1;
  }

  return expr;
}


DEFINE_PARSER(parseCallContinuation) {
  MATCH_TOKEN(openingParen, TOKEN_TYPE_LEFT_PAREN, "Expected '('");
  Array<AST *> args = {};
  for (;;) {
    if (lexer->peek().type == TOKEN_TYPE_RIGHT_PAREN) break;

    AST *arg = parseExpr(ctx, lexer, parsingFlags, error);
    if (!arg) return NULL;
    append(&args, arg, &ctx->allocator);

    if (lexer->peek().type == TOKEN_TYPE_COMMA) lexer->eat();
  }
  MATCH_TOKEN(closingParen, TOKEN_TYPE_RIGHT_PAREN, "Expected ')'");

  auto call = AST_ALLOC(ASTCall, &ctx->allocator);
  call->args = args;
  call->fileIndex = lexer->fileIndex;
  call->offset0 = openingParen.offset0;
  call->offset1 = closingParen.offset1;

  return call;
}


DEFINE_PARSER(parseSubscriptContinuation) {
  MATCH_TOKEN(openingBracket, TOKEN_TYPE_LEFT_BRACKET, "Expected '['");
  AST *index = parseExpr(ctx, lexer, parsingFlags, error);
  if (!index) return NULL;
  MATCH_TOKEN(closingBracket, TOKEN_TYPE_RIGHT_BRACKET, "Expected ']'");

  auto subscript = AST_ALLOC(ASTSubscript, &ctx->allocator);
  subscript->index = index;
  subscript->fileIndex = lexer->fileIndex;
  subscript->offset0 = openingBracket.offset0;
  subscript->offset1 = closingBracket.offset1;

  return subscript;
}

DEFINE_PARSER(parseCastContinuation) {
  MATCH_TOKEN(dot, TOKEN_TYPE_DOT, "Expected '.'");
  MATCH_TOKEN(openingParen, TOKEN_TYPE_LEFT_PAREN, "Expected '('");

  AST *typeExpr = parseExpr(ctx, lexer, parsingFlags, error);
  if (!typeExpr) return NULL;

  MATCH_TOKEN(closingParen, TOKEN_TYPE_RIGHT_PAREN, "Expected ')'");

  auto cast = AST_ALLOC(ASTCast, &ctx->allocator);
  cast->toTypeExpr = typeExpr;
  cast->fileIndex = lexer->fileIndex;
  cast->offset0 = openingParen.offset0;
  cast->offset1 = closingParen.offset1;

  return cast;
}

DEFINE_PARSER(parseMemberAccessContinuation) {
  MATCH_TOKEN(dot, TOKEN_TYPE_DOT, "Expected '.'");

  auto *ident = static_cast<ASTIdentifier *>(parseIdentifier(ctx, lexer, parsingFlags, error));
  if (!ident) return NULL;

  auto cast = AST_ALLOC(ASTMemberAccess, &ctx->allocator);
  cast->field = ident;
  cast->fileIndex = lexer->fileIndex;
  cast->offset0 = dot.offset0;
  cast->offset1 = ident->offset1;

  return cast;
}


DEFINE_PARSER(parseExpr) {
  auto left = parseUnaryExpr(ctx, lexer, parsingFlags, error);
  if (!left) return NULL;

  for (;;) {
    BinaryOp op = BINARY_OP_NOOP;

    auto opTokenType = lexer->peek().type;
    switch (opTokenType) {
    case TOKEN_TYPE_PLUS: op = BINARY_OP_PLUS; break;
    case TOKEN_TYPE_MINUS: op = BINARY_OP_MINUS; break;

    case TOKEN_TYPE_MULTIPLY: op = BINARY_OP_MULTIPLY; break;
    case TOKEN_TYPE_DIVIDE: op = BINARY_OP_DIVISION; break;

    case TOKEN_TYPE_AMPERSAND: op = BINARY_OP_BITWISE_AND; break;
    case TOKEN_TYPE_PIPE: op = BINARY_OP_BITWISE_OR; break;
    case TOKEN_TYPE_CARET: op = BINARY_OP_BITWISE_XOR; break;

    case TOKEN_TYPE_AMPERSAND_AMPERSAND: op = BINARY_OP_LOGICAL_AND; break;
    case TOKEN_TYPE_PIPE_PIPE: op = BINARY_OP_LOGICAL_OR; break;

    case TOKEN_TYPE_LEFT_ANGLE_BRACKET: op = BINARY_OP_LESS; break;
    case TOKEN_TYPE_LEFT_ANGLE_BRACKET_EQUALS: op = BINARY_OP_LESS_OR_EQUAL; break;
    case TOKEN_TYPE_RIGHT_ANGLE_BRACKET: op = BINARY_OP_GREATER; break;
    case TOKEN_TYPE_RIGHT_ANGLE_BRACKET_EQUALS: op = BINARY_OP_GREATER_OR_EQUAL; break;
    case TOKEN_TYPE_EQUALS_EQUALS: op = BINARY_OP_EQUAL; break;
    case TOKEN_TYPE_EXCLAMATION_MARK_EQUALS: op = BINARY_OP_NOT_EQUAL; break;
    default: break;
    }

    if (!op) {
      break;
    }

    Token operationToken = lexer->eat();

    auto rightOperand = parseUnaryExpr(ctx, lexer, parsingFlags, error);
    if (!rightOperand) return NULL;

    auto binaryOp = AST_ALLOC(ASTBinaryOp, &ctx->allocator);
    binaryOp->left = left;
    binaryOp->right = rightOperand;
    binaryOp->op = op;
    binaryOp->fileIndex = lexer->fileIndex;
    binaryOp->offset0 = operationToken.offset0;
    binaryOp->offset1 = operationToken.offset1;

    auto leftBinaryOp = AST_CAST(ASTBinaryOp, left);
    if (leftBinaryOp) {
      bool leftOpIsInParen = leftBinaryOp->flags & AST_FLAGS_EXPR_IN_PAREN;
      if (priority(leftBinaryOp->op) < priority(binaryOp->op) && !leftOpIsInParen) {
        binaryOp->left = leftBinaryOp->right;
        leftBinaryOp->right = binaryOp;

        binaryOp = leftBinaryOp;
      }
    }
    left = binaryOp;
  }

  return left;
}
