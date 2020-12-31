#pragma once

#include "array.h"
#include "str.h"

inline u32 mapHash(const char* str)
{
    u32 hash = 5381;
    u32 c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

template <u32 SIZE>
inline u32 mapHash(Str<SIZE> str)
{
    return mapHash(str.data());
}

inline u32 mapHash(void* ptr)
{
    return *((u32*)&ptr);
}

inline u32 mapHash(u32 val)
{
    return val;
}

inline u32 mapHash(i32 val)
{
    return *((u32*)&val);
}

inline u32 mapHash(u64 val)
{
    return *((u32*)&val);
}

inline u32 mapHash(i64 val)
{
    return *((u32*)&val);
}

template <typename T>
inline bool mapCompare(T const& lhs, T const& rhs)
{
    return lhs == rhs;
}

template <>
inline bool mapCompare(const char* const& lhs, const char* const& rhs)
{
    return strcmp(lhs, rhs) == 0;
}

template <typename KEY, typename VALUE, u32 SIZE=64>
class Map
{
public:
    struct Pair
    {
        KEY key;
        VALUE value;
    };

private:
    u32 size_ = 0;
    Array<Pair> elements_[SIZE];

    const Pair* get(u32 index, KEY key) const
    {
        for (auto& pair : elements_[index])
        {
            if (mapCompare(key, pair.key))
            {
                return &pair;
            }
        }
        return nullptr;
    }

    Pair* get(u32 index, KEY key)
    {
        for (auto& pair : elements_[index])
        {
            if (mapCompare(key, pair.key))
            {
                return &pair;
            }
        }
        return nullptr;
    }

public:
    bool empty() const { return size_ != 0; }
    u32 size() const { return size_; }
    void clear()
    {
        for (auto& bucket : elements_)
        {
            bucket.clear();
        }
        size_ = 0;
    }

    bool set(KEY const& key, VALUE const& value)
    {
        u32 index = mapHash(key) % SIZE;
        auto ptr = get(index, key);
        if (!ptr)
        {
            elements_[index].push({ key, value });
            ++size_;
            return false;
        }
        else
        {
            ptr->value = value;
            return true;
        }
    }

    bool set(KEY const& key, VALUE && value)
    {
        u32 index = mapHash(key) % SIZE;
        auto ptr = get(index, key);
        if (!ptr)
        {
            elements_[index].push({ key, std::move(value) });
            ++size_;
            return false;
        }
        else
        {
            ptr->value = std::move(value);
            return true;
        }
    }

    const VALUE* get(KEY const& key) const
    {
        u32 index = mapHash(key) % SIZE;
        auto ptr = get(index, key);
        return ptr ? &ptr->value : nullptr;
    }

    VALUE* get(KEY const& key)
    {
        u32 index = mapHash(key) % SIZE;
        auto ptr = get(index, key);
        return ptr ? &ptr->value : nullptr;
    }

    bool erase(KEY const& key)
    {
        u32 index = mapHash(key) % SIZE;
        auto n = get(index, key);
        if (n)
        {
            elements_[index].erase(n);
            --size_;
            return true;
        }
        return false;
    }

    VALUE& operator [] (KEY const& key)
    {
        VALUE* val = get(key);
        if (!val)
        {
            set(key, {});
            return *get(key);
        }
        return *val;
    }

    struct ConstIterator
    {
        const Pair* ptr;
        const Map<KEY, VALUE, SIZE>* container;
        u32 currentBucket;

        void operator ++ ()
        {
            ptr++;
            if (ptr == container->elements_[currentBucket].end())
            {
                for(;;)
                {
                    ++currentBucket;
                    if (currentBucket == SIZE)
                    {
                        ptr = nullptr;
                        return;
                    }
                    else if (container->elements_[currentBucket].size() != 0)
                    {
                        ptr = container->elements_[currentBucket].begin();
                        return;
                    }
                }
            }
        }

        void operator ++ (int)
        {
            operator ++ ();
        }

        Pair const& operator * () const
        {
            return *ptr;
        }

        Pair const* operator -> () const
        {
            return ptr;
        }

        bool operator == (ConstIterator const& other) const
        {
            return ptr == other.ptr;
        }

        bool operator != (ConstIterator const& other) const
        {
            return ptr != other.ptr;
        }
    };

    struct Iterator
    {
        Pair* ptr;
        Map<KEY, VALUE, SIZE>* container;
        u32 currentBucket;

        void operator ++ ()
        {
            ptr++;
            if (ptr == container->elements_[currentBucket].end())
            {
                for(;;)
                {
                    ++currentBucket;
                    if (currentBucket == SIZE)
                    {
                        ptr = nullptr;
                        return;
                    }
                    else if (container->elements_[currentBucket].size() != 0)
                    {
                        ptr = container->elements_[currentBucket].begin();
                        return;
                    }
                }
            }
        }

        void operator ++ (int)
        {
            operator ++ ();
        }

        Pair const& operator * () const
        {
            return *ptr;
        }

        Pair const* operator -> () const
        {
            return ptr;
        }

        Pair& operator * ()
        {
            return *ptr;
        }

        Pair* operator -> ()
        {
            return ptr;
        }

        bool operator == (Iterator const& other) const
        {
            return ptr == other.ptr;
        }

        bool operator != (Iterator const& other) const
        {
            return ptr != other.ptr;
        }
    };

    ConstIterator begin() const
    {
        for (auto& e : elements_)
        {
            if (e.size() > 0)
            {
                return { e.begin(), this, (u32)(&e - elements_) };
            }
        }
        return { nullptr };
    }

    ConstIterator end() const
    {
        return { nullptr };
    }

    Iterator begin()
    {
        for (auto& e : elements_)
        {
            if (e.size() > 0)
            {
                return { e.begin(), this, (u32)(&e - elements_) };
            }
        }
        return { nullptr };
    }

    Iterator end()
    {
        return { nullptr };
    }
};
