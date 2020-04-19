#pragma once

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <vector>
#include <SDL2/SDL.h>
#include <dirent.h>

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

#define FATAL_ERROR(...) error("Error: ", __VA_ARGS__, '\n'); {\
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", str(__VA_ARGS__).c_str(), nullptr); \
    abort(); }\

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

inline f64 getTime()
{
    static f64 freq = (f64)SDL_GetPerformanceFrequency();
    return (f64)SDL_GetPerformanceCounter() / freq;
}

std::vector<std::string> readDirectory(const char* folder, const char* extension)
{
    std::vector<std::string> files;

#if _WIN32
    WIN32_FIND_DATA fileData;
    HANDLE handle = FindFirstFileA(folder, &fileData);
    if (handle == INVALID_HANDLE_VALUE)
    {
        FATAL_ERROR("Failed to read directory: ", folder);
    }
    do
    {
        if (!(fileData & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string name = fileData.cFileName;
            auto index = name.find_last_of('.');
            if (index != std::string::npos && name.substr(index) == extension)
            {
                files.push_back(name);
            }
        }
    }
    while (FindNextFileA(handle, &fileData) != 0);
    FindClose(handle);
#else
    DIR* dirp = opendir(folder);
    if (!dirp)
    {
        FATAL_ERROR("Failed to read directory: ", folder);
    }
    dirent* dp;
    while ((dp = readdir(dirp)))
    {
        if (dp->d_type == DT_REG)
        {
            std::string name = dp->d_name;
            auto index = name.find_last_of('.');
            if (index != std::string::npos && name.substr(index) == extension)
            {
                files.push_back(name);
            }
        }
    }
    closedir(dirp);
#endif

    return files;
}
