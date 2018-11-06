#pragma once
#include <iostream>
#include <vector>
#include <cassert>

using namespace std;

constexpr int VectorLimit = 40;

template <typename T, size_t VectorLimit>
class StaticVector
{
    static_assert(VectorLimit > 0, "VectorLimit=0");

    T data[VectorLimit];
    unsigned long n;

public:

    StaticVector() 
    {
        assert(is_trivially_copyable<T>::value);
        n = 0;
    }

    T &operator[](size_t i) 
    {
        assert(i < VectorLimit);
        return data[i];
    }

    const T &operator[](size_t i) const 
    {
        assert(i < VectorLimit);
        return data[i];
    }

    void push_back(const T &x) 
    {
        assert(n < VectorLimit);
        data[n++] = x;
    }

    void pop_back() 
    {
        assert(n > 0);
        --n;
    }

    T &back() 
    {
        assert(n > 0);
        return data[n - 1];
    }

    const T &back() const 
    {
        assert(n > 0);
        return data[n - 1];
    }

    size_t size() const 
    { 
        return n; 
    }

    bool empty() const 
    { 
        return n == 0; 
    }

    const T &front() const 
    {
        assert(n > 0);
        return data[0];
    }

    T &front() 
    {
        assert(n > 0);
        return data[0];
    }
};