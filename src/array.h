#pragma once

#include "math.h"
#include "template_magic.h"

template <typename T>
class Array
{
    T* data_ = nullptr;
    u32 size_ = 0;
    u32 capacity_ = 0;

    void reallocate()
    {
        if constexpr (std::is_trivial<T>::value)
        {
            data_ = (T*)realloc(data_, capacity_ * sizeof(T));
        }
        else
        {
            T* newData = (T*)malloc(capacity_ * sizeof(T));
            if (data_)
            {
                for (u32 i=0; i<size_; ++i)
                {
                    new (newData + i) T(move(data_[i]));
                }
                free(data_);
            }
            data_ = newData;
        }
    }

    void ensureCapacity()
    {
        if (size_ < capacity_)
        {
            return;
        }
        capacity_ += capacity_ / 2 + 4;
        reallocate();
    }

    template <typename COMPARE>
    void quickSort(i32 lowIndex, i32 highIndex, COMPARE const& compare)
    {
        if (lowIndex < highIndex)
        {
            i32 swapIndex = lowIndex - 1;
            T const& pivot = data_[highIndex];
            for (i32 i=lowIndex; i<highIndex; ++i)
            {
                if (compare(data_[i], pivot))
                {
                    swap(data_[++swapIndex], data_[i]);
                }
            }
            swap(data_[swapIndex + 1], data_[highIndex]);
            i32 middleIndex = swapIndex + 1;
            quickSort(lowIndex, middleIndex - 1, compare);
            quickSort(middleIndex + 1, highIndex, compare);
        }
    }

public:
    using value_type = T;

    Array() {}

    Array(std::initializer_list<T> list) : size_(list.size()), capacity_(list.size())
    {
        reallocate();
        T* ptr = data_;
        for (auto& it : list)
        {
            new (ptr) T(it);
            ++ptr;
        }
    }

    explicit Array(u32 size) : size_(size), capacity_(size)
    {
        reallocate();
        if constexpr (!std::is_trivial<T>::value)
        {
            auto endPtr = data_ + size;
            for (T* ptr = data_; ptr != endPtr; ++ptr)
            {
                new (ptr) T;
            }
        }
    }

    Array(T* start, T* end)
    {
        assign(start, end);
    }

