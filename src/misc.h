#pragma once

#include <functional>
#include <utility>

#include <SDL2/SDL.h>

#include "math.h"
#include "ownedptr.h"
#include "smallarray.h"
#include "array.h"
#include "map.h"
#include "str.h"
#include "buffer.h"

Buffer g_tmpMem = Buffer(megabytes(32), 16);

char* tmpStr(const char* format, ...)
{
    char* buf = g_tmpMem.get<char*>();

    va_list argptr;
    va_start(argptr, format);
    auto count = stbsp_vsnprintf(buf, 4096, format, argptr);
    va_end(argptr);

    g_tmpMem.bump(count);

    return buf;
}

void print(const char* format, ...)
{
    auto callback = [](const char* buf, void* user, int len) -> char* {
        char tmp[STB_SPRINTF_MIN + 1];
        memcpy(tmp, buf, len);
        tmp[len] = '\0';
        fputs(tmp, stdout);
        return (char*)buf;
    };

    char tmp[STB_SPRINTF_MIN];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsprintfcb(callback, nullptr, tmp, format, argptr);
    va_end(argptr);
}

void println(const char* format="", ...)
{
#if 1
    auto callback = [](const char* buf, void* user, int len) -> char* {
        char tmp[STB_SPRINTF_MIN + 1];
        memcpy(tmp, buf, len);
        tmp[len] = '\0';
        fputs(tmp, stdout);
        return (char*)buf;
    };

    char tmp[STB_SPRINTF_MIN];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsprintfcb(callback, nullptr, tmp, format, argptr);
    va_end(argptr);
#else
    char tmp[2048];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsnprintf(tmp, sizeof(tmp), format, argptr);
    va_end(argptr);
    fputs(tmp, stdout);
#endif
    fputc('\n', stdout);
}

void error(const char* format="", ...)
{
    auto callback = [](const char* buf, void* user, int len) -> char* {
        char tmp[STB_SPRINTF_MIN + 1];
        memcpy(tmp, buf, len);
        tmp[len] = '\0';
        fputs(tmp, stderr);
        return (char*)buf;
    };

    char tmp[STB_SPRINTF_MIN];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsprintfcb(callback, nullptr, tmp, format, argptr);
    va_end(argptr);
    putc('\n', stderr);
}

void showError(const char* format, ...)
{
    char buf[1024];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsnprintf(buf, sizeof(buf), format, argptr);
    va_end(argptr);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", buf, nullptr); \
}

#define FATAL_ERROR(...) { error(__VA_ARGS__); showError(__VA_ARGS__); abort(); }

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

inline f64 getTime()
{
    static f64 freq = (f64)SDL_GetPerformanceFrequency();
    return (f64)SDL_GetPerformanceCounter() / freq;
}

inline RandomSeries randomSeed()
{
    return RandomSeries{ (u32)getTime() };
}

namespace path
{
    bool hasExt(const char* str, const char* ext)
    {
        return strcmp(str + (strlen(str) - strlen(ext)), ext) == 0;
    }
}
