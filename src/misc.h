#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <functional>
#include <utility>

#include <SDL2/SDL.h>

#include "math.h"
#include "ownedptr.h"
#include "smallarray.h"
#include "array.h"
#include "map.h"

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

template <typename T>
void _str_(std::ostringstream& ss, T const& val)
{
    ss << val;
}

template <typename T, typename... Args>
void _str_(std::ostringstream& ss, T const& first, Args const&... args)
{
    _str_(ss, first);
    _str_(ss, args...);
}

std::ostringstream _ss_;
template <typename... Args>
std::string str(Args const&... args)
{
    _ss_.str({});
    _str_(_ss_, args...);
    return _ss_.str();
}

#define FATAL_ERROR(...) { error("Error: ", __VA_ARGS__, '\n'); \
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", str(__VA_ARGS__).c_str(), nullptr); \
    abort(); }\

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

template <typename... Args>
inline void showError(Args const&... args)
{
    std::string msg = str(args...);
    error(msg, '\n');
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warning", msg.c_str(), nullptr); \
}

inline f64 getTime()
{
    static f64 freq = (f64)SDL_GetPerformanceFrequency();
    return (f64)SDL_GetPerformanceCounter() / freq;
}

inline RandomSeries randomSeed()
{
    return RandomSeries{ (u32)getTime() };
}
