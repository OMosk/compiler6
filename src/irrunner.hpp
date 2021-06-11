#pragma once

#include "ir.hpp"

union Value {
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

Value runIR(ir::Function *fn, Array<Value> args);
