#include "irrunner.hpp"
#include "ir.hpp"

#include <sys/mman.h>
#include <dlfcn.h>

Value trampoline_generator_with_ptr3(void *fn_ptr, int returns_float, int args_count, Value *args, bool *is_integer) {
  static thread_local char *buffer = (char *) mmap(0, 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (!buffer) {
    printf("Failed to mmap page\n");
    abort();
  }
  //current implementation omits frame pointer
  unsigned char *it = (unsigned char *)buffer;
  *it++ = 0x50; /*stack alignment: push rax //shift stack by 8bytes from clang */
  //*it++ = 0x48; *it++ = 0x83; *it++ = 0xec; *it++ = 0x08; /*stack alignment:  sub rsp, 0x8: from gcc*/

  int integer_registers_used = 0;
  int floating_point_registers_used = 0;
  for (int i = 0; i < args_count; ++i) {
    if (is_integer[i]) {
      switch (integer_registers_used++) {
      case 0: { *it++ = 0x48; *it++ = 0xbf;} break;/*mov rdi, 64bit-literal*/
      case 1: { *it++ = 0x48; *it++ = 0xbe;} break;/*mov rsi, 64bit-literal*/
      case 2: { *it++ = 0x48; *it++ = 0xba;} break;/*mov rdx, 64bit-literal*/
      case 3: { *it++ = 0x48; *it++ = 0xb9;} break;/*mov rcx, 64bit-literal*/
      case 4: { *it++ = 0x49; *it++ = 0xb8;} break;/*mov r8, 64bit-literal*/
      case 5: { *it++ = 0x49; *it++ = 0xb9;} break;/*mov r9, 64bit-literal*/
      default: {
        printf("more than 6 integer args is not supported yet\n");
        abort();
      }
      }

      for (int byte = 0; byte < 8; ++byte) {
        *it++ = ((unsigned char *)&args[i])[byte]; //copying bytes of value to fill imm64
      }
    } else {
      *it++ = 0x48; *it++ = 0xb8;/*mov rax, imm64*/
      for (int byte = 0; byte < 8; ++byte) {
        *it++ = ((unsigned char *)&args[i])[byte]; //copying bytes of value to fill imm64
      }
      switch (floating_point_registers_used++) {
        case 0: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xc0;} break; /*mov xmm0, rax*/
        case 1: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xc8;} break; /*mov xmm1, rax*/
        case 2: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xd0;} break; /*mov xmm2, rax*/
        case 3: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xd8;} break; /*mov xmm3, rax*/
        case 4: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xe0;} break; /*mov xmm4, rax*/
        case 5: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xe8;} break; /*mov xmm5, rax*/
        case 6: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xf0;} break; /*mov xmm6, rax*/
        case 7: {*it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x6e; *it++ = 0xf8;} break; /*mov xmm7, rax*/
        default: {
          printf("more than 8 floating point is not supported yet\n");
          abort();
        }
      }
    }
  }

  //*it++ = 0xb0; *it++ = 0x00; /*explicitly nullify return value register: mov al, 0x0*/

  *it++ = 0x48/*REX.W 64bit operand*/; *it++ = 0xb8/*mov to rax*/;
  for (int i = 0; i < 8; ++i) {
    *it++ = ((unsigned char *)&fn_ptr)[i]; //copying bytes of address to fill imm64
  }
  *it++ = 0xff; *it++ = 0xd0; /*call rax*/
  //*it++ = 0x48; *it++ = 0x83; *it++ = 0xc4; *it++ = 0x08; /*stack alignment:  add rsp, 0x8: from gcc*/
  *it++ = 0x59; /*stack alignment: pop rcx; add 8 bytes back upwords from clang*/
  if (returns_float) {
    //assign xmm0(register with floating point result) to rax,
    //movq rax, xmm0
    *it++ = 0x66; *it++ = 0x48; *it++ = 0x0f; *it++ = 0x7e; *it++ = 0xc0;
  }
  *it++ = 0xc3; /*ret*/

  size_t len = (char *)it - buffer;
  //printf("JITted %lu bytes\n", len);

  uint64_t (*fn)() = (uint64_t(*)())buffer;
  Value result;
  result.u64 = fn();
  return result;
}


