/* -*- c-basic-offset: 4 -*- */

/*
  vector of type-T elements that limits the number of elements to N

  major std::vector methods are supported as well as range-based for,
  iterators, and reverse iterators
  
  precond: T must be trivially copyable, N > 0

  author: Michael Buro
*/

#include <cassert>
#include <type_traits>

template <typename T, int N>
class BoundedVector
{
    static_assert(N > 0, "N=0");
    static_assert(is_trivially_copyable<T>::value, "not trivial");

    T data[N];
    int n; // number of active elements

    // optimize arg/ret types when T is small (avoiding references)
    static constexpr bool small_type = sizeof(T) <= 8 && std::is_trivially_copyable<T>::value;
    using ArgTypeT      = typename std::conditional<small_type, const T, const T&>::type;
    using ConstRetTypeT = typename std::conditional<small_type, const T, const T &>::type;
    
public:

    BoundedVector()
        : n(0)
    { }

    BoundedVector(const BoundedVector &x)
    {
        n = x.n;

        // T trivially copyable => memcpy
        for (int i=0; i < n; ++i) {
            new (&data[i]) T(x.data[i]);
        }
    }

    BoundedVector &operator=(const BoundedVector &x)
    {
        if (this != &x) {
            this->~BoundedVector(); // optimized out if T is trivially copyable
            // note: if T is not trivially copyable this could be optimized
            // by only calling the destructor on the first x.n elements
            new (this) BoundedVector(x); // in-place CC
        }
        return *this;
    }
  
    T &operator[](int i)
    {
        assert(0 <= i && i < N);
        return data[i];
    }

    ConstRetTypeT operator[](int i) const
    {
        assert(0 <= i && i < N);
        return data[i];
    }

    BoundedVector &push_back(ArgTypeT x)
    {
        assert(n < N);
        data[n++] = x;
        return *this;
    }

    BoundedVector &pop_back()
    {
        assert(n > 0);
        --n;
        return *this;
    }

    T &back()
    {
        assert(n > 0);
        return data[n-1];
    }

    ConstRetTypeT back() const 
    {
        assert(n > 0);
        return data[n-1];
    }

    int size() const { return n; }

    // [r]begin/[r]end interface

    using iterator = T *;
    using const_iterator = const T *;

    iterator begin() { return data; }
    iterator end() { return data + n; }
    const_iterator begin() const { return data; }
    const_iterator end() const { return data + n; }
    auto rbegin() { return std::reverse_iterator<iterator>(end()); }
    auto rend() { return std::reverse_iterator<iterator>(begin()); }
    auto rbegin() const { return std::reverse_iterator<const_iterator>(end()); }
    auto rend() const { return std::reverse_iterator<const_iterator>(begin()); }
};
