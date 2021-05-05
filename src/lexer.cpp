#include "lexer.hpp"
#include "utils.hpp"

namespace lexer {

#define ADVANCE(result, it, it_loc, prev_it, end) \
do { \
  (prev_it) = (it); \
  (result) = utf8advance(&(it), (end)); \
  (it_loc)->col += 1;\
  if ((*prev_it) == '\n') {\
    (it_loc)->line += 1; \
    (it_loc)->col = 1;\
  }\
} while (0)

void SkipWhitespace(char **it, Site *it_loc, char *end) {
  char *tmp_it = *it;
  Site tmp_it_loc = *it_loc;
  char *tmp_it_prev;
  int32_t ch;
  do {
    Site tmp_it_loc_prev = tmp_it_loc;
    ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
    switch (ch) {
      case ' ':
      case '\t':
      case '\n':
        break;
      default:
        *it = tmp_it_prev;
        *it_loc = tmp_it_loc_prev;
        return;
    }
  } while (true);
}

bool MatchLineComment(char **it, Site *it_loc, char *begin, char *end, Token *t) {
  char *tmp_it = *it;
  Site tmp_it_loc = *it_loc;
  char *tmp_it_prev;
  int32_t ch;
  ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
  if (ch != '/') {
    return false;
  }
  ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
  if (ch != '/') {
    return false;
  }
  t->begin = tmp_it - begin;

  do {
    ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
  } while (ch != '\n');

  t->length = tmp_it - (begin + t->begin);
  //t->loc.start = *it;
  //t->loc.end = tmp_it;
  t->tokenType = TokenLineComment;
  *it = tmp_it;
  *it_loc = tmp_it_loc;
  return true;
}

bool MatchIdentifierOrKeyword(char **it, Site *it_loc, char *begin, char *end, Token *t) {
  char *tmp_it = *it;
  Site tmp_it_loc = *it_loc, tmp_it_loc_prev;
  char *tmp_it_prev;
  int32_t ch;

  int matchedCharacters = 0;
  do {
    tmp_it_loc_prev = tmp_it_loc;
    ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
    bool isStart = matchedCharacters == 0;
    bool validStartCharacter = (ch == '_' || ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));
    bool validRestCharacter = (ch == '_' || ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ('0' <= ch && ch <= '9'));

    if ((isStart && validStartCharacter) || (!isStart && validRestCharacter)) {
      matchedCharacters += 1;
    } else {
      break;
    }
  } while (true);

  if (matchedCharacters == 0) {
    return false;
  }

  t->begin = *it - begin;
  t->length = tmp_it_prev - (begin + t->begin);

  //t->loc.start = *it;
  //t->loc.end = tmp_it_prev;

  t->tokenType = TokenIdentifier;
  *it = tmp_it_prev;
  *it_loc = tmp_it_loc_prev;
#define KEYWORD(S, TYPE) do { if (t->length == sizeof(S)-1 && memcmp(begin + t->begin, S, sizeof(S) - 1) == 0) {\
  t->tokenType = TYPE;\
}} while (0)

    KEYWORD("return", TokenReturn);
    KEYWORD("if", TokenIf);
    KEYWORD("else", TokenElse);
    KEYWORD("while", TokenWhile);
    KEYWORD("continue", TokenContinue);
    KEYWORD("break", TokenBreak);
    KEYWORD("struct", TokenStruct);
    KEYWORD("nil", TokenNil);
    KEYWORD("defer", TokenDefer);
    KEYWORD("function", TokenFunction);
    KEYWORD("var", TokenVar);
#undef KEYWORD


  //printf("Matched identifer '%.*s'\n", (int)t->value.len, t->value.s);
  return true;
}

bool MatchDirectives(char **it, Site *it_loc, char *begin, char *end, Token *t) {

  auto *tmp_it = *it;
  Site tmp_it_loc = *it_loc;
  char *tmp_it_prev;
  int32_t ch;
  auto tmp_it_loc_prev = tmp_it_loc;
  ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
  if (ch == '#') {
    if (MatchIdentifierOrKeyword(&tmp_it, &tmp_it_loc, begin, end, t)) {
      /*if (t->tokenType == TokenIdentifier)*/ {
        #define CASE(S, TokenType) \
          if ((sizeof(S)-1) == t->length && memcmp(begin + t->begin, S, sizeof(S)-1) == 0) {\
            *it = tmp_it;\
            *it_loc = tmp_it_loc;\
            t->tokenType = TokenType;\
            return true;\
          }
        CASE("foreign", TokenForeign)
        CASE("load", TokenLoad)
        CASE("opaque", TokenOpaque)
        CASE("extern", TokenExtern)
        CASE("export_scope", TokenExportScope)
        CASE("module_scope", TokenModuleScope)
        #undef CASE
        showError(tmp_it_loc_prev, "Unknown directive %.*s", (int)t->length, begin + t->begin);
        showCodeLocation(tmp_it_loc_prev);
        //if (t->value)
      }
    }
  }

  return false;
}

