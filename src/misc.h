#pragma once

#include "common.h"
#include "math.h"
#include "ownedptr.h"
#include "smallarray.h"
#include "array.h"
#include "map.h"
#include "str.h"
#include "buffer.h"

#include <SDL2/SDL.h>

Buffer g_tmpMem = Buffer(megabytes(32), 16);

char* tmpStr(const char* format, ...)
{
    char* buf = g_tmpMem.get<char*>();

    va_list argptr;
    va_start(argptr, format);
    auto count = stbsp_vsnprintf(buf, 4096, format, argptr);
    va_end(argptr);

    // TODO: guard against overflow
    g_tmpMem.bump(count+1);

    return buf;
}

Str32 hex(i64 n)
{
    u32* b = (u32*)&n;
    return Str32::format("%x%x", b[1], b[0]);
}

char* printCallback(const char* buf, void* user, int len)
{
    char tmp[STB_SPRINTF_MIN + 1];
    memcpy(tmp, buf, len);
    tmp[len] = '\0';
    fputs(tmp, (FILE*)user);
    return (char*)buf;
}

void print(const char* format, ...)
{
    char tmp[STB_SPRINTF_MIN];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsprintfcb(printCallback, stdout, tmp, format, argptr);
    va_end(argptr);
}

void println(const char* format="", ...)
{
    char tmp[STB_SPRINTF_MIN];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsprintfcb(printCallback, stdout, tmp, format, argptr);
    va_end(argptr);
    fputc('\n', stdout);
}

void error(const char* format="", ...)
{
    char tmp[STB_SPRINTF_MIN];
    va_list argptr;
    va_start(argptr, format);
    stbsp_vsprintfcb(printCallback, stderr, tmp, format, argptr);
    va_end(argptr);
    fputc('\n', stderr);
}

#define showError(...) { SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", tmpStr(__VA_ARGS__), nullptr); }
#define FATAL_ERROR(...) { error(__VA_ARGS__); showError(__VA_ARGS__); abort(); }

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

    // TODO: move this into str.h
    const char* relative(const char* path)
    {
        char* buf = tmpStr("%s", path);
        for (char* ch = buf; *ch; ++ch)
        {
            if (*ch == '\\')
            {
                *ch = '/';
            }
        }
        const char* needle = "assets/";
        const char* assetsFolder = strstr(buf, needle);
        if (!assetsFolder)
        {
            return "";
        }
        return tmpStr("%s", assetsFolder + strlen(needle));
    }
}
