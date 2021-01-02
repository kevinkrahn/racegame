#pragma once

#include <utility>
#include <functional>
#include <SDL2/SDL_assert.h>

#include "template_magic.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define assert(...) SDL_assert(__VA_ARGS__);

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef float f32;
typedef double f64;
