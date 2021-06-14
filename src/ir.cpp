#include "ir.hpp"
#include "compiler.hpp"

IRFunction *createIRFunction(IRHub *h, Str name, Array<Type *> args, Type *retType) {
  IRFunction *fn = (IRFunction *)alloc(sizeof(IRFunction), alignof(IRFunction), 1);
  *fn = {};
  fn->init = createIRBlock(fn, "init");
  fn->exit = createIRBlock(fn, "exit");

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
    IRValueInfo value = {};
    value.type = args[i];
    fn->values[i] = value;
    fn->valuesNumber++;
  }
  #endif
  fn->retType = retType;

  //TODO: Concurrency:
  append(&h->functions, fn);

  return fn;
}

IRFunction *createIRForeignFunction(IRHub *h, Str name, Str library, Array<Type *> args, Type *retType, bool variadic) {
  IRFunction *fn = createIRFunction(h, name, args, retType);
  fn->flags = FUNCTION_FLAG_EXTERNAL;
  fn->library = library;
  fn->symbolName = name;
  if (variadic) fn->flags |= FUNCTION_FLAG_VARIADIC;
  return fn;
}

IRBasicBlock *createIRBlock(IRFunction *fn, const char *name) {
  IRBasicBlock *result = alloc_back(&fn->basicBlocks);
  *result = {};
  result->fn = fn;

  //TODO: ifdef this out in optimized build
  result->name = SPrintf("%s_%d", name, fn->blockNameCounter++).data;

  return result;
}

IRInstruction *createIRInstruction(IRBasicBlock *bb, uint16_t type) {
  if (bb->terminator.type) {
    ASSERT(false, "Attempting to insert instruction into block that already has terminator");
  }

  IRInstruction *inst = alloc_back(&bb->instructions);

  inst->type = type;
  return inst;
}

IRInstruction *createIRTerminatorInstruction(IRBasicBlock *bb, uint16_t type) {
  if (bb->terminator.type) {
    ASSERT(false, "Attempting to insert instruction into block that already has terminator");
  }
  IRInstruction *inst = &bb->terminator;
  inst->type = type;
  return inst;
}

uint32_t createIRValue(IRFunction *fn) {
  append(&fn->values, {});
  return fn->valuesNumber++;
}

void irInstJump(IRBasicBlock *bb, IRBasicBlock *to) {
  IRInstruction *jump = createIRTerminatorInstruction(bb, INSTRUCTION_JUMP);
  jump->jump.block = to;
}

void irInstRetVoid(IRBasicBlock *bb) {
  IRInstruction *ret = createIRTerminatorInstruction(bb, INSTRUCTION_RET_VOID);
}


void irInstRet(IRBasicBlock *bb, uint32_t op1) {
  IRInstruction *ret = createIRTerminatorInstruction(bb, INSTRUCTION_RET);
  ret->retValue = op1;
}

uint32_t irInstIAdd(IRBasicBlock *bb, uint32_t op1, uint32_t op2) {
  IRInstruction *add = createIRInstruction(bb, INSTRUCTION_IADD);

  ASSERT(bb->fn->values[op1].type == bb->fn->values[op2].type, "Mismatched types");
  add->binaryOp.op1 = op1;
  add->binaryOp.op2 = op2;
  add->binaryOp.ret = createIRValue(bb->fn);
  bb->fn->values[add->binaryOp.ret].type = bb->fn->values[op1].type;
  return add->binaryOp.ret;
}

//void Builder::setArg(BasicBlock *bb, uint32_t argNo, uint32_t op) {
//  Instruction *i = this->instruction(bb, INSTRUCTION_SET_ARG);
//  i->setArg.argNo = argNo;
//  i->setArg.op = op;
//}

uint32_t irInstCall(IRBasicBlock *bb, IRFunction *callee, Array<uint32_t> args) {
  IRInstruction *call = createIRInstruction(bb, INSTRUCTION_CALL);
  call->call.fn = callee;
  call->call.ret = createIRValue(bb->fn);
  call->call.args = args;
  bb->fn->values[call->call.ret].type = callee->retType;
  return call->call.ret;
}

