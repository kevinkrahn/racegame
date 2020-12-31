#pragma once

#include <utility>
#include <assert.h>
#include "math.h"

template <typename T, u32 maxSize=8>
class SmallArray
{
    union { T data_[maxSize]; };
    u32 size_;

public:
    using value_type = T;

    SmallArray() : size_(0) {}

    SmallArray(std::initializer_list<T> list) : size_((u32)list.size())
    {
        T* ptr = data_;
        for (auto& it : list)
        {
            new (ptr) T(it);
            ++ptr;
        }
    }

    SmallArray(SmallArray const& other) : size_(other.size_)
    {
        for (u32 i=0; i<size_; ++i)
        {
            new (data_ + i) T(other.data_[i]);
        }
    }

    SmallArray(SmallArray&& other) : size_(other.size_)
    {
        for (u32 i=0; i<size_; ++i)
        {
            new (data_ + i) T(std::move(other.data_[i]));
        }
        other.size_ = 0;
    }

    explicit SmallArray(u32 size) : size_(size)
    {
        auto endPtr = data_ + size;
        for (T* ptr = data_; ptr != endPtr; ++ptr)
        {
            new (ptr) T;
        }
    }

    SmallArray(T* start, T* end)
    {
        assign(start, end);
    }

    ~SmallArray() { clear(); }

    SmallArray<T, maxSize>& operator = (SmallArray<T, maxSize> const& other)
    {
        clear();
        size_ = other.size_;
        for (u32 i=0; i<size_; ++i)
        {
            new (data_ + i) T(other.data_[i]);
        }
        return *this;
    }

    SmallArray<T, maxSize>& operator = (SmallArray<T, maxSize> && other)
    {
        clear();
        size_ = other.size_;
        for (u32 i=0; i<size_; ++i)
        {
            new (data_ + i) T(std::move(other.data_[i]));
        }
        return *this;
    }

    T* data() const { return data_; }
    u32 size() const { return size_; }
    u32 capacity() const { return maxSize; }

    // copy push
    void push(const T& val)
    {
        assert(size_ < maxSize);
        new (data_ + size_) T(val);
        ++size_;
    }

    // move push
    void push(T&& val)
    {
        assert(size_ < maxSize);
        new (data_ + size_) T(std::move(val));
        ++size_;
    }

    // merge two vecs into one
    SmallArray<T> concat(SmallArray<T> const& other) const
    {
        SmallArray<T> v;
        for (auto const& val : *this)
        {
            v.push(val);
        }
        for (auto const& val : other)
        {
            v.push(val);
        }
        return v;
    }

    // copy insert by index
    void insert(u32 index, const T& val)
    {
        assert(index <= size_);
        assert(size_ < maxSize);
        auto ptr = data_ + index;
        for (T* movePtr = end(); movePtr != data_ + index; --movePtr)
        {
            new (movePtr) T(std::move(*(movePtr - 1)));
        }
        ++size_;
        new (ptr) T(val);
    }

    // copy insert
    void insert(T * element, const T& val)
    {
        insert(data_ - element, val);
    }

    // move insert by index
    void insert(u32 index, T&& val)
    {
        assert(index <= size_);
        assert(size_ < maxSize);
        auto ptr = data_ + index;
        for (T* movePtr = end(); movePtr != data_ + index; --movePtr)
        {
            new (movePtr) T(std::move(*(movePtr - 1)));
        }
        ++size_;
        new (ptr) T(std::move(val));
    }

    // move insert
    void insert(T * element, T&& val)
    {
        insert(data_ - element, std::move(val));
    }

    void pop()
    {
        assert(size_ > 0);
        data_[size_ - 1].~T();
        size_--;
    }

    void clear()
    {
        for (auto& e : *this)
        {
            e.~T();
        }
        size_ = 0;
    }

    bool empty() const
    {
        return size_ == 0;
    }

    void resize(u32 newSize)
    {
        assert(newSize <= maxSize);

        // call destructors if shrinking
        if (newSize < size_)
        {
            for (T* ptr = data_ + newSize; ptr != data_ + size_; ++ptr)
            {
                ptr->~T();
            }
        }

        // initialize any new elements
        if (newSize > size_)
        {
            T* endPtr = data_ + newSize;
            for (T* ptr = data_ +size_; ptr != endPtr; ++ptr)
            {
                new (ptr) T;
            }
        }

        size_ = newSize;
    }

    // for compatibility with Array
    void reserve(u32 capacity) { /* no-op */ }

    void assign(T* start, T* end)
    {
        assert(start <= end);
        clear();
        size_ = end - start;
        for (u32 i=0; i<size_; ++i)
        {
            new (data_+i) T(start[i]);
        }
    }

    T* erase(T * element)
    {
        assert(element < data_ + size_);
        element->~T();
        for (u32 i=element - data_; i<size_-1; ++i)
        {
            new (data_ + i) T(std::move(data_[i+1]));
        }
        --size_;
        return element;
    }

    T* erase(u32 index)
    {
        return erase(data_ + index);
    }

    T& operator[](u32 index)
    {
        assert(index < size_);
        return ((T*)data_)[index];
    }

    T const& operator[](u32 index) const
    {
        assert(index < size_);
        return ((T*)data_)[index];
    }

    T* begin() { return (T*)data_; }
    T* end()   { return (T*)data_ + size_; }
    T const* begin() const { return (T*)data_; }
    T const* end()   const { return (T*)data_ + size_; }

    T& front()
    {
        assert(size_ > 0);
        return *((T*)data_);
    }

    T& back()
    {
        assert(size_ > 0);
        return *((T*)data_ + (size_ - 1));
    }

    T const& front() const
    {
        assert(size_ > 0);
        return *((T*)data_);
    }

    T const& back() const
    {
        assert(size_ > 0);
        return *((T*)data_ + (size_ - 1));
    }

    template <typename NEEDLE>
    T* find(NEEDLE const& needle)
    {
        for (auto it = begin(); it != end(); ++it)
        {
            if (*it == needle)
            {
                return it;
            }
        }
        return nullptr;
    }

    template <typename CB>
    T* findIf(CB const& cb)
    {
        for (auto it = begin(); it != end(); ++it)
        {
            if (cb(*it))
            {
                return it;
            }
        }
        return nullptr;
    }

    static constexpr u32 NONE = -1;
    template <typename NEEDLE>
    u32 findIndex(NEEDLE const& needle)
    {
        for (u32 i=0; i<size_; ++i)
        {
            if (data_[i] == needle)
            {
                return i;
            }
        }
        return NONE;
    }

    template <typename CB>
    u32 findIndexIf(CB const& cb)
    {
        for (u32 i=0; i<size_; ++i)
        {
            if (cb(data_[i]))
            {
                return i;
            }
        }
        return NONE;
    }

    void sort()
    {
        sort([](T const& a, T const& b) { return a < b; });
    }

    template <typename COMPARE>
    void sort(COMPARE const& compare)
    {
        for (u32 i=0; i<size_; ++i)
        {
            u32 j = i;
            while (j > 0 && compare(data_[j], data_[j-1]))
            {
                std::swap(data_[j-1], data_[j]);
                j--;
            }
        }
    }
};