bool MatchNumber(char **it, Site *it_loc, char *begin, char *end, Token *t) {
  char *tmp_it = *it;
  Site tmp_it_loc = *it_loc, tmp_it_loc_prev;
  char *tmp_it_prev;
  int32_t ch;

  int matches = 0;
  do {
    tmp_it_loc_prev = tmp_it_loc;
    ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
    auto isDigit = ('0' <= ch && ch <= '9');
    if (!isDigit) {
      break;
    }
    matches += 1;
  } while (true);

  if (matches == 0) {
    return false;
  }

  auto dotLoc = tmp_it_prev;

  if (ch == '.') {
    int fracLen = 0;
    do {
      tmp_it_loc_prev = tmp_it_loc;
      ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
      auto isDigit = ('0' <= ch && ch <= '9');
      if (!isDigit) {
        break;
      }
      fracLen += 1;
    } while (true);

    //NOTE: rollback one character back if directly after '.' goes non-digit character
    //to allow tokenizer understand 5.(f32) as cast of 5 to f32 and not procedure call
    if (fracLen == 0) {
      tmp_it_prev = dotLoc;
    }
  }

  t->begin = *it - begin;
  t->length = tmp_it_prev - (begin + t->begin);

  //t->loc.start = *it;
  //t->loc.end = tmp_it_prev;

  t->tokenType = TokenNumber;
  *it = tmp_it_prev;
  *it_loc = tmp_it_loc_prev;

  //printf("Matched number '%.*s'\n", (int)t->value.len, t->value.s);
  return true;
}

bool MatchCharSequence(char **it, Site *it_loc, char *begin, char *end, Token *t,
    const char *seq, int token_type) {
  char *tmp_it = *it;
  Site tmp_it_loc = *it_loc;
  char *tmp_it_prev;
  int32_t ch;

  const char *seq_it = seq;
  while (*seq_it) {
    ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
    if (ch != *seq_it) {
      return false;
    }
    seq_it += 1;
  }

  t->begin = *it - begin;
  t->length = tmp_it - *it;

  //t->loc.start = *it;
  //t->loc.end = tmp_it;

  t->tokenType = token_type;
  *it = tmp_it;
  *it_loc = tmp_it_loc;
  return true;
}

bool MatchEOF(char **it, Site *it_loc, char *begin, char *end, Token *t) {
  char eof[] = {-1, 0};
  return MatchCharSequence(it, it_loc, begin, end, t, eof, TokenEOF);
}

bool MatchStringLiteral(char **it, Site *it_loc, char *begin, char *end, Token *t) {
  char *tmp_it = *it;
  Site tmp_it_loc = *it_loc;
  char *tmp_it_prev;
  int32_t ch;

  ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
  if (ch != '"') {
    return false;
  }
  char *content_start = tmp_it;
  char *write_it = tmp_it;

  bool should_continue = true;
  int end_offset = 0;
  do {
    auto tmp_it_loc_prev = tmp_it_loc;
    ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
    if (ch == '\\') {
      //char *write_it = tmp_it_prev;
      ADVANCE(ch, tmp_it, &tmp_it_loc, tmp_it_prev, end);
      switch (ch) {
        case 'a': {
          *write_it = '\a';
        } break;
        case 'b': {
          *write_it = '\b';
        } break;
        case 'r': {
          *write_it = '\r';
        } break;
        case 'n': {
          *write_it = '\n';
        } break;
        case 't': {
          *write_it = '\t';
        } break;
        case 'v': {
          *write_it = '\v';
        } break;
        case '\\': {
          *write_it = '\\';
        } break;
        case '"': {
          *write_it = '"';
        } break;
        case '\'': {
          *write_it = '\'';
        } break;
        case '0': {
          *write_it = '\0';
        } break;
        default: {
          showError(tmp_it_loc_prev, "unexpected escape sequence \\%c", ch);
          showCodeLocation(tmp_it_loc_prev);
          return false;
        }
      }
      end_offset += 1;
      write_it++;
      continue;
    }
    switch (ch) {
      case '"': should_continue = false; break;
      case '\n': {
        showError(tmp_it_loc_prev, "unexpected new line in string literal", ch);
        showCodeLocation(tmp_it_loc_prev);
        return false;
      } break;
      case EOF: {
        showError(tmp_it_loc_prev, "unexpected end-of-file in string literal", ch);
        showCodeLocation(tmp_it_loc_prev);
        return false;
      } break;
      default: {
        while (tmp_it_prev < tmp_it) {
          *write_it = *tmp_it_prev;
          ++write_it;
          ++tmp_it_prev;
        }
      }
    }
  } while (should_continue);

  char *content_end = tmp_it_prev;

  t->begin = content_start - begin;
  t->length = (content_end - content_start) - end_offset;

  //t->loc.start = *it;
  //t->loc.end = tmp_it;

  t->tokenType = TokenStringLiteral;
  *it = tmp_it;
  *it_loc = tmp_it_loc;

  //printf("Matched string '%.*s'\n", (int)t->value.len, t->value.s);
  return true;
}

} //namespace lexer

