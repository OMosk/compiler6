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

IRValue valueI8(int8_t v);
IRValue valueI16(int16_t v);
IRValue valueI32(int32_t v);
IRValue valueI64(int64_t v);

IRValue  valueU8(uint8_t v);
IRValue valueU16(uint16_t v);
IRValue valueU32(uint32_t v);
IRValue valueU64(uint64_t v);

IRValue valueF32(float v);
IRValue valueF64(double v);

IRValue valuePtr(void *v);


struct IRBasicBlock;
struct IRFunction;

struct IRInstruction {

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

    struct {
      uint32_t ret;
      uint32_t from;
    } load;

    struct {
      uint32_t to;
      uint32_t value;
    } store;

  };
  uint16_t type;

  Site site;
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
  XX(INSTRUCTION_STORE) \
  XX(INSTRUCTION_LOAD) \
  XX(INSTRUCTION_COPY_BYTES) \
  XX(INSTRUCTION_LAST_INSTRUCTION) \


#define XX(INSTRUCTION) INSTRUCTION,
enum IRInstructionType {
  INSTRUCTIONS
};
#undef XX

typedef BucketedArray<IRInstruction, 8> InstructionsBucketedArray;

struct IRBasicBlock {
  const char *name;
  IRFunction *fn;

  //InstructionsBucketedArray instructions;
  Array<IRInstruction> instructions;

  IRInstruction terminator;
};

struct IRBasicBlock_ {
  uint16_t offset;
  uint16_t length;
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

  Array<IRBasicBlock *> basicBlocks;
  Array<IRBasicBlock_> basicBlocks_;
  //BucketedArray<IRBasicBlock, 8> basicBlocks;
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

void irInstJump(IRBasicBlock *bb, IRBasicBlock *to, Site site = {});
void irInstRetVoid(IRBasicBlock *bb, Site site = {});
void irInstRet(IRBasicBlock *bb, uint32_t op1, Site site = {});
uint32_t irInstIAdd(IRBasicBlock *bb, uint32_t op1, uint32_t op2, Site site = {});
uint32_t irInstCall(IRBasicBlock *bb, IRFunction *callee, Array<uint32_t> args, Site site = {});
uint32_t irInstConstant(IRBasicBlock *bb, Type *type, IRValue v, Site site = {});
uint32_t irInstAlloca(IRBasicBlock *bb, Type *type, Site site = {});
uint32_t irInstLoad(IRBasicBlock *bb, Type *type, uint32_t ptrValue, Site site = {});
void irInstStore(IRBasicBlock *bb, uint32_t ptrValue, uint32_t value, Site site = {});

void printType(Type *type, FILE *f);
const char *instructionTypeToString(int type);
