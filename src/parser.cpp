#include "parser.hpp"

#include <stdio.h>

namespace parser {

#define T(I) (tokens->arr.data + (I))
#define TT(I) ((tokens->arr.data + (I))->tokenType)

void fillError(Error *error, int fileIndex, Token *t, int tokenIndex, Str message) {
  error->site.file = fileIndex;
  error->site.col = t->col;
  error->site.line = t->line;
  error->tokenIndex = tokenIndex;
  error->message = message;
}

#define FILL_ERROR(MSG) fillError(error, tokens->fileIndex, T(i), i, STR(MSG))
#define SITE(I) Site{.line = T(I)->line, .col = T(I)->col, .file = (uint16_t)tokens->fileIndex}

ASTNode *parseExpression(Tokens *tokens, int *outI, Error *error);
ASTNode *parseType(Tokens *tokens, int *outI, Error *error);

ASTNode *parseUnaryExpression(Tokens *tokens, int *outI, Error *error) {
  int i = *outI;

  ASTNode *topLevel = NULL;
  ASTUnaryOp *lastOp = NULL;

  for (bool shouldContinue = true; shouldContinue;) {
    switch (TT(i)) {
      case '+':
      case '-':
      case '&':
      case '*':
      case '~':
      case '!': {
        ASTUnaryOp *op = ASTALLOC(ASTUnaryOp, i);
        op->op = TT(i);
        if (lastOp) ((ASTUnaryOp *)lastOp)->operand = op;
        lastOp = op;
        if (!topLevel) topLevel = op;
        i++;
      } break;
      default: {
        shouldContinue = false;
      }
    }
  }

  {
    ASTNode *bottom = NULL;
    switch (TT(i)) {
    case '(': {
      bottom = parseExpression(tokens, &i, error);
      if (!bottom) return NULL;
    } break;
    case TokenIdentifier: {
      ASTIdentifier *ident = ASTALLOC(ASTIdentifier, i);
      ident->name = T(i)->value;
      i++;
      bottom = ident;
    } break;
    case TokenNumber: {
      ASTLiteral *literal = ASTALLOC(ASTLiteral, i);
      literal->raw = T(i)->value;
      literal->literalKind = LiteralKindNumber;
      i++;
      bottom = literal;
    } break;
    case TokenStringLiteral: {
      ASTLiteral *literal = ASTALLOC(ASTLiteral, i);
      literal->raw = T(i)->value;
      literal->literalKind = LiteralKindString;
      i++;
      bottom = literal;
    } break;
    case TokenNil: {
      ASTNil *nil = ASTALLOC(ASTNil, i);
      i++;
      bottom = nil;
    } break;
    default: {
      FILL_ERROR("Expected expression operand");
      return NULL;
    };
    }

    for (bool shouldContinue = true; shouldContinue;) {
      switch (TT(i)) {
      case '.': {
        if (TT(i+1) == '(') {
          //Cast
          i++;
          ASTNode *type = parseType(tokens, &i, error);
          if (!type) return NULL;

          if (TT(i) != ')') {
            FILL_ERROR("Expected closing ')' in a cast");
            return NULL;
          }
          i++;
          ASTCast *cast = ASTALLOC(ASTCast, i);
          cast->operand = bottom;
          cast->type = type;
          bottom = cast;
        } else {
          ASTMemberAccess *ma = ASTALLOC(ASTMemberAccess, i);
          i++;
          //Member dereference
          if (TT(i) != TokenIdentifier) {
            FILL_ERROR("Expected identifier in member dereference");
            return NULL;
          }
          ma->container = bottom;
          ma->name = T(i)->value;
          i++;
          bottom = ma;
        }
      } break;
      case '[': {
        ASTSubscript *subscript = ASTALLOC(ASTSubscript, i);
        i++;
        ASTNode *index = parseExpression(tokens, &i, error);
        if (!index) return NULL;
        subscript->container = bottom;
        subscript->index = index;
        bottom = subscript;
        if (TT(i) != ']') {
          FILL_ERROR("Expected closing ']' in subscript operator");
          return NULL;
        }
        i++;
      } break;
      case '(': {
        //call
        NOT_IMPLEMENTED;
      } break;
      default: {
        shouldContinue = false;
        //NOT_IMPLEMENTED;
      }
      }
    }

    if (lastOp) lastOp->operand = bottom;
    if (!topLevel) topLevel = bottom;
  }

  *outI = i;
  return topLevel;
}

ASTNode *parseExpression(Tokens *tokens, int *outI, Error *error) {
  int i = *outI;
  ASTNode *lhs = NULL;
  if (TT(i) == '(') {
    i++;
    lhs = parseExpression(tokens, &i, error);
    if (TT(i) != ')') {
      FILL_ERROR("Expected closing ')' at the end of expression");
      return NULL;
    }
    i++;
    lhs->flags |= FlagsExprInParentheses;
  } else {
    lhs = parseUnaryExpression(tokens, &i, error);
  }

  for (bool shouldContinue = true; shouldContinue;) {
    switch (TT(i)) {
    default: {
      shouldContinue = false;
    }
    }
  }

  *outI = i;
  return lhs;
}

ASTNode *parseType(Tokens *tokens, int *outI, Error *error) {
  int i = *outI;

  ASTNode *result = NULL;
  ASTNode *prev = NULL;
  bool prevExpectsSuccessor = false;

  do {
    ASTNode *tmp = NULL;
    switch (TT(i)) {

      case '*': {
        auto ptr = ASTALLOC(ASTPointerTo, i);
        tmp = ptr;
        i++;
        prevExpectsSuccessor = true;
      } break;

      case TokenIdentifier: {
        auto ident = ASTALLOC(ASTIdentifier, i);
        ident->name = T(i)->value;
        tmp = ident;
        i++;
        prevExpectsSuccessor = false;
      } break;

      default: {
        FILL_ERROR("Unexpected token in type");
        return NULL;
      } break;
    }

    if (prev) {
      switch (prev->kind) {

        case ASTPointerTo_: {
          auto ptr = (ASTPointerTo *) prev;
          ptr->pointsTo = tmp;
        } break;

      }
    }

    prev = tmp;
    if (!result) result = tmp;
  } while (prevExpectsSuccessor);

  if (result) {
    *outI = i;
  } else {
    FILL_ERROR("Expected type");
  }

  return result;
}

ASTVariableDefinition *parseLongVariableDifinition(
  Tokens *tokens, int *outI, Error *error) {
  int i = *outI;
  if (TT(i) != TokenVar) {
    FILL_ERROR("Expected var in long variable definition");
    return NULL;
  }

  ASTVariableDefinition *varDefn = ASTALLOC(ASTVariableDefinition, i);

  i++;

  int expectVars = 0;
  {
    int oldIValue = i;
    for (;;) {
      if (TT(i) == TokenIdentifier) {
        expectVars++;
      } else {
        FILL_ERROR("Expected name in variable definition");
        return NULL;
      }
      i++;
      if (TT(i) == ',') {
        i++;
        continue;
      } else {
        break;
      }
    }
    i = oldIValue;
  }

  resize(&varDefn->vars, expectVars);
  for (int varNo = 0; varNo < expectVars; ++varNo) {
    varDefn->vars[varNo] = ASTALLOC(ASTVariable, i);
    varDefn->vars[varNo]->name = T(i)->value;
    i++;
    if (TT(i) == ',') i++;
  }

  ASTNode *varsType = parseType(tokens, &i, error);
  if (!varsType) return NULL;

  for (int varNo = 0; varNo < varDefn->vars.len; ++varNo) {
    varDefn->vars[varNo]->type = varsType;
  }

  if (TT(i) == '=') {
    i++;

    for (int varNo = 0; varNo < expectVars; ++varNo) {
      //TODO: continue here
      //parsing init expressions for long variables defn
      ASTNode *initExpr = parseExpression(tokens, &i, error);
      if (!initExpr) {
        FILL_ERROR("Expected initializer expression");
        return NULL;
      }
      varDefn->vars[varNo]->initExpr = initExpr;
      if (TT(i) == ',') {
        i++;
      } else {
        if (varNo == expectVars - 1) {
          //It's ok, that was the last one
        } else {
          FILL_ERROR("Expected ',' between variable initialization expressions");
          return NULL;
        }
      }
    }
  }

  if (TT(i) == ';') {
    i++;
  } else {
    FILL_ERROR("Expected ';' at the end of variable definition statement");
    return NULL;
  }

  *outI = i;
  return varDefn;
}

ASTNode *parseStatement(Tokens *tokens, int *outI, Error *error) {
  int i = *outI;
  ASTNode *result = NULL;
  switch (T(i)->tokenType) {
  case TokenVar: {
    result = parseLongVariableDifinition(tokens, &i, error);
  } break;
  default: {
    NOT_IMPLEMENTED;
  }
  }
  *outI = i;
  return result;
}

ASTBlock *parseBlock(Tokens *tokens, int *outI, Error *error) {
  int i = *outI;
  if (TT(i) == '{') {
    ++i;
  } else {
    FILL_ERROR("Expecting opening '{' in a block");
    return NULL;
  }
  auto block = ASTALLOC(ASTBlock, i-1);

  while (TT(i) != '}') {
    ASTNode *stmt = parseStatement(tokens, &i, error);
    if (!stmt) return NULL;
    append(&block->statements, stmt);
  }

  if (TT(i) == '}') {
    ++i;
  } else {
    FILL_ERROR("Expecting closing '}' in a block");
    return NULL;
  }
  *outI = i;
  return block;
}

ASTProc *parseProc(Tokens *tokens, int *outI, Error *error) {
  int i = *outI;
  ASTProc *proc = ASTALLOC(ASTProc, i);
  proc->site = SITE(i);
  i++;

  if (T(i)->tokenType == TokenIdentifier) {
    proc->name = T(i)->value;
    i++;
  }

  if (T(i)->tokenType == '(') {
    i++;
  } else {
    FILL_ERROR("Expected opening '(' in function declaration");
    return NULL;
  }

  int startArgumentWithSameType = 0;
  int endArgumentWithSameType = 0;
  while (T(i)->tokenType != ')') {
    if (TT(i) == TokenVariadic) {
      proc->flags |= FlagsProcVariadic;
      ++i;
      if (TT(i) != ')') {
        FILL_ERROR("Variadic arguments should be last in function declaration");
        return NULL;
      }
      continue;
    } else {
      for (;;) {
        if (T(i)->tokenType == TokenIdentifier) {
          auto arg = ASTALLOC(ASTProcArgument, i);
          arg->name = T(i)->value;
          append(&proc->arguments, arg);
          endArgumentWithSameType++;
          i++;
        } else {
          FILL_ERROR("Expected function argument name");
          return NULL;
        }

        if (T(i)->tokenType == ',') {
          i++;
        } else {
          break;
        }
      }

      auto type = parseType(tokens, &i, error);
      if (!type) {
        //FILL_ERROR("Expected type for function argument(s)");
        return NULL;
      }
      for (int i = startArgumentWithSameType; i < endArgumentWithSameType; ++i) {
        proc->arguments.data[i]->type = type;
      }

      if (TT(i) == ',') {
        ++i;
      }
    }
  }
  i++; // ')'

  for (;;) {
    auto type = parseType(tokens, &i, error);
    if (!type) {
      break;
    }
    append(&proc->returnTypes, type);

    if (T(i)->tokenType == ',') {
      i++;
    } else {
      break;
    }
  }

  if (TT(i) == TokenForeign) {
    proc->flags |= FlagsProcForeign;
    i++;

    if (TT(i) != ';') {
      FILL_ERROR("Expected ';' at the of foreign function declaration");
      return NULL;
    }
    ++i;
  } else {
    if (TT(i) != '{') {
      FILL_ERROR("Expected body of function");
      return NULL;
    }
    auto body = parseBlock(tokens, &i, error);
    if (!body) {
      return NULL;
    }
    proc->body = body;
  }

  *outI = i;
  return proc;
}

ASTFile *parseFile(Tokens *tokens, Error *error) {
  ASTFile *file = ASTALLOC(ASTFile, 0);

  int i = 0;
  while (tokens->arr.data[i].tokenType != TokenEOF) {
    Token *t = tokens->arr.data + i;
    ASTNode *node = NULL;

    switch (t->tokenType) {

    case TokenFunction: {
      auto fn = parseProc(tokens, &i, error);
      node = fn;
    } break;

    default: {
      FILL_ERROR("Unexpected top-level declaration");
    } break;

    }

    if (!node) return NULL;
    append(&file->declarations, node);
  }

  return file;
}

} //namespace parser

