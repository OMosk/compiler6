#include "parser.h"

AST *decorateParseFunctionCall(ParseFunction *fn, ThreadData *ctx, Lexer *lexer,
                            uint64_t parsingFlags, ParsingError *error) {
  Token positionBefore = lexer->peek();
  ParsingError tmpError = {};
  char *allocatorPositionBefore = ctx->allocator.current;
  AST *result = fn(ctx, lexer, parsingFlags, &tmpError);
  if (!result) {
    if (tmpError.offset > error->offset)
      *error = tmpError;
    lexer->reset(positionBefore);
    ctx->allocator.current = allocatorPositionBefore;
  } else {
    *error = {};
  }
  return result;
}

#define DEFINE_PARSER(NAME)                                                    \
  AST *NAME##Impl(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags,        \
                  ParsingError *error);                                        \
  AST *NAME(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags,              \
            ParsingError *error) {                                             \
    return decorateParseFunctionCall(NAME##Impl, ctx, lexer, parsingFlags,     \
                                     error);                                   \
  }                                                                            \
  AST *NAME##Impl(ThreadData *ctx, Lexer *lexer, uint64_t parsingFlags,        \
                  ParsingError *error)

#define MATCH_TOKEN(NAME, TYPE, MESSAGE)                                       \
  Token NAME = lexer->eat();                                                   \
  if (NAME.type != TYPE) {                                                     \
    error->offset = NAME.offset0;                                              \
    error->message = STR(MESSAGE);                                             \
    error->producerSourceCodeFile = __FILE__;                                  \
    error->producerSourceCodeLine = __LINE__;                                  \
    return NULL;                                                               \
  }

