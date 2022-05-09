#pragma once

#include "../utils/string.h"

#define TOKENS_LIST                                                            \
  XX(TOKEN_TYPE_NULL)                                                          \
  XX(TOKEN_TYPE_UNEXPECTED_SEQUENCE_OF_CHARS)                                  \
  XX(TOKEN_TYPE_UNTERMINATED_STRING_LITERAL)                                   \
  XX(TOKEN_TYPE_EOF)                                                           \
  XX(TOKEN_TYPE_LINE_COMMENT)                                                  \
  XX(TOKEN_TYPE_STRING_LITERAL)                                                \
  XX(TOKEN_TYPE_NUMBER_LITERAL)                                                \
  XX(TOKEN_TYPE_DOT)                                                           \
  XX(TOKEN_TYPE_COMMA)                                                         \
  XX(TOKEN_TYPE_PLUS)                                                          \
  XX(TOKEN_TYPE_MINUS)                                                         \
  XX(TOKEN_TYPE_MULTIPLY)                                                      \
  XX(TOKEN_TYPE_DIVIDE)                                                        \
  XX(TOKEN_TYPE_EXCLAMATION_MARK)                                              \
  XX(TOKEN_TYPE_EXCLAMATION_MARK_EQUALS)                                       \
  XX(TOKEN_TYPE_AMPERSAND)                                                     \
  XX(TOKEN_TYPE_AMPERSAND_AMPERSAND)                                           \
  XX(TOKEN_TYPE_PIPE)                                                          \
  XX(TOKEN_TYPE_PIPE_PIPE)                                                     \
  XX(TOKEN_TYPE_CARET)                                                         \
  XX(TOKEN_TYPE_TILDE)                                                         \
  XX(TOKEN_TYPE_COLON_COLON)                                                   \
  XX(TOKEN_TYPE_COLON_EQUAL)                                                   \
  XX(TOKEN_TYPE_COLON)                                                         \
  XX(TOKEN_TYPE_SEMICOLON)                                                     \
  XX(TOKEN_TYPE_LEFT_BRACE)                                                    \
  XX(TOKEN_TYPE_RIGHT_BRACE)                                                   \
  XX(TOKEN_TYPE_LEFT_BRACKET)                                                  \
  XX(TOKEN_TYPE_RIGHT_BRACKET)                                                 \
  XX(TOKEN_TYPE_LEFT_PAREN)                                                    \
  XX(TOKEN_TYPE_RIGHT_PAREN)                                                   \
  XX(TOKEN_TYPE_LEFT_ANGLE_BRACKET)                                            \
  XX(TOKEN_TYPE_RIGHT_ANGLE_BRACKET)                                           \
  XX(TOKEN_TYPE_LEFT_ANGLE_BRACKET_LEFT_ANGLE_BRACKET)                         \
  XX(TOKEN_TYPE_RIGHT_ANGLE_BRACKET_RIGHT_ANGLE_BRACKET)                       \
  XX(TOKEN_TYPE_LEFT_ANGLE_BRACKET_EQUALS)                                     \
  XX(TOKEN_TYPE_RIGHT_ANGLE_BRACKET_EQUALS)                                    \
  XX(TOKEN_TYPE_EQUALS)                                                        \
  XX(TOKEN_TYPE_EQUALS_EQUALS)                                                 \
  XX(TOKEN_TYPE_PERCENT)                                                       \
  XX(TOKEN_TYPE_IDENTIFIER)                                                    \
  XX(TOKEN_TYPE_LOAD_DIRECTIVE)                                                \
  XX(TOKEN_TYPE_IF)                                                            \
  XX(TOKEN_TYPE_ELSE)                                                          \
  XX(TOKEN_TYPE_WHILE)                                                         \
  XX(TOKEN_TYPE_DEFER)                                                         \
  XX(TOKEN_TYPE_FUNC)                                                          \
  XX(TOKEN_TYPE_STRUCT)

#define XX(TYPE) TYPE,
enum TokenType { TOKENS_LIST };
#undef XX

const char *toString(TokenType tokenType);

struct Token {
  TokenType type;
  uint32_t offset0, offset1;
};

struct Lexer {
  bool hasBufferedToken;
  Token bufferedToken;

  uint32_t fileIndex;
  Str source;
  uint32_t offset;

  Token eat();
  Token peek();
  void reset(Token to);
};
