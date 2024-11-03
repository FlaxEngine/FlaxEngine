// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory.h"
#include "Engine/Core/Core.h"
#include "Engine/Core/Math/Math.h"

/// <summary>
/// The memory allocation policy that uses inlined memory of the fixed size (no resize support, does not use heap allocations at all).
/// </summary>
template<int Capacity>
class FixedAllocation
{
public:
    template<typename T> //TODO Erase type
    class alignas(sizeof(void*)) Data
    {
    private:
        byte _data[Capacity * sizeof(T)];

    public:
        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data() = default;


        /// <summary> No allocation can be copied. </summary>
        Data(const Data&) = delete;

        /// <summary> Fixed allocation must not be moved. *It's fixed, after all.* </summary>
        Data(Data&&) = delete;


        /// <summary> No allocation can be copied. </summary>
        auto operator=(const Data&)->Data & = delete;

        /// <summary> Fixed allocation must not be moved. *It's fixed, after all.* </summary>
        auto operator=(Data&&)->Data & = delete;



        FORCE_INLINE T* Get()
        {
            return reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }

        FORCE_INLINE void Free()
        {
        }
    };
};

/// <summary>
/// The memory allocation policy that uses default heap allocator.
/// </summary>
class HeapAllocation
{
public:
    template<typename T>
    class Data
    {
    private:
        T* _data = nullptr;

    public:
        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data()
        {
            if (_data)
              Allocator::Free(_data);
        }

        /// <summary> No allocation can be copied. </summary>
        Data(const Data&) = delete;

        /// <summary> Initializes allocation by moving data from another allocation. </summary>
        FORCE_INLINE Data(Data&& other) noexcept
        {
            ::Swap(this->_data, other._data);
            // Other allocation is now empty.
        }

        /// <summary> No allocation can be copied. </summary>
        auto operator=(const Data&)->Data & = delete;

        /// <summary> Reassigns allocation by moving data from another allocation. </summary>
        FORCE_INLINE auto operator=(Data&& other) noexcept -> Data&
        {
            if (this != &other)
            {
                ::Swap(this->_data, other._data);
                // Other allocation now has the pointer (or nullptr) to be freed in the destructor.
            }
            return *this;
        }

        FORCE_INLINE T* Get()
        {
            return _data;
        }

        FORCE_INLINE const T* Get() const
        {
            return _data;
        }

        FORCE_INLINE int32 Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(!_data);

            const int32 effectiveCapacity = Math::RoundUpToPowerOf2(capacity);

            _data = static_cast<T*>(Allocator::Allocate(effectiveCapacity * sizeof(T)));

#if !BUILD_RELEASE
            if (!_data)
                OUT_OF_MEMORY;
#endif

            return effectiveCapacity;
        }

        FORCE_INLINE void Free()
        {
            Allocator::Free(_data);
            _data = nullptr;
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
    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        typedef typename OtherAllocator::template Data<T> OtherData;

        bool _useOther = false; //TODO Fix alignment
        byte _data[Capacity * sizeof(T)];
        OtherData _other;

    public:
        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data() = default;


        /// <summary> No allocation can be copied. </summary>
        Data(const Data&) = delete;

        /// <summary> Inlined allocation must not be moved. It uses *fixed* allocation. </summary>
        Data(Data&&) = delete;

        /// <summary> No allocation can be copied. </summary>
        auto operator=(const Data&)->Data & = delete;

        /// <summary> Inlined allocation must not be moved. It uses *fixed* allocation. </summary>
        auto operator=(Data&&)->Data & = delete;


        FORCE_INLINE T* Get()
        {
            return _useOther ? _other.Get() : reinterpret_cast<T*>(_data);
        }

        FORCE_INLINE const T* Get() const
        {
            return _useOther ? _other.Get() : reinterpret_cast<const T*>(_data);
        }

        FORCE_INLINE int32 Allocate(int32 capacity)
        {
            if (capacity > Capacity)
            {
                _useOther = true;
                return _other.Allocate(capacity);
            }

            return Capacity;
        }

        FORCE_INLINE void Free()
        {
            if (_useOther)
            {
                _useOther = false;
                _other.Free();
            }
        }
    };
};

template<typename T>
class AllocationOperations
{
public:
    /// <summary>
    /// This function transfers the data from the source allocation to the destination allocation.
    /// If possible, it will use allocation move constructor to avoid moving the individual elements.
    /// </summary>
    /// <param name="source"> The source allocation. </param>
    /// <param name="destination"> The destination allocation. It must be empty. </param>
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements from 0 to count-1 are valid, meaning that they are constructed and can be moved. </param>
    /// <param name="capacity"> The capacity of the destination allocation. If a new allocation is not required, this parameter will be ignored. </param>
    template<
        typename Allocation,
        typename TEnableIf<TIsMoveConstructible<typename Allocation::Data>::Value, int>::Type = 0
    >
    FORCE_INLINE static void MoveAllocated(
        typename Allocation::template Data<T>& source,
        typename Allocation::template Data<T>& destination,
        const int32 count,
        const int32 capacity
    )
    {
        ::Swap(source, destination);
    }

    /// <summary>
    /// This function transfers the data from the source allocation to the destination allocation.
    /// If possible, it will use allocation move constructor to avoid moving the individual elements.
    /// </summary>
    /// <param name="source"> The source allocation. </param>
    /// <param name="destination"> The destination allocation. It must be empty. </param>
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements from 0 to count-1 are valid, meaning that they are constructed and can be moved. </param>
    /// <param name="capacity"> The capacity of the destination allocation. If a new allocation is not required, this parameter will be ignored. </param>
    template<
        typename Allocation,
        typename TEnableIf<!TIsMoveConstructible<typename Allocation::Data>::Value, int>::Type = 0
    >
    FORCE_INLINE static void MoveAllocated(
        typename Allocation::template Data<T>& source,
        typename Allocation::template Data<T>& destination,
        const int32 count,
        const int32 capacity
    )
    {
        destination.Allocate(capacity);
        Memory::MoveItems(destination.Get(), source.Get(), count);
        Memory::DestructItems(source.Get(), count);
        source.Free();
    }
};

using DefaultAllocation = HeapAllocation;
