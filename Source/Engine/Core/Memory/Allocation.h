// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory.h"
#include "Engine/Core/Core.h"

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
#if ENABLE_ASSERTION_LOW_LAYERS
            ASSERT(capacity <= Capacity);
#endif
        }

        FORCE_INLINE void Relocate(const int32 capacity, int32 oldCount, int32 newCount)
        {
#if ENABLE_ASSERTION_LOW_LAYERS
            ASSERT(capacity <= Capacity);
#endif
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
/// The memory allocation policy that uses default heap allocator.
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
            {
                capacity = 8;
            }
            else
            {
                // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
                capacity--;
                capacity |= capacity >> 1;
                capacity |= capacity >> 2;
                capacity |= capacity >> 4;
                capacity |= capacity >> 8;
                capacity |= capacity >> 16;
                uint64 capacity64 = (uint64)(capacity + 1) * 2;
                if (capacity64 > MAX_int32)
                    capacity64 = MAX_int32;
                capacity = (int32)capacity64;
            }
            return capacity;
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
#if  ENABLE_ASSERTION_LOW_LAYERS
            ASSERT(!_data);
#endif
            _data = static_cast<T*>(Allocator::Allocate(capacity * sizeof(T)));
#if !BUILD_RELEASE
            if (!_data)
                OUT_OF_MEMORY;
#endif
        }

        FORCE_INLINE void Relocate(const int32 capacity, int32 oldCount, int32 newCount)
        {
            T* newData = capacity != 0 ? static_cast<T*>(Allocator::Allocate(capacity * sizeof(T))) : nullptr;
#if !BUILD_RELEASE
            if (!newData && capacity != 0)
                OUT_OF_MEMORY;
#endif

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
template<int Capacity, typename OtherAllocator = HeapAllocation>
class InlinedAllocation
{
public:
    enum { HasSwap = false };

    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        typedef typename OtherAllocator::template Data<T> OtherData;

        bool _useOther = false;
        byte _data[Capacity * sizeof(T)];
        OtherData _other;

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE ~Data()
        {
        }

        FORCE_INLINE T* Get()
        {
            return _useOther ? _other.Get() : reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return _useOther ? _other.Get() : reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
        {
            return minCapacity <= Capacity ? Capacity : _other.CalculateCapacityGrow(capacity, minCapacity);
        }

        FORCE_INLINE void Allocate(int32 capacity)
        {
            if (capacity > Capacity)
            {
                _useOther = true;
                _other.Allocate(capacity);
            }
        }

        FORCE_INLINE void Relocate(int32 capacity, int32 oldCount, int32 newCount)
        {
            T* data = reinterpret_cast<T*>(_data);

            // Check if the new allocation will fit into inlined storage
            if (capacity <= Capacity)
            {
                if (_useOther)
                {
                    // Move the items from other allocation to the inlined storage
                    Memory::MoveItems(data, _other.Get(), newCount);

                    // Free the other allocation
                    Memory::DestructItems(_other.Get(), oldCount);
                    _other.Free();
                    _useOther = false;
                }
            }
            else
            {
                if (_useOther)
                {
                    // Resize other allocation
                    _other.Relocate(capacity, oldCount, newCount);
                }
                else
                {
                    // Allocate other allocation
                    _other.Allocate(capacity);
                    _useOther = true;

                    // Move the items from the inlined storage to the other allocation
                    Memory::MoveItems(_other.Get(), data, newCount);
                    Memory::DestructItems(data, oldCount);
                }
            }
        }

        FORCE_INLINE void Free()
        {
            if (_useOther)
            {
                _useOther = false;
                _other.Free();
            }
        }

        void Swap(Data& other)
        {
            // Not supported
        }
    };
};

using DefaultAllocation = HeapAllocation;