ASTFile *parseFile(Tokens tokens, Error *error) {
  return parser::parseFile(&tokens, error);
}

void printSpace(FILE *out, int width) {
  for (int i = 0; i < width; ++i) fputc(' ', out);
}

void printTree(FILE *out, int width) {
  for (int i = 0; i < width; ++i) fputc(' ', out);
}

void debugPrintAST(ASTNode *root, FILE *out, int shiftWidth, int currentShift, int firstLineShift) {
  printSpace(out, firstLineShift);
  switch (root->kind) {
  case ASTFile_: {
    ASTFile *file = (ASTFile *) root;
    fprintf(out, "%s %s\n",
      astTypeToStr(root->kind),
      files[file->fileIndex].absolute_path
      );

    for (int i = 0; i < file->declarations.len; ++i) {
      ASTNode *child = file->declarations[i];
      debugPrintAST(child, out, shiftWidth, currentShift + shiftWidth);
    }
  } break;
  case ASTProc_: {
    ASTProc *proc = (ASTProc *)root;
    fprintf(out, "%s %.*s (%d:%d)",
      astTypeToStr(root->kind),
      (int)proc->name.len, proc->name.data,
      proc->site.line, proc->site.col
      );

    if (proc->flags & FlagsProcForeign) {
      fprintf(out, " foreign function");
    }

    fprintf(out, "\n");

    for (int i = 0; i < proc->arguments.len; ++i) {
      auto arg = proc->arguments[i];
      printSpace(out, currentShift + shiftWidth);
      fprintf(out, "arg [%d] '%.*s' (%d:%d) ", i, (int)arg->name.len, arg->name.data,
        arg->site.line, arg->site.col);
      debugPrintAST(arg->type, out, shiftWidth, currentShift + 1*shiftWidth, 0);
    }
    if (proc->flags & FlagsProcVariadic) {
      printSpace(out, currentShift + shiftWidth);
      fprintf(out, "arg [...] C variadic\n");
    }
    for (int i = 0; i < proc->returnTypes.len; ++i) {
      auto ret = proc->returnTypes[i];
      printSpace(out, currentShift + shiftWidth);
      fprintf(out, "ret [%d] ", i);
      debugPrintAST(ret, out, shiftWidth, currentShift + 1*shiftWidth, 0);
    }
    if (proc->body) {
      printSpace(out, currentShift + shiftWidth);
      fprintf(out, "body ");
      debugPrintAST(proc->body, out, shiftWidth, currentShift + 1*shiftWidth, 0);
    }
  } break;
  case ASTIdentifier_: {
    ASTIdentifier *ident = (ASTIdentifier *)root;
    fprintf(out, "%s %.*s (%d:%d)\n",
      astTypeToStr(root->kind),
      (int)ident->name.len, ident->name.data, ident->site.line, ident->site.col);
  } break;
  case ASTPointerTo_: {
    ASTPointerTo *ptr = (ASTPointerTo *)root;
    fprintf(out, "%s (%d:%d)\n",
      astTypeToStr(root->kind), root->site.line, root->site.col);
    debugPrintAST(ptr->pointsTo, out, shiftWidth, currentShift + 1 * shiftWidth);
  } break;
  case ASTBlock_: {
    ASTBlock *block = (ASTBlock *)root;
    fprintf(out, "%s (%d:%d)\n",
      astTypeToStr(root->kind), root->site.line, root->site.col);
    for (int i = 0; i < block->statements.len; ++i) {
      ASTNode *stmt = block->statements.data[i];
      printSpace(out, currentShift + shiftWidth);
      fprintf(out, "statement [%d] ", i);
      debugPrintAST(stmt, out, shiftWidth, currentShift + 1*shiftWidth, 0);
    }
  } break;
  case ASTVariableDefinition_: {
    ASTVariableDefinition *varDefn = (ASTVariableDefinition *)root;
    fprintf(out, "%s (%d:%d)\n",
      astTypeToStr(root->kind), root->site.line, root->site.col);
    for (int i = 0; i < varDefn->vars.len; ++i) {
      ASTVariable *var = varDefn->vars[i];
      printSpace(out, currentShift + shiftWidth);
      fprintf(out, "var [%d] ", i);
      debugPrintAST(var, out, shiftWidth, currentShift + 1*shiftWidth, 0);
    }
  } break;
  case ASTVariable_: {
    ASTVariable *var = (ASTVariable *)root;
    fprintf(out, "%s '%.*s' (%d:%d)\n",
      astTypeToStr(root->kind), (int)var->name.len, var->name.data, root->site.line, root->site.col);

    printSpace(out, currentShift + shiftWidth);
    if (var->type) {
      fprintf(out, "type ");
      debugPrintAST(var->type, out, shiftWidth, currentShift + 2*shiftWidth, 0);
    } else {
      fprintf(out, "type NULL\n");
    }

    printSpace(out, currentShift + shiftWidth);
    if (var->initExpr) {
      fprintf(out, "initExpr ");
      debugPrintAST(var->initExpr, out, shiftWidth, currentShift + 2*shiftWidth, 0);
    } else {
      fprintf(out, "initExpr NULL\n");
    }

  } break;
  case ASTLiteral_: {
    ASTLiteral *literal = (ASTLiteral *)root;
    fprintf(out, "%s", astTypeToStr(root->kind));

    if (literal->literalKind & LiteralKindNumber) fprintf(out, " number");
    if (literal->literalKind & LiteralKindString) fprintf(out, " string");

    fprintf(out, " '%.*s' (%d:%d)\n",
      (int)literal->raw.len, literal->raw.data, root->site.line, root->site.col);

  } break;
  default: {
    fprintf(out, "'%s' in debugPrintAST not implemented\n", astTypeToStr(root->kind));
    NOT_IMPLEMENTED;
  }
  }
}
