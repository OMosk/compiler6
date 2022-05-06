#include "tokenization.h"

#include "../utils/utf8.h"

Token findNextToken(Str content, uint32_t offset);

Token Lexer::peek() {
  if (this->hasBufferedToken) {
    return this->bufferedToken;
  }

  auto nextToken = findNextToken(this->source, this->offset);
  this->bufferedToken = nextToken;
  this->hasBufferedToken = true;
  this->offset = nextToken.offset1;

  return nextToken;
}

Token Lexer::eat() {
  if (!this->hasBufferedToken) {
    this->peek();
  }
  this->hasBufferedToken = false;
  return this->bufferedToken;
}

void Lexer::reset(Token to) {
  this->bufferedToken = to;
  this->hasBufferedToken = true;
  this->offset = to.offset1;
}


bool isAlpha(int ch) {
  auto result = (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_');
  return result;
}
bool isNum(int ch) {
  auto result = ('0' <= ch && ch <= '9');
  return result;
}
bool isAlphaNum(int ch) {
  auto result = isAlpha(ch) || isNum(ch);
  return result;
}
bool isWhiteSpace(int ch) {
  auto result = (ch == ' ' || ch == '\t' || ch == '\n');
  return result;
}

bool hasPrefix(const char *str, const char *prefix) {
  while (*str == *prefix && *str) { str++; prefix++; }
  if (*prefix != '\0') return false;
  return true;
}

// Token matchLineComment(Str content, uint32_t offset);
Token matchIdentifierOrKeyword(Str content, uint32_t offset);
Token matchStringLiteral(Str content, uint32_t offset);
Token matchNumberLiteral(Str content, uint32_t offset);

Token findNextToken(Str content, uint32_t offset) {
  while (offset < content.len && isWhiteSpace(content.data[offset])) offset++;

  if (offset == content.len) return {TOKEN_TYPE_EOF, offset, offset};

  #define IF_PREFIX_RETURN(LITERAL, TYPE) \
    do { \
      if (hasPrefix(content.data + offset, (LITERAL))) \
        return {(TYPE), offset, offset + static_cast<uint32_t>(sizeof(LITERAL)-1)}; \
    } while(0)

  IF_PREFIX_RETURN(".", TOKEN_TYPE_DOT);
  IF_PREFIX_RETURN(",", TOKEN_TYPE_COMMA);
  IF_PREFIX_RETURN("+", TOKEN_TYPE_PLUS);
  IF_PREFIX_RETURN("-", TOKEN_TYPE_MINUS);
  IF_PREFIX_RETURN("*", TOKEN_TYPE_MULTIPLY);
  IF_PREFIX_RETURN("/", TOKEN_TYPE_DIVIDE);
  IF_PREFIX_RETURN("!=", TOKEN_TYPE_EXCLAMATION_MARK_EQUALS);
  IF_PREFIX_RETURN("!", TOKEN_TYPE_EXCLAMATION_MARK);
  IF_PREFIX_RETURN("&&", TOKEN_TYPE_AMPERSAND_AMPERSAND);
  IF_PREFIX_RETURN("&", TOKEN_TYPE_AMPERSAND);
  IF_PREFIX_RETURN("||", TOKEN_TYPE_PIPE_PIPE);
  IF_PREFIX_RETURN("|", TOKEN_TYPE_PIPE);
  IF_PREFIX_RETURN("~", TOKEN_TYPE_TILDE);
  IF_PREFIX_RETURN("^", TOKEN_TYPE_CARET);
  IF_PREFIX_RETURN("==", TOKEN_TYPE_EQUALS_EQUALS);
  IF_PREFIX_RETURN("=", TOKEN_TYPE_EQUALS);
  IF_PREFIX_RETURN("<<", TOKEN_TYPE_LEFT_ANGLE_BRACKET_LEFT_ANGLE_BRACKET);
  IF_PREFIX_RETURN("<", TOKEN_TYPE_LEFT_ANGLE_BRACKET);
  IF_PREFIX_RETURN(">>", TOKEN_TYPE_RIGHT_ANGLE_BRACKET_RIGHT_ANGLE_BRACKET);
  IF_PREFIX_RETURN(">", TOKEN_TYPE_RIGHT_ANGLE_BRACKET);
  IF_PREFIX_RETURN("<=", TOKEN_TYPE_LEFT_ANGLE_BRACKET_EQUALS);
  IF_PREFIX_RETURN(">=", TOKEN_TYPE_RIGHT_ANGLE_BRACKET_EQUALS);
  IF_PREFIX_RETURN("%", TOKEN_TYPE_PERCENT);

  IF_PREFIX_RETURN("::", TOKEN_TYPE_COLON_COLON);
  IF_PREFIX_RETURN(":=", TOKEN_TYPE_COLON_EQUAL);
  IF_PREFIX_RETURN(":", TOKEN_TYPE_COLON);
  IF_PREFIX_RETURN(";", TOKEN_TYPE_SEMICOLON);
  IF_PREFIX_RETURN("{", TOKEN_TYPE_LEFT_BRACE);
  IF_PREFIX_RETURN("}", TOKEN_TYPE_RIGHT_BRACE);
  IF_PREFIX_RETURN("[", TOKEN_TYPE_LEFT_BRACKET);
  IF_PREFIX_RETURN("]", TOKEN_TYPE_RIGHT_BRACKET);
  IF_PREFIX_RETURN("(", TOKEN_TYPE_LEFT_PAREN);
  IF_PREFIX_RETURN(")", TOKEN_TYPE_RIGHT_PAREN);

  if (hasPrefix(content.data + offset, "\"")) return matchStringLiteral(content, offset);
  if (isNum(content.data[offset])) return matchNumberLiteral(content, offset);
  if (isAlpha(content.data[offset])) return matchIdentifierOrKeyword(content, offset);

  //if (hasPrefix(content.data + offset, "//")) return 
  #undef IF_PREFIX_RETURN

  return {TOKEN_TYPE_UNEXPECTED_SEQUENCE_OF_CHARS, offset, offset};
}

Token matchIdentifierOrKeyword(Str content, uint32_t offset) {
  uint32_t offset1 = offset;

  // Following check can be skipped, because we performed check on calling side
  // before calling this function

  // if (!isAlpha(content.data[offset1]) return {TYPE_TOKEN_NO_TOKEN, offset, offset1}

  while (offset1 < content.len && isAlphaNum(content.data[offset1])) offset1++;

  if (strncmp(content.data + offset, "struct", offset1 - offset) == 0) return {TOKEN_TYPE_STRUCT, offset, offset1};

  return {TOKEN_TYPE_IDENTIFIER, offset, offset1};
}

// TypeOffset matchLineComment(Str content, uint32_t offset) {
// }

Token matchStringLiteral(Str content, uint32_t offset) {
  for (uint32_t offset1 = offset + 1; offset1 < content.len; ++offset1) {
    if (content.data[offset1 - 1] != '\\' && content.data[offset1] == '"') {
      return {TOKEN_TYPE_STRING_LITERAL, offset, offset1 + 1};
    }
  }
  return {TOKEN_TYPE_UNTERMINATED_STRING_LITERAL, offset, static_cast<uint32_t>(content.len)};
}

Token matchNumberLiteral(Str content, uint32_t offset) {
  uint32_t offset1 = offset + 1;
  while (isNum(content.data[offset1])) offset1++;

  if (content.data[offset1] == '.' && isNum(content.data[offset1 + 1])) {
    offset1++;
    while (isNum(content.data[offset1])) offset1++;
  }

  return {TOKEN_TYPE_NUMBER_LITERAL, offset, offset1};
}

const char * toString(TokenType tokenType) {
  switch (tokenType) {
  #define XX(TokenType) case TokenType: return #TokenType;
    TOKENS_LIST
  #undef XX
    default: return "<UNKNOWN>";
  }
}

