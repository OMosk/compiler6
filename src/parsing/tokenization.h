#pragma once

#include "../utils/file-entry.h"
#include "../utils/string.h"

#define TOKENS_LIST                                                            \
  XX(TOKEN_TYPE_NO_TOKEN)                                                      \
  XX(TOKEN_TYPE_EOF)                                                           \
  XX(TOKEN_TYPE_LINE_COMMENT)                                                  \
  XX(TOKEN_TYPE_COLON_COLON)                                                   \
  XX(TOKEN_TYPE_COLON_EQUAL)                                                   \
  XX(TOKEN_TYPE_COLON)                                                         \
  XX(TOKEN_TYPE_SEMICOLON)                                                     \
  XX(TOKEN_TYPE_LEFT_BRACE)                                                    \
  XX(TOKEN_TYPE_RIGHT_BRACE)                                                   \
  XX(TOKEN_TYPE_IDENTIFIER)                                                    \
  XX(TOKEN_TYPE_STRUCT)

#define XX(TYPE) TYPE,
enum TokenType {
  TOKENS_LIST
};
#undef XX

const char * toString(TokenType tokenType);

struct Token {
  TokenType type;
  uint32_t offset0, offset1;
};

struct Lexer {
  bool hasBufferedToken;
  Token bufferedToken;

  FileEntry fileEntry;
  uint32_t offset;

  Token eat();
  Token peek();
  void reset(Token to);
};
