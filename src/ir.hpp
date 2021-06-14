#pragma once

#include <inttypes.h>
#include "utils.hpp"
#include "compiler.hpp"

union IRValue {
  int8_t   i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;

  uint8_t   u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;

  float f32;
  double f64;

  void *ptr;
};


struct IRBasicBlock;
struct IRFunction;

struct IRInstruction {
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
      IRBasicBlock *block;
    } jump;

    struct {
      IRFunction *fn;
      uint32_t ret;
      Array<uint32_t> args;
    } call;

    struct {
      IRValue constant;
      uint32_t valueIndex;
    } constantInit;

    struct {
      Type *type;
      uint32_t valueIndex;
    } alloca;
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
  XX(INSTRUCTION_CONSTANT) \
  XX(INSTRUCTION_ALLOCA) \
  XX(INSTRUCTION_LAST_INSTRUCTION) \


#define XX(INSTRUCTION) INSTRUCTION,
enum IRInstructionType {
  INSTRUCTIONS
};
#undef XX

struct IRBasicBlock {
  const char *name;
  IRFunction *fn;

  BucketedArray<IRInstruction, 8> instructions;

  IRInstruction terminator;
};

struct IRValueInfo {
  Type *type;
};

enum {
  FUNCTION_FLAG_EXTERNAL = 1 << 0,
  FUNCTION_FLAG_VARIADIC = 1 << 1,
};

struct IRFunction {
  Str name;
  Type *retType;

  IRBasicBlock *init;
  IRBasicBlock *exit;

  BucketedArray<IRBasicBlock, 8> basicBlocks;
  Array<IRValueInfo> values;

  int blockNameCounter;
  int valuesNumber;
  int argsNumber;

  uint16_t flags;
  Str symbolName, library;
  void *ptr;
};

struct IRHub {
  Array<IRFunction *> functions;
};
void printAll(IRHub *h, FILE *f);


IRFunction *createIRFunction(IRHub *h, Str name, Array<Type *> args = {}, Type *retType = NULL);

IRFunction *createIRForeignFunction(IRHub *h, Str name, Str library, Array<Type *> args = {}, Type *retType = NULL, bool variadic = false);

IRBasicBlock *createIRBlock(IRFunction *fn, const char *name = "");

void irInstJump(IRBasicBlock *bb, IRBasicBlock *to);
void irInstRetVoid(IRBasicBlock *bb);
void irInstRet(IRBasicBlock *bb, uint32_t op1);
uint32_t irInstIAdd(IRBasicBlock *bb, uint32_t op1, uint32_t op2);
uint32_t irInstCall(IRBasicBlock *bb, IRFunction *callee, Array<uint32_t> args);
uint32_t irInstConstant(IRBasicBlock *bb, Type *type, IRValue v);
uint32_t irInstAlloca(IRBasicBlock *bb, Type *type);

void printType(Type *type, FILE *f);
const char *instructionTypeToString(int type);
