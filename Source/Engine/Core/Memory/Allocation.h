// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory.h"
#include "MemoryUtils.h"
#include "Engine/Core/Core.h"

/// <summary>
/// The memory allocation policy that uses inlined memory of the fixed size (no resize support, does not use heap allocations at all).
/// </summary>
template<int Capacity>
class FixedAllocation
{
public:
    enum { HasSwap = false }; //TODO(mtszkarbowiak) Replace with move semantics
    enum { HasContext = false }; //TODO(mtszkarbowiak) Replace with SFINAE

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

        Data(const Data&) = delete;
        Data(Data&&) = delete;

        auto operator=(const Data&) -> Data& = delete;
        auto operator=(Data&&) -> Data& = delete;

        FORCE_INLINE T* Get()
        {
            return reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE int32 CalculateCapacityGrow(const int32 capacity, const int32 minCapacity) const
        {
            ASSERT(minCapacity <= Capacity);
            return Capacity;
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }

        FORCE_INLINE void Relocate(const int32 capacity, const int32 oldCount, const int32 newCount)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }

        FORCE_INLINE void Free()
        {
        }

        void Swap(Data& other)
        {
            CRASH
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
    enum { HasSwap = true }; //TODO(mtszkarbowiak) Replace with move semantics
    enum { HasContext = false }; //TODO(mtszkarbowiak) Replace with SFINAE

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

        Data(const Data&) = delete;
        Data(Data&&) = delete;

        auto operator=(const Data&)->Data & = delete;
        auto operator=(Data&&)->Data & = delete;

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
            {
                uint64 capacity64 = static_cast<uint64>(MemoryUtils::NextPow2(capacity)) * 2;

                if (capacity64 > MAX_int32)
                    capacity64 = MAX_int32;

                capacity = static_cast<int32>(capacity64);
            }

            return capacity;
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(!_data);
            _data = static_cast<T*>(Allocator::Allocate(capacity * sizeof(T)));

#if !BUILD_RELEASE
            if (!_data)
                OUT_OF_MEMORY;
#endif
        }

        FORCE_INLINE void Relocate(const int32 capacity, const int32 oldCount, const int32 newCount)
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
    enum { HasSwap = false }; //TODO(mtszkarbowiak) Replace with move semantics
    enum { HasContext = false }; //TODO(mtszkarbowiak) Replace with SFINAE

    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        using OtherData = typename OtherAllocator::template Data<T>;

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

        Data(const Data&) = delete;
        Data(Data&&) = delete;

        auto operator=(const Data&)->Data & = delete;
        auto operator=(Data&&)->Data & = delete;

        FORCE_INLINE T* Get()
        {
            return _useOther ? _other.Get() : reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return _useOther ? _other.Get() : reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE int32 CalculateCapacityGrow(const int32 capacity, const int32 minCapacity) const
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

        FORCE_INLINE void Relocate(const int32 capacity, const int32 oldCount, const int32 newCount)
        {
            // Check if the new allocation will fit into inlined storage
            if (capacity <= Capacity)
            {
                if (_useOther)
                {
                    // Move the items from other allocation to the inlined storage
                    Memory::MoveItems(reinterpret_cast<T*>(_data), _other.Get(), newCount);

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
                    Memory::MoveItems(_other.Get(), reinterpret_cast<T*>(_data), newCount);
                    Memory::DestructItems(reinterpret_cast<T*>(_data), oldCount);
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
            CRASH;
            // Not supported
        }
    };
};

typedef HeapAllocation DefaultAllocation; //TODO(mtszkarbowiak) Use use :D
