#pragma once

#include "common.h"

template <typename T>
class OwnedPtr
{
    T* ptr_ = nullptr;

public:
    OwnedPtr() {}
    OwnedPtr(T* ptr) : ptr_(ptr) {}
    OwnedPtr(OwnedPtr const& other) = delete;
    OwnedPtr(OwnedPtr&& other) { *this = move(other); }
    ~OwnedPtr() { if (ptr_) delete ptr_; }

    void reset(T* ptr=nullptr)
    {
        if (ptr_) { delete ptr_; }
        ptr_ = ptr;
    }

    OwnedPtr& operator = (OwnedPtr&& other)
    {
        reset(other.ptr_);
        other.ptr_ = nullptr;
        return *this;
    }

    T* get() const { return ptr_; }
    OwnedPtr& operator = (OwnedPtr const& other) = delete;
    operator bool() const { return !!ptr_; }
    T& operator * () const { return *ptr_; }
    T* operator -> () const { return ptr_; }
    T& operator [] (unsigned long long index) const { return ptr_[index]; }
    bool operator == (OwnedPtr const& other) { return ptr_ == other.ptr_; }
    bool operator == (const T* ptr) { return ptr_ == ptr; }
};

template <typename T>
class OwnedPtr<T[]>
{
    T* ptr_ = nullptr;

public:
    OwnedPtr() {}
    OwnedPtr(T* ptr) : ptr_(ptr) {}
    OwnedPtr(OwnedPtr const& other) = delete;
    OwnedPtr(OwnedPtr&& other) { *this = move(other); }
    ~OwnedPtr() { if (ptr_) delete[] ptr_; }

    void reset(T* ptr=nullptr)
    {
        if (ptr_) { delete[] ptr_; }
        ptr_ = ptr;
    }

    OwnedPtr& operator = (OwnedPtr&& other)
    {
        reset(other.ptr_);
        other.ptr_ = nullptr;
        return *this;
    }

    T* get() const { return ptr_; }
    OwnedPtr& operator = (OwnedPtr const& other) = delete;
    operator bool() const { return !!ptr_; }
    T& operator * () const { return *ptr_; }
    T* operator -> () const { return ptr_; }
    T& operator [] (unsigned long long index) const { return ptr_[index]; }
};