#define RETURN_NULL_WITH_ERROR(OFFSET, MESSAGE)                                \
  do {                                                                         \
    error->offset = (OFFSET);                                                  \
    error->message = STR(MESSAGE);                                             \
    error->producerSourceCodeFile = __FILE__;                                  \
    error->producerSourceCodeLine = __LINE__;                                  \
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
    //TODO: come back to this and think again if this behavior is ok and correct
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

DEFINE_PARSER(parseFile) {
  Array<AST *> topLevelDecls = {};
  for (;;) {
    auto decl = parseTopLevelDeclaration(ctx, lexer, parsingFlags, error);
    if (decl) {
      append(&topLevelDecls, decl, &ctx->allocator);
    } else {
      auto token = lexer->peek();

      if (token.type == TOKEN_TYPE_EOF) {
        lexer->eat();
        break;
      } else if (token.type == TOKEN_TYPE_UNEXPECTED_SEQUENCE_OF_CHARS) {
        RETURN_NULL_WITH_ERROR(token.offset0, "Unexpected sequence of characters");
      } else if (token.type == TOKEN_TYPE_UNTERMINATED_STRING_LITERAL) {
        RETURN_NULL_WITH_ERROR(token.offset0, "Unterminated string literal");
      } else {
        return NULL;
      }
    }
  }

  auto file = AST_ALLOC(ASTFile, &ctx->allocator);
  file->topLevelDecls = topLevelDecls;
  return file;
}

DEFINE_PARSER(parseTopLevelDeclaration) {
  AST *decl = NULL;
  if (!decl) decl = parseLoadDirective(ctx, lexer, parsingFlags, error);
  if (!decl) decl = parseDeclaration(ctx, lexer, parsingFlags, error);
  if (!decl) decl = parseVariableDefinition(ctx, lexer, parsingFlags, error);

  return decl;
}


DEFINE_PARSER(parseLoadDirective) {
  MATCH_TOKEN(loadToken, TOKEN_TYPE_LOAD_DIRECTIVE, "Expected '#load' directive");
  auto stringLiteralBase = parseStringLiteral(ctx, lexer, parsingFlags, error);
  if (!stringLiteralBase) return NULL;

  auto stringLiteral = AST_CAST(ASTStringLiteral, stringLiteralBase);

  auto directive = AST_ALLOC(ASTLoadDirective, &ctx->allocator);
  directive->path = stringLiteral;
  directive->fileIndex = lexer->fileIndex;
  directive->offset0 = loadToken.offset0;
  directive->offset1 = stringLiteral->offset1;

  return directive;
}


DEFINE_PARSER(parseAnonymousStruct) {
  MATCH_TOKEN(strucToken, TOKEN_TYPE_STRUCT, "Expected struct keyword");
  MATCH_TOKEN(openingBrace, TOKEN_TYPE_LEFT_BRACE, "Expected {");

  auto newStruct = AST_ALLOC(ASTStruct, &ctx->allocator);

  while (lexer->peek().type != TOKEN_TYPE_RIGHT_BRACE) {
    Array<ASTIdentifier *> fieldNames = {};

    while (lexer->peek().type != TOKEN_TYPE_COLON) {
      auto ident = parseIdentifier(ctx, lexer, parsingFlags, error);
      if (!ident) return NULL;

      append(&fieldNames, AST_ASSERT_CAST(ASTIdentifier, ident),
        &ctx->allocator);

      if (lexer->peek().type == TOKEN_TYPE_COMMA) {
        lexer->eat();
        continue;
      } else {
        break;
      }
    }

    MATCH_TOKEN(colon, TOKEN_TYPE_COLON, "Expected :");
    auto typeExpr = parseExpr(ctx, lexer, parsingFlags, error);
    if (!typeExpr) NULL;
    MATCH_TOKEN(semicolon, TOKEN_TYPE_SEMICOLON, "Expected ;");

    for (int i = 0; i < fieldNames.len; i++) {
      auto name = fieldNames[i];
      auto newMember = AST_ALLOC(ASTVar, &ctx->allocator);
      newMember->name = name;
      newMember->typeExpr = typeExpr;
      newMember->parentScope = newStruct;
      append(&newStruct->members, newMember, &ctx->allocator);
    }
  }

  MATCH_TOKEN(closingBrace, TOKEN_TYPE_RIGHT_BRACE, "Expected }");

  newStruct->fileIndex = lexer->fileIndex;
  newStruct->offset0 = strucToken.offset0;
  newStruct->offset1 = closingBrace.offset1;
  return newStruct;
}

DEFINE_PARSER(parseDeclaration) {
  auto identAST = parseIdentifier(ctx, lexer, parsingFlags, error);
  if (!identAST) return NULL;
  auto ident = AST_ASSERT_CAST(ASTIdentifier, identAST);

  MATCH_TOKEN(colonColon, TOKEN_TYPE_COLON_COLON, "Expected '::'");

  AST *thing = NULL;
  //TODO: probably anonymous struct can be part of expression
  // so it makes sense to in future to move parsing there
  if (!thing) thing = parseAnonymousStruct(ctx, lexer, parsingFlags, error);
  if (!thing) thing = parseAnonymousFunction(ctx, lexer, parsingFlags, error);
  if (!thing) thing = parseExpr(ctx, lexer, parsingFlags, error);
  if (!thing) return NULL;

  switch (thing->type) {
  case ASTStruct_: {
    auto n = AST_ASSERT_CAST(ASTStruct, thing);
    n->name = ident;
    n->offset0 = ident->offset0;
  } break;
  case ASTFunction_: {
    auto n = AST_ASSERT_CAST(ASTFunction, thing);
    n->name = ident;
    n->offset0 = ident->offset0;
  } break;
  default: {
    auto n = AST_ALLOC(ASTConst, &ctx->allocator);
    n->name = ident;
    n->initExpr = thing;
    n->fileIndex = lexer->fileIndex;
    n->offset0 = ident->offset0;
    n->offset1 = thing->offset1;
    thing = n;
  } break;
  }

  return thing;
}

DEFINE_PARSER(parseAnonymousFunction) {
  MATCH_TOKEN(funcToken, TOKEN_TYPE_FUNC, "Expected 'func' keyword");

  auto newFunction = AST_ALLOC(ASTFunction, &ctx->allocator);

  MATCH_TOKEN(openingParen, TOKEN_TYPE_LEFT_PAREN, "Expected '('");

  while (lexer->peek().type != TOKEN_TYPE_RIGHT_PAREN) {
    Array<ASTIdentifier *> parameterNames = {};

    while (lexer->peek().type != TOKEN_TYPE_COLON) {
      auto ident = parseIdentifier(ctx, lexer, parsingFlags, error);
      if (!ident) return NULL;

      append(&parameterNames, AST_ASSERT_CAST(ASTIdentifier, ident),
        &ctx->allocator);

      if (lexer->peek().type == TOKEN_TYPE_COMMA) {
        lexer->eat();
        continue;
      } else {
        break;
      }
    }

    MATCH_TOKEN(colon, TOKEN_TYPE_COLON, "Expected :");
    auto typeExpr = parseExpr(ctx, lexer, parsingFlags, error);
    if (!typeExpr) NULL;


    for (int i = 0; i < parameterNames.len; i++) {
      auto name = parameterNames[i];
      auto newMember = AST_ALLOC(ASTVar, &ctx->allocator);
      newMember->name = name;
      newMember->typeExpr = typeExpr;
      newMember->parentScope = newFunction;
      append(&newFunction->args, newMember, &ctx->allocator);
    }

    if (lexer->peek().type == TOKEN_TYPE_COMMA) {
      lexer->eat();
      continue;
    } else {
      break;
    }
  }

  MATCH_TOKEN(closingParen, TOKEN_TYPE_RIGHT_PAREN, "Expected ')'");

  if (lexer->peek().type == TOKEN_TYPE_LEFT_PAREN) {
    lexer->eat();
    while (lexer->peek().type != TOKEN_TYPE_RIGHT_PAREN) {
      auto typeExpr = parseExpr(ctx, lexer, parsingFlags, error);
      if (!typeExpr) return NULL;

      ASTVar *returnVar = AST_ALLOC(ASTVar, &ctx->allocator);
      returnVar->fileIndex = lexer->fileIndex;
      returnVar->offset0 = typeExpr->offset0;
      returnVar->offset1 = typeExpr->offset1;
      returnVar->typeExpr = typeExpr;

      append(&newFunction->returns, returnVar, &ctx->allocator);

      if (lexer->peek().type == TOKEN_TYPE_COMMA) {
        lexer->eat();
        continue;
      } else {
        break;
      }
    }
    MATCH_TOKEN(closingParen, TOKEN_TYPE_RIGHT_PAREN, "Expected ')'");
  } else if (lexer->peek().type != TOKEN_TYPE_LEFT_BRACE) {
    auto typeExpr = parseExpr(ctx, lexer, parsingFlags, error);
    if (!typeExpr) return NULL;

    ASTVar *returnVar = AST_ALLOC(ASTVar, &ctx->allocator);
    returnVar->fileIndex = lexer->fileIndex;
    returnVar->offset0 = typeExpr->offset0;
    returnVar->offset1 = typeExpr->offset1;
    returnVar->typeExpr = typeExpr;

    append(&newFunction->returns, returnVar, &ctx->allocator);
  }

  auto bodyBlock = parseBlock(ctx, lexer, parsingFlags, error);
  if (!bodyBlock) return NULL;

  newFunction->body = AST_ASSERT_CAST(ASTBlock, bodyBlock);

  newFunction->fileIndex = lexer->fileIndex;
  newFunction->offset0 = funcToken.offset0;
  newFunction->offset1 = bodyBlock->offset1;

  return newFunction;
}

DEFINE_PARSER(parseBlock) {
  MATCH_TOKEN(openingBrace, TOKEN_TYPE_LEFT_BRACE, "expected '{'");
  auto block = AST_ALLOC(ASTBlock, &ctx->allocator);

  while (lexer->peek().type != TOKEN_TYPE_RIGHT_BRACE) {
    auto statement = parseStatement(ctx, lexer, parsingFlags, error);
    if (!statement) return NULL;
    append(&block->statements, statement, &ctx->allocator);
  }
  MATCH_TOKEN(closingBrace, TOKEN_TYPE_RIGHT_BRACE, "expected '}'");
  block->fileIndex = lexer->fileIndex;
  block->offset0 = openingBrace.offset0;
  block->offset1 = closingBrace.offset1;
  return block;
}


DEFINE_PARSER(parseStatement) {
  AST *s = NULL;
  if (!s) s = parseBlock(ctx, lexer, parsingFlags, error);
  if (!s) s = parseVariableDefinition(ctx, lexer, parsingFlags, error);
  if (!s) s = parseDeclaration(ctx, lexer, parsingFlags, error);
  if (!s) s = parseIfStatement(ctx, lexer, parsingFlags, error);
  if (!s) s = parseWhileLoop(ctx, lexer, parsingFlags, error);
  if (!s) s = parseDeferStatement(ctx, lexer, parsingFlags, error);
  if (!s) s = parseExprStatement(ctx, lexer, parsingFlags, error);

  return s;
}


DEFINE_PARSER(parseVariableDefinition) {
  Array<ASTIdentifier *> identifiers = {};
  for (;;) {
    auto ast = parseIdentifier(ctx, lexer, parsingFlags, error);
    if (!ast) return NULL;
    append(&identifiers, AST_ASSERT_CAST(ASTIdentifier, ast), &ctx->allocator);
    if (lexer->peek().type == TOKEN_TYPE_COMMA) {
      lexer->eat();
      continue;
    } else {
      break;
    }
  }

  AST *typeExpr = NULL;

  bool expectInitialization = false;

  if (lexer->peek().type == TOKEN_TYPE_COLON_EQUAL) {
    lexer->eat();
    expectInitialization = true;
  } else if (lexer->peek().type == TOKEN_TYPE_COLON) {
    lexer->eat();
    typeExpr = parseExpr(ctx, lexer, parsingFlags, error);
    if (!typeExpr) return NULL;

    if (lexer->peek().type == TOKEN_TYPE_EQUALS) {
      lexer->eat();
      expectInitialization = true;
    }
  } else {
    RETURN_NULL_WITH_ERROR(lexer->peek().offset0, "Expected ':=' or ':'");
  }

  Array<AST *> initializationValues = {};

  if (expectInitialization) {
    for (;;) {
      auto expr = parseExpr(ctx, lexer, parsingFlags, error);
      if (!expr) return NULL;

      append(&initializationValues, expr, &ctx->allocator);
      if (lexer->peek().type == TOKEN_TYPE_COMMA) {
        lexer->eat();
        continue;
      } else {
        break;
      }
    }
  }

  MATCH_TOKEN(semicolon, TOKEN_TYPE_SEMICOLON, "Expected ';'");

  auto varDefn = AST_ALLOC(ASTVariableDefinition, &ctx->allocator);
  varDefn->fileIndex = lexer->fileIndex;
  varDefn->offset0 = identifiers[0]->offset0;
  varDefn->offset1 = semicolon.offset1;
  varDefn->names = identifiers;
  varDefn->typeExpr = typeExpr;
  varDefn->initilizationValues = initializationValues;
  return varDefn;
}


DEFINE_PARSER(parseIfStatement) {
  MATCH_TOKEN(ifToken, TOKEN_TYPE_IF, "Expected 'if' keyword");

  auto conditionExpr = parseParenExpr(ctx, lexer, parsingFlags, error);
  if (!conditionExpr) return NULL;

  auto thenStatement = parseStatement(ctx, lexer, parsingFlags, error);
  if (!thenStatement) return NULL;

  AST *elseStatement = NULL;
  if (lexer->peek().type == TOKEN_TYPE_ELSE) {
    lexer->eat();
    elseStatement = parseStatement(ctx, lexer, parsingFlags, error);
    if (!elseStatement) return NULL;
  }

  auto ifStatement = AST_ALLOC(ASTIfStatement, &ctx->allocator);
  ifStatement->fileIndex = lexer->fileIndex;
  ifStatement->offset0 = ifToken.offset0;
  if (elseStatement) {
    ifStatement->offset1 = elseStatement->offset1;
  } else {
    ifStatement->offset1 = thenStatement->offset1;
  }

  ifStatement->conditionExpr = conditionExpr;
  ifStatement->thenStatement = thenStatement;
  ifStatement->elseStatement = elseStatement;
  return ifStatement;
}

DEFINE_PARSER(parseWhileLoop) {
  MATCH_TOKEN(whileToken, TOKEN_TYPE_WHILE, "Expected 'while' keyword");

  auto conditionExpr = parseParenExpr(ctx, lexer, parsingFlags, error);
  if (!conditionExpr) return NULL;

  auto loopBody = parseStatement(ctx, lexer, parsingFlags, error);
  if (!loopBody) return NULL;

  auto loop = AST_ALLOC(ASTWhileLoop, &ctx->allocator);
  loop->fileIndex = lexer->fileIndex;
  loop->offset0 = whileToken.offset0;
  loop->offset1 = loopBody->offset1;
  loop->condition = conditionExpr;
  loop->body = loopBody;
  return loop;
}

DEFINE_PARSER(parseDeferStatement) {
  MATCH_TOKEN(deferToken, TOKEN_TYPE_DEFER, "Expected 'defer' keyword");
  auto statement = parseStatement(ctx, lexer, parsingFlags, error);
  if (!statement) return NULL;

  auto defer = AST_ALLOC(ASTDeferStatement, &ctx->allocator);
  defer->fileIndex = lexer->fileIndex;
  defer->offset0 = deferToken.offset0;
  defer->offset1 = statement->offset1;
  defer->statement = statement;
  return defer;
}

DEFINE_PARSER(parseExprStatement) {
  auto expr = parseExpr(ctx, lexer, parsingFlags, error);
  if (!expr) return NULL;
  MATCH_TOKEN(semicolon, TOKEN_TYPE_SEMICOLON, "Expected ';'");
  return expr;
}
