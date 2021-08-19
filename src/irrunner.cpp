#include "irrunner.hpp"
#include "ir.hpp"

#include <sys/mman.h>
#include <dlfcn.h>

IRValue trampoline_generator_with_ptr3(void *fn_ptr, int returns_float, int args_count, IRValue *args, bool *is_integer) {
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
  IRValue result;
  result.u64 = fn();
  return result;
}

struct InterpreterFrameData {
  IRFunction *fn;
  int registersOffset;
  int stackOffset;

  int instructionIndex;

  IRInstruction *instruction;
  InterpreterFrameData *prevFrame;
};

IRValue runIR(IRFunction *entryFunction, Array<IRValue> args) {
  //NOTE: function runs in temporary area
  char *allocatorCurrent = global.current;

  ASSERT(args.len == entryFunction->argsNumber, "wrong number of args");
  Array<IRValue> registers = {};
  resize(&registers, 1/*return register*/ + args.len);
  registers[0].i64 = 0;
  for (int i = 0; i < args.len; ++i) {
    registers[i + 1] = args[i];
  }

  Str stack = {};
  stack.len = 8 * 1024;
  stack.data = (char *)alloc(stack.len, 8/*alignment*/, 1); // Think through if we need to match 16 byte alignment as in amd64 convention

  Array<bool> isIntegers = {};
  Array<IRValue> argValues = {};

  InterpreterFrameData *currentFrame = NULL;
  {
    InterpreterFrameData *frame = (InterpreterFrameData *) stack.data;
    *frame = {};
    frame->fn = entryFunction;
    frame->registersOffset = 1;
    frame->stackOffset = sizeof(InterpreterFrameData);
    frame->instructionIndex = 0;
    //TODO maybe combine with resize for arguments earlier
    resize(&registers, frame->registersOffset + entryFunction->values.len);

    currentFrame = frame;
  }

  while (currentFrame) {
    currentFrame->instruction = &currentFrame->fn->instructions[currentFrame->instructionIndex];
    currentFrame->instructionIndex++;

    switch (currentFrame->instruction->type) {
    case INSTRUCTION_JUMP: {
      currentFrame->instructionIndex
        = currentFrame->fn->basicBlocks[currentFrame->instruction->jump.block].offset;
    } break;

    case INSTRUCTION_IADD: {
     registers[currentFrame->registersOffset + currentFrame->instruction->binaryOp.ret].u64
       = registers[currentFrame->registersOffset + currentFrame->instruction->binaryOp.op1].u64 +
         registers[currentFrame->registersOffset + currentFrame->instruction->binaryOp.op2].u64;
    } break;

    case INSTRUCTION_RET:
      registers[currentFrame->registersOffset - 1] = registers[currentFrame->registersOffset + currentFrame->instruction->retValue];
      /*fallthrough*/
    case INSTRUCTION_RET_VOID:
      currentFrame = currentFrame->prevFrame;
      if (currentFrame) {
        ASSERT(currentFrame->instruction->type == INSTRUCTION_CALL, "on return instruction is not CALL");
        registers[currentFrame->registersOffset + currentFrame->instruction->call.ret] = registers[currentFrame->registersOffset + currentFrame->fn->values.len];
      }
    break;

    case INSTRUCTION_CALL: {
      IRFunction *callee = currentFrame->instruction->call.fn;
      if (callee->flags & FUNCTION_FLAG_EXTERNAL) {
        if (!callee->ptr) {
          callee->ptr = dlsym(RTLD_DEFAULT, callee->symbolName.data);
          //NOTE: load symbol from already loaded libraries
          if (!callee->ptr) {
            //TODO: dlopen + dlsym should go here
            NOT_IMPLEMENTED;
          }
        }
        int argsCount = currentFrame->instruction->call.args.len;

        resize(&isIntegers, argsCount);
        resize(&argValues, argsCount);

        for (int argNo = 0; argNo < currentFrame->instruction->call.args.len; ++argNo) {
          argValues[argNo] = registers[currentFrame->registersOffset + currentFrame->instruction->call.args[argNo]];
          isIntegers[argNo] = (currentFrame->fn->values[currentFrame->instruction->call.args[argNo]].type->flags & TYPE_FLAG_FLOATING_POINT) == 0;
        }
        bool returnIsFloat = callee->retType->flags&TYPE_FLAG_FLOATING_POINT;
        double start = now();
        IRValue result = trampoline_generator_with_ptr3(callee->ptr, returnIsFloat, argsCount, argValues.data, isIntegers.data);
        double end = now();
        printf("Trampoline JIT + printf call took: %fsec\n", end - start);
        double start2  = now();
        printf("Hello world\n");
        double end2 = now();
        printf("Regular          printf call took: %fsec\n", end2 - start2);
        registers[currentFrame->registersOffset + currentFrame->instruction->call.ret] = result;
      } else {
        ensureAtLeast(&registers, currentFrame->registersOffset + currentFrame->fn->values.len + 1 + currentFrame->instruction->call.args.len); // return register + args
        for (int argNo = 0; argNo < currentFrame->instruction->call.args.len; ++argNo) {
          registers[currentFrame->registersOffset + currentFrame->fn->values.len + 1 + argNo] = registers[currentFrame->registersOffset + currentFrame->instruction->call.args[argNo]];
        }
        currentFrame->stackOffset = alignAddressUpwards(currentFrame->stackOffset, alignof(InterpreterFrameData));
        ASSERT(currentFrame->stackOffset + sizeof(InterpreterFrameData) < stack.len, "interpreter stack overflow on call instruction");
        InterpreterFrameData *newFrame = (InterpreterFrameData *)(stack.data + currentFrame->stackOffset);
        *newFrame = {};
        newFrame->fn = currentFrame->instruction->call.fn;
        newFrame->registersOffset = currentFrame->registersOffset + currentFrame->fn->values.len + 1;
        newFrame->stackOffset = currentFrame->stackOffset + sizeof(InterpreterFrameData);
        newFrame->prevFrame = currentFrame;
        currentFrame = newFrame;

        resize(&registers, currentFrame->registersOffset + entryFunction->values.len);
      }
    } break;

    case INSTRUCTION_CONSTANT: {
      registers[currentFrame->registersOffset + currentFrame->instruction->constantInit.valueIndex] = currentFrame->instruction->constantInit.constant;
    } break;

    case INSTRUCTION_ALLOCA: {
      char *addr = stack.data + currentFrame->stackOffset;
      ASSERT(currentFrame->stackOffset + currentFrame->instruction->alloca.type->size < stack.len, "interpreter stack overflow on alloca");
      currentFrame->stackOffset = alignAddressUpwards(currentFrame->stackOffset, currentFrame->instruction->alloca.type->alignment);
      currentFrame->stackOffset += currentFrame->instruction->alloca.type->size;
      registers[currentFrame->registersOffset + currentFrame->instruction->alloca.valueIndex].ptr = addr;
    } break;

    case INSTRUCTION_STORE: {
      IRValue data = registers[currentFrame->registersOffset + currentFrame->instruction->store.value];
      uint32_t sizeBytes = currentFrame->fn->values[currentFrame->instruction->store.value].type->size;
      IRValue *target = (IRValue *)registers[currentFrame->registersOffset + currentFrame->instruction->store.to].ptr;
      switch (sizeBytes) {
        case 1: target->u8 = data.u8; break;
        case 2: target->u16 = data.u16; break;
        case 4: target->u32 = data.u32; break;
        case 8: target->u64 = data.u64; break;
        default: {
          printf("Unexpected data size in INSTRUCTION_STORE: %u\n", sizeBytes);
          NOT_IMPLEMENTED;
        } break;
      }
    } break;

    case INSTRUCTION_LOAD: {
      IRValue *target = &registers[currentFrame->registersOffset + currentFrame->instruction->load.ret];

      IRValue *data = (IRValue *) registers[currentFrame->registersOffset + currentFrame->instruction->load.from].ptr;
      uint32_t sizeBytes = currentFrame->fn->values[currentFrame->instruction->load.ret].type->size;
      switch (sizeBytes) {
        case 1: target->u8 = data->u8; break;
        case 2: target->u16 = data->u16; break;
        case 4: target->u32 = data->u32; break;
        case 8: target->u64 = data->u64; break;
        default: {
          printf("Unexpected data size in INSTRUCTION_LOAD: %u\n", sizeBytes);
          NOT_IMPLEMENTED;
        } break;
      }
    } break;

    default:
      printf("instruction[%d] %d %s instruction not implemented\n", currentFrame->instructionIndex, currentFrame->instruction->type, instructionTypeToString(currentFrame->instruction->type));
    NOT_IMPLEMENTED;
    }

    continue;

  }

  IRValue result = registers[0];

  //NOTE: restoring temporary area to previous state
  //Maybe it is better if caller restores temporary area after deep copy is made if necessary
  global.current = allocatorCurrent;

  return result;
}
