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


typedef uint32_t IRBasicBlockIndex;
struct IRFunction;

struct IRInstruction {

  union {
    struct {
      uint32_t ret;
      uint32_t op1;
      uint32_t op2;
    } binaryOp;

    uint32_t retValue;

    struct {
      IRBasicBlockIndex block;
    } jump;

    struct {
      uint32_t argValue;
    } arg;

    struct {
      IRFunction *fn;
      uint32_t ret;
      uint32_t argsNumber;
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
  Site site;
  uint16_t type;
};

#define INSTRUCTIONS \
  XX(INSTRUCTION_TYPE_INVALID) \
  XX(INSTRUCTION_ALLOC_STACK_NODE) \
  XX(INSTRUCTION_JUMP) \
  XX(INSTRUCTION_RET_VOID) \
  XX(INSTRUCTION_RET) \
  XX(INSTRUCTION_IADD) \
  XX(INSTRUCTION_PUSH_ARG) \
  XX(INSTRUCTION_CALL) \
  XX(INSTRUCTION_CONSTANT) \
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

struct IRBasicBlock_ {
  uint32_t offset;
  uint32_t length;
  const char *name;
};


struct IRValueInfo {
  Type *type;
};

struct IRLocalVariableAllocation {
  Type *type;
  Site site;
  uint32_t addressValue;
};

enum {
  FUNCTION_FLAG_EXTERNAL = 1 << 0,
  FUNCTION_FLAG_VARIADIC = 1 << 1,
};

struct IRFunction {
  Str name;
  Type *retType;

  IRBasicBlockIndex init;

  Array<IRValueInfo> values;

  Array<IRBasicBlock_> basicBlocks;
  Array<IRInstruction> instructions;
  Array<IRLocalVariableAllocation> localVariables;

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

IRBasicBlockIndex createIRBlock(IRFunction *fn, const char *name = "");

void irInstJump(IRFunction *fn, IRBasicBlockIndex bb, IRBasicBlockIndex to, Site site = {});
void irInstRetVoid(IRFunction *fn, IRBasicBlockIndex bb, Site site = {});
void irInstRet(IRFunction *fn, IRBasicBlockIndex bb, uint32_t op1, Site site = {});
uint32_t irInstIAdd(IRFunction *fn, IRBasicBlockIndex bb, uint32_t op1, uint32_t op2, Site site = {});
uint32_t irInstCall(IRFunction *fn, IRBasicBlockIndex bb, IRFunction *callee, Array<uint32_t> args, Site site = {});
uint32_t irInstConstant(IRFunction *fn, IRBasicBlockIndex bb, Type *type, IRValue v, Site site = {});
uint32_t irInstAlloca(IRFunction *fn, IRBasicBlockIndex bb, Type *type, Site site = {});
uint32_t irInstLoad(IRFunction *fn, IRBasicBlockIndex bb, Type *type, uint32_t ptrValue, Site site = {});
void irInstStore(IRFunction *fn, IRBasicBlockIndex bb, uint32_t ptrValue, uint32_t value, Site site = {});

void printType(Type *type, FILE *f);
const char *instructionTypeToString(int type);
