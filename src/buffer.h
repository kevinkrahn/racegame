#pragma once

#include "misc.h"

constexpr size_t kilobytes(size_t bytes)
{
    return bytes * 1024;
}

constexpr size_t megabytes(size_t bytes)
{
    return kilobytes(bytes) * 1024;
}

inline size_t align(size_t x, size_t alignment)
{
    return (x + alignment - 1) / alignment * alignment;
}

class Buffer
{
public:
    std::unique_ptr<u8[]> data;
    size_t size;
    size_t pos;
    size_t alignment;

    Buffer() : data(NULL), size(0), pos(0), alignment(1) {}

    Buffer(size_t size, size_t alignment=1) : size(size), pos(0),  alignment(alignment)
    {
        if (size > 0)
        {
            data.reset(new u8[size]);
        }
    }

    Buffer(Buffer&& other)
    {
        data = std::move(other.data);
        size = other.size;
        pos = other.pos;
        alignment = other.alignment;
        other.data = nullptr;
        other.size = 0;
        other.pos = 0;
    }

    // non-copyable
    Buffer& operator=(Buffer const&) = delete;
    Buffer(Buffer const&) = delete;

    // move assignment
    Buffer& operator=(Buffer&& other)
    {
        data = std::move(other.data);
        size = other.size;
        pos = other.pos;
        alignment = other.alignment;
        other.size = 0;
        other.pos = 0;
        return *this;
    }

    u8* bump(size_t len)
    {
        u8* prev = data.get() + pos;

        pos += align(len, alignment);
        assert(pos <= size);

        return prev;
    }

    template <typename T>
    T* bump(size_t len=1)
    {
        return reinterpret_cast<T*>(bump(len * sizeof(T)));
    }

    u8* writeBytes(void* d, size_t len)
    {
        assert(len > 0);
        memcpy(data.get() + pos, d, len);
        return bump(len);
    }

    template <typename T>
    T* write(T const& v)
    {
        return reinterpret_cast<T*>(writeBytes((void*)&v, sizeof(T)));
    }

    void writeBuffer(const Buffer &buf)
    {
        writeBytes(buf.data.get(), buf.pos);
    }

    void clear(size_t pos=0)
    {
        this->pos = pos;
    }

    u8* get() const
    {
        return data.get() + pos;
    }

    template <typename T>
    T get() const
    {
        return reinterpret_cast<T>(get());
    }
};
