#pragma once
#include "sysdeps.h"
#include "softfloat/softfloat.h"
typedef double fptype;
typedef struct { floatx80 fpx; fptype fp; } fpdata;
