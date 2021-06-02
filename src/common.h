#pragma once

#if _WIN32
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4244)
#endif

#include <assert.h>
#include <new>
#include <initializer_list>
#include "template_magic.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef float              f32;
typedef double             f64;

#define FORCE_INLINE __attribute__((always_inline))
#define FORCE_FLATTEN __attribute__((flatten))
