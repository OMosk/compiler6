#include "ir.hpp"
#include "compiler.hpp"

IRValue valueI8(int8_t v) {
  IRValue r = {};
  r.i8 = v;
  return r;
}
IRValue valueI16(int16_t v) {
  IRValue r = {};
  r.i16 = v;
  return r;
}
IRValue valueI32(int32_t v) {
  IRValue r = {};
  r.i32 = v;
  return r;
}
IRValue valueI64(int64_t v) {
  IRValue r = {};
  r.i64 = v;
  return r;
}
IRValue  valueU8(uint8_t v) {
  IRValue r = {};
  r.u8 = v;
  return r;
}
IRValue valueU16(uint16_t v) {
  IRValue r = {};
  r.u16 = v;
  return r;
}
IRValue valueU32(uint32_t v) {
  IRValue r = {};
  r.u32 = v;
  return r;
}
IRValue valueU64(uint64_t v){
  IRValue r = {};
  r.u64 = v;
  return r;
}
IRValue valueF32(float v){
  IRValue r = {};
  r.f32 = v;
  return r;
}
IRValue valueF64(double v) {
  IRValue r = {};
  r.f64 = v;
  return r;
}
IRValue valuePtr(void *v) {
  IRValue r = {};
  r.ptr = v;
  return r;
}

IRInstruction *createIRInstruction(IRFunction *fn, IRBasicBlockIndex bIndex, uint16_t type, Site site);

