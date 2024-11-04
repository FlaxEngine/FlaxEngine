// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory.h"
#include "Engine/Core/Core.h"
#include "Engine/Core/Math/Math.h"


/// <summary>
/// Utility class used to manage memory allocations, with objects occupying the memory one by one.
/// </summary>
/// <param name="T"> The type of the elements stored in the allocation. </param>
template<typename T>
class AllocationOperation
{
public:
    template<typename Allocation>
    constexpr static bool IsMoveConstructible = TIsMoveConstructible<typename Allocation::Data>::Value;

private:
    template<typename Allocation>
    FORCE_INLINE static int32 MoveAllocatedOneByOne(
        typename Allocation::template Data<T>& source,
        typename Allocation::template Data<T>& destination,
        const int32 count,
        const int32 capacity
    )
    {
        const int32 newCapacity = destination.Allocate(capacity);

        Memory::MoveItems(destination.Get(), source.Get(), count);
        Memory::DestructItems(source.Get(), count);
        source.Free();

        return newCapacity;
    }


public:
    /// <summary>
    /// This function transfers the data from the source allocation to the destination allocation.
    /// If possible, it will use allocation move constructor to avoid moving the individual elements.
    /// </summary>
    /// <param name="source"> The source allocation. After the call, it will be empty. </param>
    /// <param name="destination"> The destination allocation. It must be empty. </param>
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements from 0 to count-1 are valid, meaning that they are constructed and can be moved. </param>
    /// <param name="capacity"> The capacity of the destination allocation. If a new allocation is not required, this parameter will be ignored. </param>
    /// <returns> The new capacity of the destination allocation. </returns>
    /// <remarks> Empty allocation means it has 0 capacity (outside of Allocation and Free). </remarks>
    template<
        typename Allocation,
        typename TEnableIf<IsMoveConstructible<Allocation>, int>::Type = 0 // Move constructible
    >
    FORCE_INLINE static int32 MoveAllocated(
        typename Allocation::template Data<T>& source,
        typename Allocation::template Data<T>& destination,
        const int32 count,
        const int32 capacity
    )
    {
        if (source.CanMove())
        {
            destination = ::MoveTemp(source);
            return capacity;
        }

        return MoveAllocatedOneByOne<Allocation>(source, destination, count, capacity);
    }

    /// <summary>
    /// This function transfers the data from the source allocation to the destination allocation.
    /// If possible, it will use allocation move constructor to avoid moving the individual elements.
    /// </summary>
    /// <param name="source"> The source allocation. After the call, it will be empty. </param>
    /// <param name="destination"> The destination allocation. It must be empty. </param>
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements [0, count) are valid, meaning that they are constructed and can be moved. </param>
    /// <param name="capacity"> The capacity of the destination allocation. If a new allocation is not required, this parameter will be ignored. </param>
    /// <returns> The new capacity of the destination allocation. </returns>
    /// <remarks> Empty allocation means it has 0 capacity (outside of Allocation and Free). </remarks>
    template<
        typename Allocation,
        typename TEnableIf<!IsMoveConstructible<Allocation>, int>::Type = 0 // NOT move constructible
    >
    FORCE_INLINE static int32 MoveAllocated(
        typename Allocation::template Data<T>& source,
        typename Allocation::template Data<T>& destination,
        const int32 count,
        const int32 capacity
    )
    {
        return MoveAllocatedOneByOne<Allocation>(source, destination, count, capacity);
    }


    /// <summary> This function attempts to relocate the elements from the current allocation to a new allocation, to alter the capacity. </summary>
    /// <param name="allocation"> The current allocation. </param>
    /// <param name="desiredCapacity"> The desired capacity of the allocation, be it higher or lower. </param>
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements [0, count) are valid, meaning that they are constructed and can be moved. </param>
    /// <returns> The new capacity of the allocation. </returns>
    template<
        typename Allocation,
        typename TEnableIf<IsMoveConstructible<Allocation>, int>::Type = 0 // Move constructible
    >
    FORCE_INLINE static int32 Relocate(
        typename Allocation::template Data<T>& allocation,
        const int32 desiredCapacity,
        const int32 count
    )
    {
        ASSERT(Math::IsInRange(desiredCapacity, Allocation::MinCapacity, Allocation::MaxCapacity)); // Ensure that the desired capacity is within the bounds of the allocation policy.

        // Invoking this method means that an allocation must happen!
        // Collection should take responsibility of determining that a relocation is actually required.
        // It must also ensure that the new capacity obeys the rules of the allocation policy.

        typename Allocation::template Data<T> newAllocation;

        const int32 newCapacity = newAllocation.Allocate(desiredCapacity); // Allocate new memory.
        const int32 newCount = Math::Min(count, newCapacity); // Determine how many elements can be moved, how many fit in the new allocation.

        Memory::MoveItems(newAllocation.Get(), allocation.Get(), newCount); // Move fitting elements.
        Memory::DestructItems(allocation.Get(), count); // Destruct all elements in the old allocation. (Both moved and not fitting)

        allocation.Free(); // Free the old allocation...
        allocation = ::MoveTemp(newAllocation); // ...and replace it with the new one.

        return newCapacity; // Return the new capacity, so the collection can update its internal state.
    }

    /// <summary> This function attempts to relocate the elements from the current allocation to a new allocation, to alter the capacity. </summary>
    /// <param name="allocation"> The current allocation. </param>
    /// <param name="desiredCapacity"> The desired capacity of the allocation, be it higher or lower. </param>
    /// <param name="count"> The number of elements to move. Algorithm assumes that elements [0, count) are valid, meaning that they are constructed and can be moved. </param>
    /// <returns> The new capacity of the allocation. </returns>
    template<
        typename Allocation,
        typename TEnableIf<!IsMoveConstructible<Allocation>, int>::Type = 0 // NOT move constructible
    >
    FORCE_INLINE static int32 Relocate(
        typename Allocation::template Data<T>& allocation,
        const int32 desiredCapacity,
        const int32 count
    )
    {
        ASSERT(Math::IsInRange(desiredCapacity, Allocation::MinCapacity, Allocation::MaxCapacity)); // Ensure that the desired capacity is within the bounds of the allocation policy.

        // Relocating elements is not possible for non-move constructible types.
        // Thus, the only way to change the capacity is to destroy not fitting elements and cap the capacity.

        const int32 newCount = Math::Min(count, desiredCapacity); // Determine how many elements can stay, how many fit in the hypothetical allocation.

        Memory::DestructItems(allocation.Get() + newCount, count - newCount); // Destruct all elements that do not fit in the new allocation. //TODO Should that even be a thing?

        return desiredCapacity; // Return the new capacity, so the collection can update its internal state.
    }
};
