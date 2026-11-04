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

// Minimal implementation to replace std::allocator_traits
namespace std_flax
{
    struct true_type { static constexpr bool value = true; };
    struct false_type { static constexpr bool value = false; };

    template <typename A, typename = void>
    struct has_max_size : false_type {};

    template <typename... Args>
    using void_t = void;

    template <typename T> T&& declval() noexcept;

    template<typename A>
    struct has_max_size<A, void_t<decltype(declval<const A&>().max_size())>>: true_type {};

    template<typename A, typename = void>
    struct propagate_move
    {
        using type = false_type;
    };

    template<typename A>
    struct propagate_move<A, void_t<typename A::propagate_on_container_move_assignment>>
    {
        using type = typename A::propagate_on_container_move_assignment;
    };

    template<typename A>
    struct allocator_traits
    {
        using value_type = typename A::value_type;
        using propagate_on_container_move_assignment = typename propagate_move<A>::type;

    private:
        static size_t max_size_impl(const A& a, true_type)
        {
            return a.max_size();
        }

        static size_t max_size_impl(const A&, false_type)
        {
            return size_t(-1);
        }

    public:
        static size_t max_size(const A& a)
        {
            return max_size_impl(a, has_max_size<A>{});
        }
    };
}