uint32_t irInstConstant(IRBasicBlock *bb, Type *type, IRValue v) {
  uint32_t result = createIRValue(bb->fn);
  bb->fn->values[result].type = type;
  IRInstruction *constant = createIRInstruction(bb, INSTRUCTION_CONSTANT);
  constant->constantInit.constant = v;
  constant->constantInit.valueIndex = result;

  return result;
}


uint32_t irInstAlloca(IRBasicBlock *bb, Type *type) {
  uint32_t result = createIRValue(bb->fn);
  bb->fn->values[result].type = getPointerToType(type);
  IRInstruction *alloca = createIRInstruction(bb, INSTRUCTION_ALLOCA);
  alloca->alloca.type = type;
  alloca->alloca.valueIndex = result;

  return result;
}

const char *instructionTypeToString(int type) {
  #define XX(INSTRUCTION) case INSTRUCTION: return #INSTRUCTION;
  switch (type) {
  INSTRUCTIONS
  default: ASSERT(false, "unexpected instruction type value");
  }
  #undef XX
}


void printInstruction(IRFunction *fn, IRInstruction *instruction, FILE *f) {
  fprintf(f, "\t%8p: %3d", instruction, instruction->type);
  switch (instruction->type) {
    case INSTRUCTION_JUMP: {
      fprintf(f, " %s to %s(%p)\n",
        instructionTypeToString(instruction->type), instruction->jump.block->name, instruction->jump.block);
    } break;
    case INSTRUCTION_RET_VOID: {
      fprintf(f, " %s\n", instructionTypeToString(instruction->type));
    } break;
    case INSTRUCTION_RET: {
      fprintf(f, " %s v%u\n", instructionTypeToString(instruction->type), instruction->retValue);
    } break;
    case INSTRUCTION_IADD: {
      fprintf(f, " ");
      printType(fn->values[instruction->binaryOp.ret].type, f);
      fprintf(f, " v%u = iadd v%u, v%u\n", instruction->binaryOp.ret, instruction->binaryOp.op1, instruction->binaryOp.op2);
    } break;

    //case INSTRUCTION_SET_ARG: {
    //  fprintf(f, " set_arg arg[%u] = v%u\n", instruction->setArg.argNo, instruction->setArg.op);
    //} break;

    case INSTRUCTION_CALL: {
      fprintf(f, " v%u = call %p %.*s (", instruction->call.ret, instruction->call.fn, (int)instruction->call.fn->name.len,
        instruction->call.fn->name.data);
      for (int i = 0; i < instruction->call.args.len; ++i) {
        if (i != 0) fprintf(f, ", ");
        fprintf(f, "v%u", instruction->call.args[i]);
      }
      fprintf(f, ")\n");
    } break;

    case INSTRUCTION_CONSTANT: {
      Type *type = fn->values[instruction->constantInit.valueIndex].type;
      IRValue value = instruction->constantInit.constant;
      fprintf(f, " ");
      printType(type, f);
      fprintf(f, " v%u = constant %p", instruction->constantInit.valueIndex, instruction->constantInit.constant.ptr);
      #define S(TYPE, FORMAT_SPECIFIER, FIELD) do { if (type == FIELD) { fprintf(f, "(%s = " FORMAT_SPECIFIER ")", #FIELD, value.FIELD); } } while(0)
      S(i8, "%d", i8);
      S(i16, "%d", i16);
      S(i32, "%d", i32);
      S(i64, "%ld", i64);
      S(u8, "%u", u8);
      S(u16, "%u", u16);
      S(u32, "%u", u32);
      S(u64, "%lu", u64);
      S(f32, "%f", f32);
      S(f64, "%f", f64);
      fprintf(f, "\n");
    } break;

    case INSTRUCTION_ALLOCA: {
      Type *ptrType = fn->values[instruction->alloca.valueIndex].type;
      Type *type = instruction->alloca.type;
      fprintf(f, " ");
      printType(ptrType, f);
      fprintf(f, " v%u = alloca ", instruction->alloca.valueIndex);
      printType(type, f);
      fprintf(f, " (size = %hu, alignment = %hu)\n", type->size, type->alignment);
    } break;

    default: {
      printf(" unhandled case in printInstruction: %d %s\n", instruction->type, instructionTypeToString(instruction->type));
      ASSERT(false, "unhandled case");
    }
  }
}

