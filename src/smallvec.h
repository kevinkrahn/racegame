#pragma once

#include "misc.h"

template <typename T, u32 maxSize=8>
class SmallVec
{
private:
    union { T data_[maxSize]; };
    u32 size_;

public:
    SmallVec() : size_(0) {}

    SmallVec(std::initializer_list<T> list) : size_(list.size())
    {
        T* ptr = data_;
        for (auto& it : list)
        {
            new (ptr) T(it);
            ++ptr;
        }
    }

    explicit SmallVec(u32 size) : size_(size)
    {
        auto endPtr = data_ + size;
        for (T* ptr = data_; ptr != endPtr; ++ptr)
        {
            new (ptr) T;
        }
    }

    ~SmallVec()
    {
        for (auto& l : *this)
        {
            l.~T();
        }
    }

    T* data() const { return data_; }
    u32 size() const { return size_; }
    u32 capacity() const { return maxSize; }

    // copy push
    void push_back(const T& val)
    {
        assert(size_ < maxSize);
        new (data_ + size_) T(val);
        ++size_;
    }

    // move push
    void push_back(T&& val)
    {
        assert(size_ < maxSize);
        new (data_ + size_) T(std::move(val));
        ++size_;
    }

    // copy insert by index
    void insert(u32 index, const T& val)
    {
        assert(index <= size_);
        assert(size_ < maxSize);
        auto ptr = data_ + index;
        memmove(ptr+1, ptr, (size_ - index) * sizeof(T));
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
        memmove(ptr+1, ptr, (size_ - index) * sizeof(T));
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

    void erase(T * element)
    {
        assert(element < data_ + size_);
        element->~T();
        memmove(element, element+1, ((data_ + size_) - element) * sizeof(T));
        --size_;
    }

    void erase(u32 index)
    {
        erase(data_ + index);
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
};