void internalRun(ir::Function *fn, Array<Value> *registers, int registersOffset, Str stack, int stackOffset) {
  resize(registers, registersOffset + fn->valuesNumber);

  ir::BasicBlock *block = fn->init;
  auto instructionIt = iterator(&block->instructions);
  ir::Instruction *instruction = instructionIt.next();
  if (!instruction) {
    instruction = &block->terminator;
  }
  int maxArgIndex = 0;
  for (;;) {
    switch (instruction->type) {
    case ir::INSTRUCTION_JUMP: {
      block = instruction->jump.block;
      instructionIt = iterator(&block->instructions);
    } break;

    case ir::INSTRUCTION_IADD: {
     (*registers)[registersOffset + instruction->binaryOp.ret].u64
       = (*registers)[registersOffset + instruction->binaryOp.op1].u64 +
         (*registers)[registersOffset + instruction->binaryOp.op2].u64;
    } break;

    case ir::INSTRUCTION_RET: {
      (*registers)[registersOffset - 1] = (*registers)[registersOffset + instruction->retValue];
      return;
    } break;

    //case ir::INSTRUCTION_SET_ARG: {
    //  ensureAtLeast(registers, registersOffset + fn->valuesNumber + 2 + instruction->setArg.argNo); // return register + arg registers from zero
    //  (*registers)[registersOffset + fn->valuesNumber + 1 + instruction->setArg.argNo] = (*registers)[registersOffset + instruction->setArg.op];
    //  maxArgIndex = instruction->setArg.argNo;
    //} break;

    case ir::INSTRUCTION_CALL: {
      ir::Function *callee = instruction->call.fn;
      if (callee->flags & ir::FUNCTION_FLAG_EXTERNAL) {
        if (!callee->ptr) {
          callee->ptr = dlsym(RTLD_DEFAULT, callee->symbolName.data);
          //load symbol from already loaded libraries
          if (!callee->ptr) {
            NOT_IMPLEMENTED;
          }
        }
        int argsCount = instruction->call.args.len;
        bool isIntegers[instruction->call.args.len];
        Value values[instruction->call.args.len];
        for (int argNo = 0; argNo < instruction->call.args.len; ++argNo) {
          values[argNo] = (*registers)[registersOffset + instruction->call.args[argNo]];
          isIntegers[argNo] = (fn->values[instruction->call.args[argNo]].type->flags & TYPE_FLAG_FLOATING_POINT) == 0;
        }
        bool returnIsFloat = callee->retType->flags&TYPE_FLAG_FLOATING_POINT;
        double start = now();
        Value result = trampoline_generator_with_ptr3(callee->ptr, returnIsFloat, argsCount, values, isIntegers);
        double end = now();
        printf("Trampoline JIT + printf call took: %fsec\n", end - start);
        double start2  = now();
        printf("Hello world\n");
        double end2 = now();
        printf("Regular          printf call took: %fsec\n", end2 - start2);
        (*registers)[registersOffset + instruction->call.ret] = result;
      } else {
        ensureAtLeast(registers, registersOffset + fn->valuesNumber + 1 + instruction->call.args.len); // return register + args
        for (int argNo = 0; argNo < instruction->call.args.len; ++argNo) {
          (*registers)[registersOffset + fn->valuesNumber + 1 + argNo] = (*registers)[registersOffset + instruction->call.args[argNo]];
        }
        //TODO: stackOffset alignment may be needed
        internalRun(instruction->call.fn, registers, registersOffset + fn->valuesNumber + 1, stack, stackOffset);
        (*registers)[registersOffset + instruction->call.ret] = (*registers)[registersOffset + fn->valuesNumber];
      }
      maxArgIndex = 0;
    } break;

    default:
      printf("%d %s instruction not implemented\n", instruction->type, ir::instructionTypeToString(instruction->type));
    NOT_IMPLEMENTED;
    }

    instruction = instructionIt.next();
    if (!instruction) {
      instruction = &block->terminator;
    }
  }
}

Value runIR(ir::Function *fn, Array<Value> args) {
  //NOTE: function runs in temporary area
  char *allocatorCurrent = global.current;

  ASSERT(args.len == fn->argsNumber, "wrong number of args");
  Array<Value> registers = {};
  resize(&registers, 1/*return register*/ + args.len);
  registers[0].i64 = 0;
  for (int i = 0; i < args.len; ++i) {
    registers[i + 1] = args[i];
  }

  Str stack = {};
  stack.len = 8 * 1024;
  stack.data = (char *)alloc(stack.len, 8/*alignment*/, 1); // Think through if we need to match 16 byte alignment as int amd64 convention

  internalRun(fn, &registers, 1, stack, 0);

  Value result = registers[0];

  //NOTE: restoring temporary area to previous state
  global.current = allocatorCurrent;

  return result;
}
