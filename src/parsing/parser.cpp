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

Str slice(Str src, uint32_t offset0, uint32_t offset1) {
  auto result = Str {
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
