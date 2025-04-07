// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Memory.h"
#include "Engine/Core/Core.h"

namespace AllocationUtils
{
    // Rounds up the input value to the next power of 2 to be used as bigger memory allocation block. Handles overflow.
    inline int32 RoundUpToPowerOf2(int32 capacity)
    {
        // Reference: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        capacity--;
        capacity |= capacity >> 1;
        capacity |= capacity >> 2;
        capacity |= capacity >> 4;
        capacity |= capacity >> 8;
        capacity |= capacity >> 16;
        uint64 capacity64 = (uint64)(capacity + 1) * 2;
        if (capacity64 > MAX_int32)
            capacity64 = MAX_int32;
        return (int32)capacity64;
    }

    // Aligns the input value to the next power of 2 to be used as bigger memory allocation block.
    inline int32 AlignToPowerOf2(int32 capacity)
    {
        // Reference: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        capacity--;
        capacity |= capacity >> 1;
        capacity |= capacity >> 2;
        capacity |= capacity >> 4;
        capacity |= capacity >> 8;
        capacity |= capacity >> 16;
        capacity++;
        return capacity;
    }
}

/// <summary>
/// The memory allocation policy that uses inlined memory of the fixed size (no resize support, does not use heap allocations at all).
/// </summary>
template<int Capacity>
class FixedAllocation
{
public:
    enum { HasSwap = false };

    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        byte _data[Capacity * sizeof(T)];

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE ~Data()
        {
        }

        FORCE_INLINE T* Get()
        {
            return reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, const int32 minCapacity) const
        {
            ASSERT(minCapacity <= Capacity);
            return Capacity;
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }

        FORCE_INLINE void Relocate(const int32 capacity, int32 oldCount, int32 newCount)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }

        FORCE_INLINE void Free()
        {
        }

        void Swap(Data& other)
        {
            // Not supported
        }
    };
};

/// <summary>
/// The memory allocation policy that uses default heap allocation.
/// </summary>
class HeapAllocation
{
public:
    enum { HasSwap = true };

    template<typename T>
    class Data
    {
    private:
        T* _data = nullptr;

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE ~Data()
        {
            Allocator::Free(_data);
        }

        FORCE_INLINE T* Get()
        {
            return _data;
        }

        FORCE_INLINE const T* Get() const
        {
            return _data;
        }

        FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, const int32 minCapacity) const
        {
            if (capacity < minCapacity)
                capacity = minCapacity;
            if (capacity < 8)
                capacity = 8;
            else
                capacity = AllocationUtils::RoundUpToPowerOf2(capacity);
            return capacity;
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(!_data);
            _data = static_cast<T*>(Allocator::Allocate(capacity * sizeof(T)));
        }

        FORCE_INLINE void Relocate(const int32 capacity, int32 oldCount, int32 newCount)
        {
            T* newData = capacity != 0 ? static_cast<T*>(Allocator::Allocate(capacity * sizeof(T))) : nullptr;
            if (oldCount)
            {
                if (newCount > 0)
                    Memory::MoveItems(newData, _data, newCount);
                Memory::DestructItems(_data, oldCount);
            }
            Allocator::Free(_data);
            _data = newData;
        }

        FORCE_INLINE void Free()
        {
            Allocator::Free(_data);
            _data = nullptr;
        }

        FORCE_INLINE void Swap(Data& other)
        {
            ::Swap(_data, other._data);
        }
    };
};

/// <summary>
/// The memory allocation policy that uses inlined memory of the fixed size and supports using additional allocation to increase its capacity (eg. via heap allocation).
/// </summary>
template<int Capacity, typename FallbackAllocation = HeapAllocation>
class InlinedAllocation
{
public:
    enum { HasSwap = false };

    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        typedef typename FallbackAllocation::template Data<T> FallbackData;

        bool _useFallback = false;
        byte _data[Capacity * sizeof(T)];
        FallbackData _fallback;

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE ~Data()
        {
        }

        FORCE_INLINE T* Get()
        {
            return _useFallback ? _fallback.Get() : reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return _useFallback ? _fallback.Get() : reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
        {
            return minCapacity <= Capacity ? Capacity : _fallback.CalculateCapacityGrow(capacity, minCapacity);
        }

        FORCE_INLINE void Allocate(int32 capacity)
        {
            if (capacity > Capacity)
            {
                _useFallback = true;
                _fallback.Allocate(capacity);
            }
        }

        FORCE_INLINE void Relocate(int32 capacity, int32 oldCount, int32 newCount)
        {
            T* data = reinterpret_cast<T*>(_data);

            // Check if the new allocation will fit into inlined storage
            if (capacity <= Capacity)
            {
                if (_useFallback)
                {
                    // Move the items from other allocation to the inlined storage
                    Memory::MoveItems(data, _fallback.Get(), newCount);

                    // Free the other allocation
                    Memory::DestructItems(_fallback.Get(), oldCount);
                    _fallback.Free();
                    _useFallback = false;
                }
            }
            else
            {
                if (_useFallback)
                {
                    // Resize other allocation
                    _fallback.Relocate(capacity, oldCount, newCount);
                }
                else
                {
                    // Allocate other allocation
                    _fallback.Allocate(capacity);
                    _useFallback = true;

                    // Move the items from the inlined storage to the other allocation
                    Memory::MoveItems(_fallback.Get(), data, newCount);
                    Memory::DestructItems(data, oldCount);
                }
            }
        }

        FORCE_INLINE void Free()
        {
            if (_useFallback)
            {
                _useFallback = false;
                _fallback.Free();
            }
        }

        void Swap(Data& other)
        {
            // Not supported
        }
    };
};

using DefaultAllocation = HeapAllocation;