void printType(Type *type, FILE *f) {
  if (type == NULL) {
    fprintf(f, "(nothing)");
    return;
  }
  #define XX(TYPE) do { if (type == TYPE) { fprintf(f, "%s", #TYPE); return; } } while (0)

  XX(i8);
  XX(i16);
  XX(i32);
  XX(i64);

  XX(u8);
  XX(u16);
  XX(u32);
  XX(u64);

  XX(f32);
  XX(f64);

  #undef XX

  if (type->flags & TYPE_FLAG_POINTER) {
    fprintf(f, "*");
    printType(type->pointerToType, f);
    return;
  }

  NOT_IMPLEMENTED;
}

void printFunction(IRFunction *fn, FILE *f) {
  if (fn->flags & FUNCTION_FLAG_EXTERNAL) {
    fprintf(f, "external ");
  }

  fprintf(f, "function %p %.*s:\n", fn, (int)fn->name.len, fn->name.data);
  fprintf(f, "\treturns ");
  printType(fn->retType, f);
  fprintf(f, "\n");

  #if 0
  {
    auto it = iterator(&fn->values);
    Value *value = it.next();
    for (int i = 0; i < fn->argsNumber; ++i, value = it.next()) {
      fprintf(f, "\t");
      printType(value->type, f);
      fprintf(f, " v%d = arg[%d] ", i, i);
      fprintf(f, "\n");
    }
  }
  #else
  for (int i = 0; i < fn->argsNumber; ++i) {
    fprintf(f, "\t");
    printType(fn->values[i].type, f);
    fprintf(f, " v%d = arg[%d] ", i, i);
    fprintf(f, "\n");
  }
  #endif
  if (fn->flags & FUNCTION_FLAG_VARIADIC) {
    fprintf(f, "\t...(variadic)\n");
  }

  if (fn->flags & FUNCTION_FLAG_EXTERNAL) {
    return;
  }

  #if 0
  for (auto *blockBucketIt = &fn->basicBlocks.first; blockBucketIt != NULL; blockBucketIt = blockBucketIt->next) {
    int bbBucketSize = bucket_size(&fn->basicBlocks);
    if (blockBucketIt->next == NULL) {
      //last bucket, may be not full
      bbBucketSize = fn->basicBlocks.lastBucketSize;
    }
    for (int basicBlockIndex = 0; basicBlockIndex < bbBucketSize; ++basicBlockIndex) {
      BasicBlock *block = blockBucketIt->items + basicBlockIndex;
      fprintf(f, "%s(%p):\n", block->name, block);

      for (auto *instructionBucketIt = &block->instructions.first;
        instructionBucketIt != NULL; instructionBucketIt = instructionBucketIt->next) {
        int instBucketSize = bucket_size(&block->instructions);
        if (instructionBucketIt->next == NULL) {
          instBucketSize = block->instructions.lastBucketSize;
        }
        for (int instructionIndex = 0; instructionIndex < instBucketSize; ++instructionIndex) {
          Instruction *instruction = instructionBucketIt->items + instructionIndex;
          printInstruction(instruction, f);
        }
      }

      printInstruction(&block->terminator, f);
    }
  }
  #else
  auto bucketIt = iterator(&fn->basicBlocks);
  for (IRBasicBlock *block = bucketIt.next(); block != NULL; block = bucketIt.next()) {
    fprintf(f, "\n");
    fprintf(f, "%s(%p):\n", block->name, block);

    auto instructionIt = iterator(&block->instructions);
    for (IRInstruction *instruction = instructionIt.next(); instruction != NULL; instruction = instructionIt.next()) {
      printInstruction(fn, instruction, f);
    }

    printInstruction(fn, &block->terminator, f);
  }
  #endif
  fprintf(f, "end of %.*s:\n\n", (int)fn->name.len, fn->name.data);
}

void printAll(IRHub *h, FILE *f) {
  for (int functionIndex = 0; functionIndex < h->functions.len; ++functionIndex) {
    printFunction(h->functions[functionIndex], f);
  }
}
