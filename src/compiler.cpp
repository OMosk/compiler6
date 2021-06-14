#include "compiler.hpp"

#define DECLARE_TYPE(NAME, FLAGS, SIZE, ALIGNMENT) \
  static Type NAME ## _ = (Type){.flags = (FLAGS), .size = (SIZE), .alignment = (ALIGNMENT)};\
  Type *NAME = &NAME ## _
//Type *u8 = &(Type){.flags = TYPE_FLAG_INT};

DECLARE_TYPE(u8,  TYPE_PRIMITIVE|TYPE_FLAG_INT, 1, 1);
DECLARE_TYPE(u16, TYPE_PRIMITIVE|TYPE_FLAG_INT, 2, 2);
DECLARE_TYPE(u32, TYPE_PRIMITIVE|TYPE_FLAG_INT, 4, 4);
DECLARE_TYPE(u64, TYPE_PRIMITIVE|TYPE_FLAG_INT, 8, 8);

DECLARE_TYPE(i8,  TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 1, 1);
DECLARE_TYPE(i16, TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 2, 2);
DECLARE_TYPE(i32, TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 4, 4);
DECLARE_TYPE(i64, TYPE_PRIMITIVE|TYPE_FLAG_INT|TYPE_FLAG_UNSIGNED, 8, 8);

DECLARE_TYPE(f32, TYPE_PRIMITIVE|TYPE_FLAG_FLOATING_POINT, 4, 4);
DECLARE_TYPE(f64, TYPE_PRIMITIVE|TYPE_FLAG_FLOATING_POINT, 8, 8);

#undef DECLARE_TYPE


Names builtin;

void initBuiltinTypes() {
  //registerName(&builtin, STR("u8"), )
}

Array<Type *> pointeeTypes;
Array<Type *> pointerTypes;

Type *getPointerToType(Type *pointee) {
  //TODO: concurrency
  for (int i = 0; i < pointeeTypes.len; ++i) {
    if (pointeeTypes[i] == pointee) {
      return pointerTypes[i];
    }
  }

  Type *result = ALLOC(Type);

  *result = {};
  result->flags = TYPE_PRIMITIVE | TYPE_FLAG_POINTER;
  result->size = 8;
  result->alignment = 8;
  result->pointerToType = pointee;
  append(&pointeeTypes, pointee);
  append(&pointerTypes, result);
  return result;
}