IRFunction *createIRFunction(IRHub *h, Str name, Array<Type *> args, Type *retType) {
  IRFunction *fn = (IRFunction *)alloc(sizeof(IRFunction), alignof(IRFunction), 1);
  *fn = {};
  fn->init = createIRBlock(fn, "init");
  createIRInstruction(fn, fn->init, INSTRUCTION_ALLOC_STACK_NODE, {});

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
    //fn->valuesNumber++;
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

IRBasicBlockIndex createIRBlock(IRFunction *fn, const char *name) {
  IRBasicBlockIndex result = fn->basicBlocks.len;
  append(&fn->basicBlocks, {});
  IRBasicBlock_ *block = &fn->basicBlocks[result];
  block->name = name;
  block->offset = fn->instructions.len;

  //reserve for terminator instruction
  append(&fn->instructions, {});
  block->length++;

  return result;
}

bool isTerminatorInstruction(uint16_t type) {
  switch (type) {
    case INSTRUCTION_RET:
    case INSTRUCTION_RET_VOID:
    case INSTRUCTION_JUMP: {
      return true;
    } break;
  }

  return false;
}

IRInstruction *createIRInstruction(IRFunction *fn, IRBasicBlockIndex bIndex, uint16_t type, Site site) {
  IRBasicBlock_ *bb = &fn->basicBlocks[bIndex];

  if (bb->length) {
    IRInstruction *inst = &fn->instructions[bb->offset + bb->length - 1];
    if (isTerminatorInstruction(inst->type)) {
        ASSERT(false,
          "Attempting to insert instruction into block that already has terminator");
    }
  }

  int shift = 0;
  if (!isTerminatorInstruction(type)) {
    if (bb->offset + bb->length != fn->instructions.len) {
      ASSERT(false, "You can't insert instructions to previous blocks unless it is terminator");
    }

    append(&fn->instructions, {});
    bb->length++;
    shift = 1;
  }


  IRInstruction *inst = &fn->instructions[bb->offset + bb->length - 1 - shift];

  inst->type = type;
  inst->site = site;
  return inst;
}

uint32_t createIRValue(IRFunction *fn) {
  uint32_t valueNumber = fn->values.len;
  append(&fn->values, {});
  //uint32_t valueNumber = fn->valuesNumber++;
  //ASSERT(fn->valuesNumber == fn->values.len, "valuesNumber is in sync with fn->values.len");
  return valueNumber;
}

void irInstJump(IRFunction *fn, IRBasicBlockIndex bIndex, IRBasicBlockIndex to, Site site) {
  IRInstruction *jump = createIRInstruction(fn, bIndex, INSTRUCTION_JUMP, site);
  jump->jump.block = to;
}

void irInstRetVoid(IRFunction *fn, IRBasicBlockIndex bIndex, Site site) {
  IRInstruction *ret = createIRInstruction(fn, bIndex, INSTRUCTION_RET_VOID, site);
}


void irInstRet(IRFunction *fn, IRBasicBlockIndex bIndex, uint32_t op1, Site site) {
  IRInstruction *ret = createIRInstruction(fn, bIndex, INSTRUCTION_RET, site);
  ret->retValue = op1;
}

uint32_t irInstIAdd(IRFunction *fn, IRBasicBlockIndex bIndex, uint32_t op1, uint32_t op2, Site site) {
  IRInstruction *add = createIRInstruction(fn, bIndex, INSTRUCTION_IADD, site);

  ASSERT(fn->values[op1].type == fn->values[op2].type, "Mismatched types");
  add->binaryOp.op1 = op1;
  add->binaryOp.op2 = op2;
  add->binaryOp.ret = createIRValue(fn);
  fn->values[add->binaryOp.ret].type = fn->values[op1].type;
  return add->binaryOp.ret;
}

//void Builder::setArg(BasicBlock *bb, uint32_t argNo, uint32_t op) {
//  Instruction *i = this->instruction(bb, INSTRUCTION_SET_ARG);
//  i->setArg.argNo = argNo;
//  i->setArg.op = op;
//}

uint32_t irInstCall(IRFunction *fn, IRBasicBlockIndex bIndex, IRFunction *callee, Array<uint32_t> args, Site site) {
  for (int i = 0; i < args.len; ++i) {
    IRInstruction *push = createIRInstruction(fn, bIndex, INSTRUCTION_PUSH_ARG, site);
    push->arg.argValue = args[i];
  }
  IRInstruction *call = createIRInstruction(fn, bIndex, INSTRUCTION_CALL, site);
  call->call.fn = callee;
  call->call.ret = createIRValue(fn);
  call->call.argsNumber = args.len;
  //call->call.args = args;
  fn->values[call->call.ret].type = callee->retType;
  return call->call.ret;
}

uint32_t irInstConstant(IRFunction *fn, IRBasicBlockIndex bIndex, Type *type, IRValue v, Site site) {
  uint32_t result = createIRValue(fn);
  fn->values[result].type = type;
  IRInstruction *constant = createIRInstruction(fn, bIndex, INSTRUCTION_CONSTANT, site);
  constant->constantInit.constant = v;
  constant->constantInit.valueIndex = result;

  return result;
}


uint32_t irInstAlloca(IRFunction *fn, IRBasicBlockIndex bIndex, Type *type, Site site) {
  uint32_t result = createIRValue(fn);
  fn->values[result].type = getPointerType(type);
  IRLocalVariableAllocation local = {};
  local.type = type;
  local.site = site;
  local.addressValue = result;
  append(&fn->localVariables, local);

  return result;
}

uint32_t irInstLoad(IRFunction *fn, IRBasicBlockIndex bIndex, Type *type, uint32_t ptrValue, Site site) {
  uint32_t result = createIRValue(fn);
  ASSERT(type->size <= 8, "Load works only for primitive types");
  fn->values[result].type = type;
  IRInstruction *load = createIRInstruction(fn, bIndex, INSTRUCTION_LOAD, site);
  load->load.ret = result;
  load->load.from = ptrValue;

  return result;
}

void irInstStore(IRFunction *fn, IRBasicBlockIndex bIndex, uint32_t ptrValue, uint32_t value, Site site) {
  IRInstruction *store = createIRInstruction(fn, bIndex, INSTRUCTION_STORE, site);
  store->store.to = ptrValue;
  store->store.value = value;
}

const char *instructionTypeToString(int type) {
  #define XX(INSTRUCTION) case INSTRUCTION: return #INSTRUCTION;
  switch (type) {
  INSTRUCTIONS
  default: ASSERT(false, "unexpected instruction type value");
  }
  #undef XX
}


void printInstruction(IRFunction *fn, IRInstruction *instruction, int index, FILE *f) {
  fprintf(f, "  %4d: %3d %-20s|", index, instruction->type, instructionTypeToString(instruction->type) + 12);
  //+12 skips INSTRUCTION_
  if (instruction->site.file) {
    fprintf(f, "%15s:%3d| ", files[instruction->site.line].relative_path, instruction->site.line);
  } else {
    fprintf(f, "%15s:%3d| ", "(internal)", 0);
  }
  switch (instruction->type) {
    case INSTRUCTION_ALLOC_STACK_NODE: {
      fprintf(f, " alloc_stack_node\n");
      //fprintf(f, " jump to %s(%u)\n",
      //  fn->basicBlocks[instruction->jump.block].name, instruction->jump.block);
    } break;
    case INSTRUCTION_JUMP: {
      fprintf(f, " jump to %s(%u)\n",
        fn->basicBlocks[instruction->jump.block].name, instruction->jump.block);
    } break;
    case INSTRUCTION_RET_VOID: {
      fprintf(f, " ret\n");
    } break;
    case INSTRUCTION_RET: {
      fprintf(f, " ret v%u\n", instruction->retValue);
    } break;
    case INSTRUCTION_IADD: {
      fprintf(f, " ");
      printType(fn->values[instruction->binaryOp.ret].type, f);
      fprintf(f, " v%u = iadd v%u, v%u\n", instruction->binaryOp.ret, instruction->binaryOp.op1, instruction->binaryOp.op2);
    } break;

    case INSTRUCTION_PUSH_ARG: {
      fprintf(f, " push_arg v%u\n", instruction->arg.argValue);
    } break;

    case INSTRUCTION_CALL: {
      fprintf(f, " v%u = call %p %.*s (", instruction->call.ret, instruction->call.fn, (int)instruction->call.fn->name.len,
        instruction->call.fn->name.data);
      int argsNumber = instruction->call.argsNumber;
      for (int i = 0; i < argsNumber; ++i) {
        if (i != 0) fprintf(f, ", ");
        //fprintf(f, "v%u", instruction->call.args[i]);
        fprintf(f, "v%u", fn->instructions[index - argsNumber + i].arg.argValue);
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

    case INSTRUCTION_STORE: {
      //fprintf(f, " store value v%u to address v%u\n", instruction->store.value, instruction->store.to);
      fprintf(f, " *v%u = v%u\n", instruction->store.to, instruction->store.value);
    } break;

    case INSTRUCTION_LOAD: {
      fprintf(f, " ");
      printType(fn->values[instruction->load.ret].type, f);
      fprintf(f, " v%u = load v%u\n", instruction->load.ret, instruction->load.from);
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
  fprintf(f, "  returns ");
  printType(fn->retType, f);
  fprintf(f, "\n");

  for (int i = 0; i < fn->argsNumber; ++i) {
    fprintf(f, "  ");
    printType(fn->values[i].type, f);
    fprintf(f, " v%d = arg[%d] ", i, i);
    fprintf(f, "\n");
  }

  if (fn->flags & FUNCTION_FLAG_VARIADIC) {
    fprintf(f, "\t...(variadic)\n");
  }

  if (fn->flags & FUNCTION_FLAG_EXTERNAL) {
    return;
  }

  for (int i = 0; i < fn->localVariables.len; ++i) {
    Type *ptrType = fn->values[fn->localVariables[i].addressValue].type;
    fprintf(f, "  ");
    printType(ptrType, f);
    fprintf(f, " v%u = stack variable", fn->localVariables[i].addressValue);
    fprintf(f, " ");
    printType(fn->localVariables[i].type, f);
    fprintf(f, " (size = %hu, alignment = %hu)\n", fn->localVariables[i].type->size, fn->localVariables[i].type->alignment);

  }

  for (int i = 0; i < fn->basicBlocks.len; ++i) {
    IRBasicBlock_ *block = &fn->basicBlocks[i];
    fprintf(f, "\n");
    fprintf(f, "  %s(%d):\n", block->name, i);

    for (int i = 0; i < block->length; ++i) {
      IRInstruction *instruction = &fn->instructions[block->offset + i];
      printInstruction(fn, instruction, block->offset + i, f);
    }
  }
  fprintf(f, "end of %.*s:\n\n", (int)fn->name.len, fn->name.data);
}

void printAll(IRHub *h, FILE *f) {
  for (int functionIndex = 0; functionIndex < h->functions.len; ++functionIndex) {
    printFunction(h->functions[functionIndex], f);
  }
}
