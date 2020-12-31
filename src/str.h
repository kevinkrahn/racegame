#pragma once

#include <utility>
#include <string.h>
#include <stb_sprintf.h>
#include "math.h"

template <u32 SIZE>
class Str
{
    char buf_[SIZE] = {0};

public:
    static constexpr u32 MAX_SIZE = SIZE;

    Str() {};
    Str(const char* s) { *this = s; }
    Str(const char* begin, const char* end) { memcpy(buf_, begin, min(SIZE, end-begin)); }
    void write(const char* format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        stbsp_vsnprintf(buf_, SIZE, format, argptr);
        va_end(argptr);
    }
    static Str format(const char* format, ...)
    {
        Str<SIZE> s;
        va_list argptr;
        va_start(argptr, format);
        stbsp_vsnprintf(s.buf_, SIZE, format, argptr);
        va_end(argptr);
        return s;
    }
    Str(Str const& other) { *this = other; }
    Str(Str && other) { *this = other; }
    Str& operator=(Str const& other) = default;
    Str& operator=(Str && other) = default;
    Str& operator=(const char* s) { strncpy(buf_, s, SIZE-1); return *this; }
    bool operator==(Str const& other) const { return strncmp(buf_, other.buf_, SIZE) == 0; }
    bool operator==(const char* s) const { return strncmp(buf_, s, SIZE) == 0; }
    const char* find(const char* needle) const { return strstr(buf_, needle); }
    const char* find(Str const& needle) const { return strstr(buf_, needle.buf_); }
    char* data() const { return (char*)buf_; }
    char const& operator[](u32 index) const { return buf_[index]; }
    char& operator[](u32 index) { return buf_[index]; }
    u32 size() const { return strlen(buf_); }
    bool empty() const { return size() == 0; }
    Str substr(u32 from, u32 len=0) const
    {
        return Str(buf_ + from, len == 0 ? buf_ + size() : buf_ + from + len);
    }
    bool operator<(Str const& rhs) { return strcmp(buf_, rhs.buf_) < 0; }
    bool operator>(Str const& rhs) { return strcmp(buf_, rhs.buf_) > 0; }
};

using Str16 = Str<16>;
using Str32 = Str<32>;
using Str64 = Str<64>;
using Str512 = Str<512>;

class StrBuf
{
    char* buf_ = nullptr;
    u32 capacity_ = 0;
    u32 len_ = 0;

    void ensureCapacity(u32 capacity)
    {
        if (capacity_ < capacity)
        {
            capacity_ = capacity + capacity_ / 4;
            buf_ = (char*)realloc(buf_, capacity_);
        }
    }

public:
    ~StrBuf()
    {
        if (buf_)
        {
            free(buf_);
        }
    }
    StrBuf() {}
    StrBuf(u32 size) : len_(size)
    {
        ensureCapacity(size);
        buf_[0] = 0;
    }
    StrBuf(StrBuf&& other)
    {
        *this = std::move(other);
    }

    StrBuf(StrBuf const& other) = delete;
    StrBuf& operator=(StrBuf const& other) = delete;

    StrBuf& operator=(StrBuf&& other)
    {
        this->buf_ = other.buf_;
        this->capacity_ = other.capacity_;
        this->len_ = other.len_;
        other.buf_ = nullptr;
        other.capacity_ = 0;
        other.len_ = 0;
        return *this;
    }

    void write(char ch, u32 n=1)
    {
        ensureCapacity(len_ + n + 1);
        for (u32 i=0; i<n; ++i)
        {
            buf_[len_+i] = ch;
        }
        len_ += n;
        buf_[len_] = '\0';
    }

    void write(const char* s)
    {
        write(s, strlen(s));
    }

    void write(const char* s, u32 n)
    {
        ensureCapacity(len_ + n + 1);
        memcpy(buf_ + len_, s, n);
        len_ += n;
        buf_[len_] = '\0';
    }

    template <u32 SIZE>
    void write(Str<SIZE> str)
    {
        write(str.data());
    }

    void writef(const char* format, ...)
    {
        auto callback = [](const char* buf, void* user, int len) -> char* {
            StrBuf* strbuf = (StrBuf*)user;
            strbuf->write(buf, (u32)len);
            return (char*)buf;
        };

        char tmp[STB_SPRINTF_MIN];
        va_list argptr;
        va_start(argptr, format);
        stbsp_vsprintfcb(callback, this, tmp, format, argptr);
        va_end(argptr);
    }

    const char* find(const char* needle) const { return strstr(buf_, needle); }

    void clear()
    {
        len_ = 0;
        if (buf_)
        {
            buf_[0] = '\0';
        }
    }

    char* data() const { return buf_; }
    u32 size() const { return len_; }
    char* begin() { return buf_; }
    char* end() { return buf_ + len_; }
    const char* begin() const { return buf_; }
    const char* end() const { return buf_ + len_; }
};
