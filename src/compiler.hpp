#pragma once

#include <inttypes.h>
#include "utils.hpp"

struct NamedEntry {
  Str name;
  //ASTNode *node;
};


struct Names{
  Array<NamedEntry> entries;
};

//void registerName(Names *ns, Str name, ASTNode *node);

//struct ASTModule: ASTNode {
//  Names names;
//};


extern Names builtin;
void initBuiltinTypes();

enum {
  TYPE_PRIMITIVE           = (1 << 0),
  TYPE_FLAG_INT            = (1 << 1),
  TYPE_FLAG_UNSIGNED       = (1 << 2),
  TYPE_FLAG_FLOATING_POINT = (1 << 3),
  TYPE_FLAG_POINTER        = (1 << 4),
};

struct Type {
  uint32_t flags;
  uint16_t size; //bytes
  uint16_t alignment; //bytes

  Type *pointerToType;
};

extern Type *i8;
extern Type *i16;
extern Type *i32;
extern Type *i64;

extern Type *u8;
extern Type *u16;
extern Type *u32;
extern Type *u64;

extern Type *f32;
extern Type *f64;

Type *getPointerToType(Type *pointee);
