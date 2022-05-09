#pragma once

#include <stdio.h>

#include "core_types.h"

void report(ThreadData *ctx, FILE *out, const char *levelPrefix, int fileIndex,
            uint32_t offset0, uint32_t offset1, Str message);
