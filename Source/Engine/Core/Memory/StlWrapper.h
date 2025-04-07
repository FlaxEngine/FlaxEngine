// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"

namespace std_flax
{
    /// <summary>
    /// Implementation of STL memory allocator that uses Flax default Allocator.
    /// <summary>
    template<class T>
    class allocator
    {
    public:

#if PLATFORM_64BITS
        typedef unsigned long long size_type;
        typedef long long difference_type;
#else
        typedef unsigned int size_type;
        typedef int difference_type;
#endif
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T value_type;

        allocator()
        {
        }

        allocator(const allocator&)
        {
        }

        pointer allocate(size_type n, const void* = 0)
        {
            return (pointer)Allocator::Allocate(n * sizeof(T));
        }

        void deallocate(void* p, size_type)
        {
            Allocator::Free(p);
        }

        pointer address(reference x) const
        {
            return &x;
        }

        const_pointer address(const_reference x) const
        {
            return &x;
        }

        allocator<T>& operator=(const allocator&)
        {
            return *this;
        }

        void construct(pointer p, const T& val)
        {
            new((T*)p) T(val);
        }

        void destroy(pointer p)
        {
            p->~T();
        }

        size_type max_size() const
        {
            return size_type(-1);
        }

        template<class U>
        struct rebind
        {
            typedef allocator<U> other;
        };

        template<class U>
        allocator(const allocator<U>&)
        {
        }

        template<class U>
        allocator& operator=(const allocator<U>&)
        {
            return *this;
        }
    };
}