Tokens tokenize(Str content, const char *abs_filename, const char *relative_filename) {
  using namespace lexer;

  Tokens result = {};
  reserve(&result.arr, 64);

  FileEntry entry = {};
  entry.absolute_path = abs_filename;
  entry.relative_path = relative_filename;
  //TODO: investigate why we need to make a copy of file content
  // - probably character escaping
  entry.content = StrDup(content);

  result.fileIndex = files.len;
  result.fileContent = content;
  append(&files, entry);

  char *it = content.data;
  Site it_loc, it_loc_prev;
  it_loc.line = 1;
  it_loc.col = 1;
  char *it_prev = it;
  char *end = content.data + content.len;
  Token token = {};

  while (it < end) {
    token.tokenType = 0;
    token.begin = 0;
    token.length = 0;
    //token.value = {};

    SkipWhitespace(&it, &it_loc, end);
    token.line = it_loc.line;
    token.col = it_loc.col;
#define M(x) if (x) goto handle_match
#define Seq(s, type) M(MatchCharSequence(&it, &it_loc, content.data, end, &token, s, type))
#define KEYWORD(S, TYPE) M(MatchKeyword(&it, &it_loc, content.data, end, &token, S, sizeof(S) - 1, TYPE))

    //M(MatchNewlines(&it, &it_loc, end, &token));
    M(MatchLineComment(&it, &it_loc, content.data, end, &token));

    Seq("[", '[');
    Seq("]", ']');
    Seq("^", '^');

    Seq("==", TokenEq);
    Seq("!=", TokenNEq);
    Seq("=", '=');

    Seq("<<", TokenShl);
    Seq(">>", TokenShr);

    Seq("<=", TokenLE);
    Seq(">=", TokenGE);
    Seq("<", '<');
    Seq(">", '>');

    Seq("!", '!');
    Seq("~", '~');

    Seq("&&", TokenLogicalAnd);
    Seq("&", '&');

    Seq("||", TokenLogicalOr);
    Seq("|", '|');

    Seq("%", '%');
    Seq("*", '*');
    Seq("/", '/');
    Seq("+", '+');
    Seq("-", '-');

    Seq(":=", TokenDefine);
    Seq("::", TokenDeclare);
    Seq(":", ':');
    Seq(";", ';');
    Seq("(", '(');
    Seq(")", ')');
    Seq("{", '{');
    Seq("}", '}');

    M(MatchNumber(&it, &it_loc, content.data, end, &token));

    M(MatchDirectives(&it, &it_loc, content.data, end, &token));

    M(MatchIdentifierOrKeyword(&it, &it_loc, content.data, end, &token));
    M(MatchStringLiteral(&it, &it_loc, content.data, end, &token));
    Seq("...", TokenVariadic);
    Seq(".", '.');
    Seq(",", ',');
    M(MatchEOF(&it, &it_loc, content.data, end, &token));
#undef Seq
#undef M

    showError(it_loc, "None of the tokenizer matchers matched");
    showCodeLocation(it_loc);
    result.ok = false;
    return result;

    handle_match:

    if (token.tokenType == TokenLineComment) {
      continue;
    }
    append(&result.arr, token);
  }


  result.ok = true;
  return result;
}


void debugPrintTokens(Tokens tokens, FILE *output) {
  for (int i = 0; i < tokens.arr.len; ++i) {
    Token *t = tokens.arr.data + i;
    fprintf(output, "[%3d] at %3d:%-3d ", i, t->line, t->col);

    char singleCharType[] = {0, 0};
    const char *type = "";
    switch (t->tokenType) {
    #define S(t) case t: type = #t; break
      S(TokenEOF);
      S(TokenLineComment);
      S(TokenIdentifier);
      S(TokenDeclare);
      S(TokenEq);
      S(TokenNEq);
      S(TokenShl);
      S(TokenShr);
      S(TokenLE);
      S(TokenGE);
      S(TokenLogicalOr);
      S(TokenLogicalAnd);
      S(TokenDefine);
      S(TokenNumber);
      S(TokenStringLiteral);
      S(TokenReturn);
      S(TokenForeign);
      S(TokenVariadic);
      S(TokenIf);
      S(TokenElse);
      S(TokenWhile);
      S(TokenBreak);
      S(TokenContinue);

      S(TokenLoad);
      S(TokenStruct);
      S(TokenOpaque);
      S(TokenExtern);
      S(TokenNil);
      S(TokenDefer);
      S(TokenExportScope);
      S(TokenModuleScope);
      S(TokenFunction);
      S(TokenVar);
  #undef S
    case '\n': type = "\\n"; break;
    default:
      if (t->tokenType < 0) {
        NOT_IMPLEMENTED;
      }
      singleCharType[0] = t->tokenType;
      type = singleCharType;
    }
    fprintf(output, "type = '%s' value = '", type);
    for (int i = 0; i < t->length; ++i) {
      //auto ch = t->value.data[i];
      auto ch = tokens.fileContent.data[t->begin + i];
      switch (ch) {
      case '\n': {
        fprintf(output, "\\n");
      } break;
      default:
        fprintf(output, "%c", ch);
      }
    }
    fprintf(output, "' \n");
  }
}

