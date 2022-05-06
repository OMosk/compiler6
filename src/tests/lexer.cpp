#include "../all.h"

bool expectToken(T *t, const char *file, int line, const char *funcName,
                     Lexer *lexer, TokenType wantType, const char *wantValue) {
  Token token = lexer->eat();
  if (token.type != wantType) {
    t->Printf("%s:%d:%s Expected token type %s, got %s at offset %u\n", file, line,
              funcName, toString(wantType), toString(token.type), token.offset0);
    t->Fail();
    return false;
  }

  int wantValueLength = strlen(wantValue);
  Str gotValue = Str{lexer->source.data + token.offset0,
                     token.offset1 - token.offset0};

  if (wantValueLength != gotValue.len ||
      memcmp(wantValue, gotValue.data, wantValueLength) != 0) {
    t->Printf("%s:%d:%s Token value mismatch, expected '%.*s', got '%.*s'\n",
              file, line, funcName, wantValueLength, wantValue,
              (int)gotValue.len, gotValue.data);
    t->Fail();
    return false;
  }
  return true;
}

TEST(LexingSimpleStruct) (T *t) {
  auto src = STR("MyStruct :: struct {\n\tfield1: type1;\n}\n");
  Lexer lexer = {.source = src};

  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_IDENTIFIER, "MyStruct")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_COLON_COLON, "::")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_STRUCT, "struct")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_LEFT_BRACE, "{")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_IDENTIFIER, "field1")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_COLON, ":")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_IDENTIFIER, "type1")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_SEMICOLON, ";")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_RIGHT_BRACE, "}")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_EOF, "")) return;
}

TEST(LexingRegressionClosingParen) (T *t) {
  auto src = STR("1 * (2 + 3)");
  Lexer lexer = {.source = src};

  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_NUMBER_LITERAL, "1")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_MULTIPLY, "*")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_LEFT_PAREN, "(")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_NUMBER_LITERAL, "2")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_PLUS, "+")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_NUMBER_LITERAL, "3")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_RIGHT_PAREN, ")")) return;
  if (!expectToken(t, __FILE__, __LINE__, __func__, &lexer, TOKEN_TYPE_EOF, "")) return;
}

TEST(LexingOffsetsAreCorrect) (T *t) {
  auto src = STR("1 + 2");
  Lexer lexer = {.source = src};

#define CHECK(GOT, WANTED)                                                     \
  do {                                                                         \
    auto got = (GOT);                                                          \
    auto wanted = (WANTED);                                                    \
    if (got != wanted) {                                                       \
      t->Printf("%s:%d Failed, got %u, wanted %u\n", __FILE__, __LINE__, got,  \
                wanted);                                                       \
    }                                                                          \
  } while (0)

  auto first = lexer.peek();
  CHECK(lexer.peek().offset0, 0);
  CHECK(lexer.eat().offset0, 0);

  CHECK(lexer.peek().offset0, 2);
  CHECK(lexer.eat().offset0, 2);

  CHECK(lexer.peek().offset0, 4);
  CHECK(lexer.eat().offset0, 4);

  lexer.reset(first);

  CHECK(lexer.peek().offset0, 0);
  CHECK(lexer.eat().offset0, 0);

  CHECK(lexer.peek().offset0, 2);
  CHECK(lexer.eat().offset0, 2);

  CHECK(lexer.peek().offset0, 4);
  CHECK(lexer.eat().offset0, 4);
}
