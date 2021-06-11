#pragma once

#include <inttypes.h>
#include "utils.hpp"
#include "compiler.hpp"

namespace ir {

struct BasicBlock;
struct Function;

struct Instruction {
  uint16_t type;

  union {
    struct {
      uint32_t ret;
      uint32_t op1;
      uint32_t op2;
    } binaryOp;

    //struct {
    //  uint32_t argNo;
    //  uint32_t op;
    //} setArg;

    uint32_t retValue;

    struct {
      BasicBlock *block;
    } jump;

    struct {
      Function *fn;
      uint32_t ret;
      Array<uint32_t> args;
    } call;
  };

  Site *site;
};

#define INSTRUCTIONS \
  XX(INSTRUCTION_TYPE_INVALID) \
  XX(INSTRUCTION_JUMP) \
  XX(INSTRUCTION_RET_VOID) \
  XX(INSTRUCTION_RET) \
  XX(INSTRUCTION_IADD) \
  XX(INSTRUCTION_SET_ARG) \
  XX(INSTRUCTION_CALL) \
  XX(INSTRUCTION_LAST_INSTRUCTION) \


#define XX(INSTRUCTION) INSTRUCTION,
enum InstructionType {
  INSTRUCTIONS
};
#undef XX

struct BasicBlock {
  const char *name;

  BucketedArray<Instruction, 8> instructions;

  Instruction terminator;
};

struct Value {
  Type *type;
};

enum {
  FUNCTION_FLAG_EXTERNAL = 1 << 0,
  FUNCTION_FLAG_VARIADIC = 1 << 1,
};

struct Function {
  Str name;
  Type *retType;

  BasicBlock *init;
  BasicBlock *exit;

  BucketedArray<BasicBlock, 8> basicBlocks;
  Array<Value> values;

  int blockNameCounter;
  int valuesNumber;
  int argsNumber;

  uint16_t flags;
  Str symbolName, library;
  void *ptr;
};

struct Hub {
  Array<Function *> functions;
};
void printAll(Hub *h, FILE *f);

struct Builder {
  Hub *h;

  Function *function(Str name, Array<Type *> args = {}, Type *retType = NULL) {
    Function *fn = (Function *)alloc(sizeof(Function), alignof(Function), 1);
    *fn = {};
    fn->init = alloc_back(&fn->basicBlocks);
    fn->exit = alloc_back(&fn->basicBlocks);

    fn->init->name = "init";
    fn->exit->name = "exit";

    fn->name = name;

    fn->argsNumber = args.len;
    #if 0
    for (int i = 0; i < args.len; ++i) {
      Value *value = alloc_back(&fn->values);
      value->type = args[i];
      value->idx = i;
      fn->valuesNumber++;
    }
    #else
    resize(&fn->values, args.len);
    for (int i = 0; i < args.len; ++i) {
      Value value = {};
      value.type = args[i];
      fn->values[i] = value;
      fn->valuesNumber++;
    }
    #endif
    fn->retType = retType;

    //TODO: Concurrency:
    append(&h->functions, fn);

    return fn;
  };

  Function *foreignFunction(Str name, Str library, Array<Type *> args = {}, Type *retType = NULL, bool variadic = false) {
    Function *fn = function(name, args, retType);
    fn->flags = FUNCTION_FLAG_EXTERNAL;
    fn->library = library;
    fn->symbolName = name;
    if (variadic) fn->flags |= FUNCTION_FLAG_VARIADIC;
    return fn;
  }

  BasicBlock *block(Function *fn, const char *name = "") {
    BasicBlock *result = alloc_back(&fn->basicBlocks);
    *result = {};

    //TODO: ifdef this out in optimized build
    result->name = SPrintf("%s_%d", name, fn->blockNameCounter++).data;

    return result;
  };

  Instruction *instruction(BasicBlock *bb, uint16_t type) {
    if (bb->terminator.type) {
      ASSERT(false, "Attempting to insert instruction into block that already has terminator");
    }

    Instruction *inst = alloc_back(&bb->instructions);

    inst->type = type;
    return inst;
  }

  Instruction *terminator(BasicBlock *bb, uint16_t type) {
    if (bb->terminator.type) {
      ASSERT(false, "Attempting to insert instruction into block that already has terminator");
    }
    Instruction *inst = &bb->terminator;
    inst->type = type;
    return inst;
  }

  uint32_t value(Function *fn) {
    append(&fn->values, {});
    return fn->valuesNumber++;
  }

  void jump(BasicBlock *bb, BasicBlock *to);
  void retVoid(BasicBlock *bb);
  void ret(BasicBlock *bb, uint32_t op1);
  uint32_t iadd(BasicBlock *bb, Function *fn, uint32_t op1, uint32_t op2);
  //void setArg(BasicBlock *bb, uint32_t argNo, uint32_t op);
  uint32_t call(BasicBlock *bb, Function *caller, Function *callee, Array<uint32_t> args);
};

void printType(Type *type, FILE *f);
const char *instructionTypeToString(int type);

}
