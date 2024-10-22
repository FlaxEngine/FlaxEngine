// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory/Allocation.h"

/// <summary>
/// Uniquely owning pointer that manages the lifetime of an object.
/// </summary>
/// <remarks>
/// This class is similar to std::unique_ptr but tailored for the engine's memory management system.
/// Short name is used to make it easier to type and read.
/// </remarks>
template<typename T, typename Alloc = HeapAllocation>
class UPtr
{
    static_assert(Alloc::HasSwap, "UPtr allocator must support swapping.");

private:
    using AllocationData = typename Alloc::template Data<T>;

    AllocationData _allocation;


public:
    FORCE_INLINE void Reset()
    {
        _allocation.Free();

        // If the allocation was correctly freed, the pointer should be null.
        // If that's not true, it means that this allocator should not be used with UPtr.
        ASSERT(IsEmpty());
    }

    ~UPtr()
    {
        Reset();
    }

    FORCE_INLINE void Swap(UPtr& other)
    {
        _allocation.Swap(other._allocation);
    }


    /// <summary>
    /// Initializes empty UPtr.
    /// </summary>
    FORCE_INLINE UPtr() = default;


    /// <summary>
    /// This class is not copyable.
    /// </summary>
    UPtr(const UPtr&) = delete;

    /// <summary>
    /// This class is not copyable.
    /// </summary>
    UPtr& operator=(const UPtr&) = delete;


    /// <summary>
    /// Initializes UPtr by moving the pointer from another UPtr.
    /// </summary>
    FORCE_INLINE UPtr(UPtr&& other) noexcept
        : _allocation()
    {
        ASSERT(Alloc::HasContext == false);
        ::Swap(other);
    }

    template<typename TAllocContext>
    FORCE_INLINE UPtr(UPtr&& other, TAllocContext&& context)
        : _allocation(Forward<TAllocContext>(context))
    {
        ASSERT(Alloc::HasContext);
        ::Swap(other);
    }

    /// <summary>
    /// Reassigns UPtr by moving the pointer from another UPtr.
    /// </summary>
    FORCE_INLINE UPtr& operator=(UPtr&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            ::Swap(other);
        }
        return *this;
    }

    /// <summary>
    /// Initializes UPtr by passing constructor arguments and emplacement.
    /// </summary>
    template<typename... TInitArgs>
    FORCE_INLINE explicit UPtr(TInitArgs&&... initArgs)
        : _allocation()
    {
        static_assert(Alloc::HasContext == false, "This constructor is not available for allocators with context.");
        _allocation.Allocate(1);
        T* target = _allocation.Get();
        new (target) T(Forward<TInitArgs>(initArgs)...);
    }

    /// <summary>
    /// Initializes UPtr by passing constructor arguments and emplacement into the allocator using the provided context.
    /// </summary>
    template<typename... TInitArgs, typename TAllocContext>
    FORCE_INLINE explicit UPtr(TInitArgs&&... initArgs, TAllocContext&& context)
        : _allocation(Forward<TAllocContext>(context))
    {
        static_assert(Alloc::HasContext, "This constructor is only available for allocators with context.");
        _allocation.Allocate(1);
        T* target = _allocation.Get();
        new (target) T(Forward<TInitArgs>(initArgs)...);
    }

    FORCE_INLINE bool IsEmpty() const
    {
        return _allocation.Get() == nullptr;
    }

    FORCE_INLINE T* Get()
    {
        return _allocation.Get();
    }

    FORCE_INLINE const T* Get() const
    {
        return _allocation.Get();
    }

    FORCE_INLINE T* operator->()
    {
        ASSERT(!IsEmpty());
        return Get();
    }

    FORCE_INLINE const T* operator->() const
    {
        ASSERT(!IsEmpty());
        return Get();
    }

    FORCE_INLINE T& operator*()
    {
        ASSERT(!IsEmpty());
        return *Get();
    }

    FORCE_INLINE const T& operator*() const
    {
        ASSERT(!IsEmpty());
        return *Get();
    }
};