    Array(Array&& other)
    {
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    Array(Array const& other) { *this = other; }

    Array& operator=(Array&& other)
    {
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        return *this;
    }

    Array& operator = (Array const& other)
    {
        assign((T*)other.begin(), (T*)other.end());
        return *this;
    }

    ~Array()
    {
        u32 count = 0;
        if constexpr (!std::is_trivial<T>::value)
        {
            for (auto& l : *this)
            {
                l.~T();
                count++;
            }
        }
        if (data_)
        {
            free(data_);
        }
    }

    void reserve(u32 newCapacity)
    {
        if (newCapacity > capacity_)
        {
            capacity_ = newCapacity;
            reallocate();
        }
    }

    void assign(T* start, T* end)
    {
        assert(start <= end);
        clear();
        size_ = end - start;
        reserve(size_);
        if constexpr (std::is_trivial<T>::value)
        {
            memcpy(data_, start, size_ * sizeof(T));
        }
        else
        {
            for (u32 i=0; i<size_; ++i)
            {
                new (data_+i) T(start[i]);
            }
        }
    }

    // copy push
    void push(const T& val)
    {
        ensureCapacity();
        new (data_ + size_) T(val);
        ++size_;
    }

    // move push
    void push(T&& val)
    {
        ensureCapacity();
        new (data_ + size_) T(move(val));
        ++size_;
    }

    // copy insert by index
    void insert(u32 index, const T& val)
    {
        assert(index <= size_);
        ensureCapacity();
        auto ptr = data_+index;
        if constexpr (std::is_trivial<T>::value)
        {
            memmove(ptr+1, ptr, (size_ - index) * sizeof(T));
        }
        else
        {
            for (T* movePtr = end(); movePtr != ptr; --movePtr)
            {
                new (movePtr) T(move(*(movePtr - 1)));
            }
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
        ensureCapacity();
        auto ptr = data_+index;
        if constexpr (std::is_trivial<T>::value)
        {
            memmove(ptr+1, ptr, (size_ - index) * sizeof(T));
        }
        else
        {
            for (T* movePtr = end(); movePtr != ptr; --movePtr)
            {
                new (movePtr) T(move(*(movePtr - 1)));
            }
        }
        ++size_;
        new (ptr) T(move(val));
    }

    // move insert
    void insert(T * element, T&& val)
    {
        insert(data_ - element, move(val));
    }

    T* erase(T* element)
    {
        assert(element < data_+size_);
        element->~T();
        if constexpr (std::is_trivial<T>::value)
        {
            memmove(element, element+1, ((data_+size_) - element) * sizeof(T));
        }
        else
        {
            for (u32 i=element - data_; i<size_-1; ++i)
            {
                new (data_ + i) T(move(data_[i+1]));
            }
        }
        --size_;
        return element;
    }

    T* erase(T* startPtr, T* endPtr)
    {
        assert(startPtr < endPtr);
        assert(size_ > 0);
        assert(startPtr >= begin());
        assert(endPtr <= end());
        for (auto it = startPtr; it != endPtr; ++it)
        {
            it->~T();
        }
        u32 elementsRemoved = endPtr - startPtr;
        if constexpr (std::is_trivial<T>::value)
        {
            if (size_ > 0 && endPtr != end())
            {
                memmove(startPtr, endPtr, (end() - endPtr) * sizeof(T));
            }
        }
        else
        {
            if (size_ > 0 && endPtr != end())
            {
                for (u32 i=startPtr - data_; i<size_-1; ++i)
                {
                    new (startPtr + i) T(move(data_[elementsRemoved + i]));
                }
            }
        }
        size_ -= elementsRemoved;
        return startPtr;
    }

    void erase(u32 index)
    {
        erase(data_ + index);
    }

    void pop_back()
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

    void shrink_to_fit()
    {
        if (capacity_ > size_)
        {
            capacity_ = size_;
            if (capacity_ > 0)
            {
                reallocate();
            }
            else
            {
                free(data_);
                data_ = nullptr;
            }
        }
    }

    void resize(u32 newSize)
    {
        u32 tmp = size_;

        // call destructors if shrinking
        if (newSize < size_)
        {
            for (T* ptr = data_+newSize; ptr != data_ + size_; ++ptr)
            {
                ptr->~T();
            }
        }

        size_ = newSize;
        if (size_ > capacity_)
        {
            capacity_ = size_;
            reallocate();
        }

        // initialize any new elements
        if (size_ > tmp)
        {
            for (T* ptr = data_+tmp; ptr != data_ + size_; ++ptr)
            {
                new (ptr) T;
            }
        }
    }

    T& front()
    {
        assert(size_ > 0);
        return *data_;
    }

    T& back()
    {
        assert(size_ > 0);
        return *(data_ + (size_ - 1));
    }

    T const& front() const
    {
        assert(size_ > 0);
        return *data_;
    }

    T const& back() const
    {
        assert(size_ > 0);
        return *(data_ + (size_ - 1));
    }

    T& operator[](u32 index)
    {
        assert(index < size_);
        return data_[index];
    }

    T const& operator[](u32 index) const
    {
        assert(index < size_);
        return data_[index];
    }

    T* begin() { return data_; }
    T* end()   { return data_ + size_; }
    T const* begin() const { return data_; }
    T const* end()   const { return data_ + size_; }

    bool empty() const { return size_ == 0; }
    T* data() const { return data_; }
    u32 size() const { return size_; }
    u32 capacity() const { return capacity_; }

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
        quickSort(0, (i32)size_ - 1, compare);
    }

    void stableSort()
    {
        stableSort([](T const& a, T const& b) { return a < b; });
    }

    template <typename COMPARE>
    void stableSort(COMPARE const& compare)
    {
        for (u32 i=0; i<size_; ++i)
        {
            u32 j = i;
            while (j > 0 && compare(data_[j], data_[j-1]))
            {
                swap(data_[j-1], data_[j]);
                j--;
            }
        }
    }

    void reverse()
    {
        for (u32 low=0, high = size_-1; low < high; low++, high--)
        {
            swap(data_[low], data_[high]);
        }
    }
};
