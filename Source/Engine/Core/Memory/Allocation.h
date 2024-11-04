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
        constexpr static int32 MinCapacity = Capacity, MaxCapacity = Capacity;


        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data() = default;


        /// <summary> Checks if the allocation can be moved. </summary>
        FORCE_INLINE bool CanMove() const
        {
            return false; // Fixed allocation cannot be moved at all.
        }

        /// <summary> No allocation can be copied. </summary>
        auto operator=(const Data&) -> Data& = delete;

        /// <summary> Fixed allocation must not be moved. *It's fixed, after all.* </summary>
        auto operator=(Data&&) -> Data& = delete;


        /// <summary> No allocation can be copied. </summary>
        Data(const Data&) = delete;

        /// <summary> Fixed allocation must not be moved. *It's fixed, after all.* </summary>
        Data(Data&&) = delete;


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
        constexpr static int32 MinCapacity = 0, MaxCapacity = INT32_MAX;

        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data()
        {
            if (_data)
              Allocator::Free(_data);
        }


        /// <summary> Checks if the allocation can be moved. </summary>
        FORCE_INLINE bool CanMove() const
        {
            return true;
        }

        /// <summary> Initializes allocation by moving data from another allocation. </summary>
        FORCE_INLINE Data(Data&& other) noexcept
        {
            ::Swap(this->_data, other._data);
            // Other allocation is now empty.
        }

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


        /// <summary> No allocation can be copied. </summary>
        Data(const Data&) = delete;

        /// <summary> No allocation can be copied. </summary>
        auto operator=(const Data&) -> Data & = delete;


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
template<int Capacity, typename FallbackAllocator = HeapAllocation>
class InlinedAllocation
{
public:
    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        using FallbackData = typename FallbackAllocator::template Data<T>;
        
        byte _inlinedData[Capacity * sizeof(T)];
        FallbackData _fallbackData;
        bool _useFallback = false; 

    public:
        constexpr static int32 MinCapacity = Capacity, MaxCapacity = INT32_MAX;


        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data() = default;


        /// <summary> Checks if the allocation can be moved. </summary>
        FORCE_INLINE bool CanMove() const
        {
            // If the inlined allocation is used, it can be moved.
            return _useFallback ? _fallbackData.CanMove() : false;
        }


        /// <summary> No allocation can be copied. </summary>
        Data(const Data&) = delete;

        /// <summary> No allocation can be copied. </summary>
        auto operator=(const Data&)->Data & = delete;

        /// <summary>
        /// Inlined allocation move is conditional.
        /// Attempts to move when not possible will result in crash.
        /// </summary>
        Data(Data&& other) noexcept
        {
            // Do not initialize the inlined data.

            ASSERT_LOW_LAYER(other.CanMove());

            _useFallback = true; // We know that only the backup the other allocation can be moved.
            _fallbackData = ::MoveTemp<FallbackData>(other._fallbackData);
        }

        /// <summary>
        /// Inlined allocation move is conditional
        /// Attempts to move when not possible will result in crash.
        /// </summary>
        auto operator=(Data&& other) noexcept -> Data&
        {
            if (this == &other) 
            {
                return *this;
            }

            // Do not initialize the inlined data.

            ASSERT_LOW_LAYER(other.CanMove());

            _useFallback = true; // We know that only the backup the other allocation can be moved.
            _fallbackData = ::MoveTemp<FallbackData>(other._fallbackData);

            return *this;
        }


        FORCE_INLINE T* Get()
        {
            return _useFallback ? _fallbackData.Get() : reinterpret_cast<T*>(_inlinedData);
        }

        FORCE_INLINE const T* Get() const
        {
            return _useFallback ? _fallbackData.Get() : reinterpret_cast<const T*>(_inlinedData);
        }

        FORCE_INLINE int32 Allocate(int32 capacity)
        {
            if (capacity > Capacity)
            {
                _useFallback = true;
                return _fallbackData.Allocate(capacity);
            }

            return Capacity;
        }

        FORCE_INLINE void Free()
        {
            if (_useFallback)
            {
                _useFallback = false;
                _fallbackData.Free();
            }
        }
    };
};

/// <summary>
/// Utility class used to manage memory allocations, with objects occupying the memory one by one.
/// </summary>
/// <param name="T"> The type of the elements stored in the allocation. </param>
template<typename T>
class AllocationOperations
{
public:
    template<typename Allocation>
    constexpr static bool IsMoveConstructible = TIsMoveConstructible<typename Allocation::Data>::Value;

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
        typename TEnableIf<IsMoveConstructible<Allocation>, int>::Type = 0 // Move constructible
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
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements [0, count) are valid, meaning that they are constructed and can be moved. </param>
    /// <param name="capacity"> The capacity of the destination allocation. If a new allocation is not required, this parameter will be ignored. </param>
    template<
        typename Allocation,
        typename TEnableIf<!IsMoveConstructible<Allocation>, int>::Type = 0 // NOT move constructible
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


    template<
        typename Allocation,
        typename TEnableIf<IsMoveConstructible<Allocation>, int>::Type = 0 // Move constructible
    >
    FORCE_INLINE static int32 Relocate(
        typename Allocation::template Data<T>& allocation,
        const int32 desiredCapacity,
        const int32 currentCount
    )
    {
        ASSERT(Math::IsInRange(desiredCapacity, Allocation::MinCapacity, Allocation::MaxCapacity)); // Ensure that the desired capacity is within the bounds of the allocation policy.

        // Invoking this method means that an allocation must happen!
        // Collection should take responsibility of determining that a relocation is actually required.
        // It must also ensure that the new capacity obeys the rules of the allocation policy.

        typename Allocation::template Data<T> newAllocation;

        const int32 newCapacity = newAllocation.Allocate(desiredCapacity); // Allocate new memory.
        const int32 newCount = Math::Min(currentCount, newCapacity); // Determine how many elements can be moved, how many fit in the new allocation.

        Memory::MoveItems(newAllocation.Get(), allocation.Get(), newCount); // Move fitting elements.
        Memory::DestructItems(allocation.Get(), currentCount); // Destruct all elements in the old allocation. (Both moved and not fitting)

        allocation.Free(); // Free the old allocation...
        allocation = ::MoveTemp(newAllocation); // ...and replace it with the new one.

        return newCapacity; // Return the new capacity, so the collection can update its internal state.
    }

    template<
        typename Allocation,
        typename TEnableIf<!IsMoveConstructible<Allocation>, int>::Type = 0 // NOT move constructible
    >
    FORCE_INLINE static int32 Relocate(
        typename Allocation::template Data<T>& allocation,
        const int32 desiredCapacity,
        const int32 currentCount
    )
    {
        ASSERT(Math::IsInRange(desiredCapacity, Allocation::MinCapacity, Allocation::MaxCapacity)); // Ensure that the desired capacity is within the bounds of the allocation policy.

        // Relocating elements is not possible for non-move constructible types.
        // Thus the only way to change the capacity is to destroy not fitting elements and cap the capacity.

        const int32 newCount = Math::Min(currentCount, desiredCapacity); // Determine how many elements can stay, how many fit in the hypothetical allocation.

        Memory::DestructItems(allocation.Get() + newCount, currentCount - newCount); // Destruct all elements that do not fit in the new allocation.

        return desiredCapacity; // Return the new capacity, so the collection can update its internal state.
    }
};

using DefaultAllocation = HeapAllocation;
