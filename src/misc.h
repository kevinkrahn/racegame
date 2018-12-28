#pragma once

#include <stdint.h>
#include <iostream>
#include <cassert>
#include <SDL2/SDL.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;

typedef uint32_t b32;

template <typename T>
void print(T const& val)
{
    std::cout << val;
}

template <typename T, typename... Args>
void print(T const& first, Args const&... args)
{
    print(first);
    print(args...);
}

template <typename T>
void error(T const& val)
{
    std::cerr << val;
}

template <typename T, typename... Args>
void error(T const& first, Args const&... args)
{
    error(first);
    error(args...);
}

#define FATAL_ERROR(...) error(__VA_ARGS__, '\n'); assert(0);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

inline f64 getTime()
{
    static f64 freq = (f64)SDL_GetPerformanceFrequency();
    return (f64)SDL_GetPerformanceCounter() / freq;
}
