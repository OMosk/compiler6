#include "ir.hpp"

namespace ir {

void Builder::jump(BasicBlock *bb, BasicBlock *to) {
  Instruction *jump = this->terminator(bb, INSTRUCTION_JUMP);
  jump->jump.block = to;
}

void Builder::retVoid(BasicBlock *bb) {
  Instruction *ret = this->terminator(bb, INSTRUCTION_RET_VOID);
}


void Builder::ret(BasicBlock *bb, uint32_t op1) {
  Instruction *ret = this->terminator(bb, INSTRUCTION_RET);
  ret->retValue = op1;
}

uint32_t Builder::iadd(BasicBlock *bb, Function *fn, uint32_t op1, uint32_t op2) {
  Instruction *add = this->instruction(bb, INSTRUCTION_IADD);

  ASSERT(fn->values[op1].type == fn->values[op2].type, "Mismatched types");
  add->binaryOp.op1 = op1;
  add->binaryOp.op2 = op2;
  add->binaryOp.ret = this->value(fn);
  fn->values[add->binaryOp.ret].type = fn->values[op1].type;
  return add->binaryOp.ret;
}

//void Builder::setArg(BasicBlock *bb, uint32_t argNo, uint32_t op) {
//  Instruction *i = this->instruction(bb, INSTRUCTION_SET_ARG);
//  i->setArg.argNo = argNo;
//  i->setArg.op = op;
//}

uint32_t Builder::call(BasicBlock *bb, Function *caller, Function *callee, Array<uint32_t> args) {
  Instruction *call = this->instruction(bb, INSTRUCTION_CALL);
  call->call.fn = callee;
  call->call.ret = this->value(caller);
  call->call.args = args;
  caller->values[call->call.ret].type = callee->retType;
  return call->call.ret;
}

const char *instructionTypeToString(int type) {
  #define XX(INSTRUCTION) case INSTRUCTION: return #INSTRUCTION;
  switch (type) {
  INSTRUCTIONS
  default: ASSERT(false, "unexpected instruction type value");
  }
  #undef XX
}


void printInstruction(Function *fn, Instruction *instruction, FILE *f) {
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

  NOT_IMPLEMENTED;
}

void printFunction(Function *fn, FILE *f) {
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
  for (BasicBlock *block = bucketIt.next(); block != NULL; block = bucketIt.next()) {
    fprintf(f, "\n");
    fprintf(f, "%s(%p):\n", block->name, block);

    auto instructionIt = iterator(&block->instructions);
    for (Instruction *instruction = instructionIt.next(); instruction != NULL; instruction = instructionIt.next()) {
      printInstruction(fn, instruction, f);
    }

    printInstruction(fn, &block->terminator, f);
  }
  #endif
  fprintf(f, "end of %.*s:\n\n", (int)fn->name.len, fn->name.data);
}

void printAll(Hub *h, FILE *f) {
  for (int functionIndex = 0; functionIndex < h->functions.len; ++functionIndex) {
    printFunction(h->functions[functionIndex], f);
  }
}

}
