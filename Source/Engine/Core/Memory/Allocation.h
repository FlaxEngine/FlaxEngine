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
    enum { HasContext = false }; //TODO(mtszkarbowiak) Replace with SFINAE

    class alignas(sizeof(void*)) Data
    {
    private:
        byte _data[Capacity];

    public:
        /// <summary> Initializes new fixed allocation, by doing nothing. </summary>
        FORCE_INLINE Data() = default;

        /// <summary> Deallocates the fixed allocation, by doing nothing. </summary>
        FORCE_INLINE ~Data() = default;


        /// <summary> No allocation can be copied, because it has no information about type. </summary>
        Data(const Data&) = delete;

        /// <summary> No allocation can be copied, because it has no information about type. </summary>
        auto operator=(const Data&)->Data & = delete;


        /// <summary> Inlined allocations must not be moved, because they are not aware of the type. </summary>
        Data(Data&&) = delete;

        /// <summary> Inlined allocations must not be moved, because they are not aware of the type. </summary>
        auto operator=(Data&&) -> Data& = delete;


        FORCE_INLINE void* Get()
        {
            return _data;
        }

        FORCE_INLINE const void* Get() const
        {
            return _data;
        }

        //TODO(mtszkarbowiak) Move this method to the allocation policy.
        FORCE_INLINE int32 CalculateCapacityGrow(const int32 capacity, const int32 minCapacity) const 
        {
            ASSERT(minCapacity <= Capacity);
            return Capacity;
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }

        // Type erased! Relocating is no longer possible.
        /*FORCE_INLINE void Relocate(const int32 capacity, const int32 oldCount, const int32 newCount)
        {
            ASSERT_LOW_LAYER(capacity <= Capacity);
        }*/

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
    enum { HasContext = false }; //TODO(mtszkarbowiak) Replace with SFINAE

    class Data
    {
    private:
        void* _data = nullptr;

    public:
        FORCE_INLINE Data() = default;

        FORCE_INLINE ~Data()
        {
            Allocator::Free(_data);
        }


        /// <summary> No allocation can be copied, because it has no information about type. </summary>
        Data(const Data&) = delete;

        /// <summary> No allocation can be copied, because it has no information about type. </summary>
        auto operator=(const Data&)->Data & = delete;

        /// <summary>
        /// Initializes a new heap allocation data by moving.
        /// </summary>
        /// <remarks>
        /// For simple heap allocations, the move operation is trivial.
        /// It is caused by the fact that the pointer works as a handle.
        /// Moving the handle does not influence the data itself.
        /// </remarks>
        FORCE_INLINE Data(Data&& moved) noexcept
        {
            ::Swap<void*>(this->_data, moved._data);
        }

        /// <summary>
        /// Reassigns a new heap allocation data by moving.
        /// </summary>
        /// <remarks>
        /// For simple heap allocations, the move operation is trivial.
        /// It is caused by the fact that the pointer works as a handle.
        /// Moving the handle does not influence the data itself.
        /// </remarks>
        /// <summary></summary>
        FORCE_INLINE auto operator=(Data&& moved) noexcept -> Data&
        {
            if (this != &moved)
            {
                ::Swap<void*>(this->_data, moved._data);
            }
            return *this;
        }


        FORCE_INLINE void* Get()
        {
            return _data;
        }

        FORCE_INLINE const void* Get() const
        {
            return _data;
        }

        //TODO(mtszkarbowiak) Move this method to the allocation policy.
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

            // _data = static_cast<T*>(Allocator::Allocate(capacity * sizeof(T)));
            _data = Allocator::Allocate(capacity);

#if !BUILD_RELEASE
            if (!_data)
                OUT_OF_MEMORY;
#endif
        }

        /*FORCE_INLINE void Relocate(const int32 capacity, const int32 oldCount, const int32 newCount)
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
        }*/

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
class InlinedAllocation //TODO(mtszkarbowiak) Deprecate this class and swap for PolymorphicAllocation<FixedAllocation<Capacity>, OtherAllocator=HeapAllocation>
{
public:
    enum { HasContext = false }; //TODO(mtszkarbowiak) Replace with SFINAE

    class alignas(sizeof(void*)) Data
    {
    private:
        using OtherData = typename OtherAllocator::Data;

        bool _useOther = false;
        byte _data[Capacity];
        OtherData _other;

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE ~Data()
        {
        }


        /// <summary> No allocation can be copied, because it has no information about type. </summary>
        Data(const Data&) = delete;

        /// <summary> No allocation can be copied, because it has no information about type. </summary>
        auto operator=(const Data&)->Data & = delete;

        /// <summary> Inlined allocations must not be moved, because they are not aware of the type. </summary>
        Data(Data&&) = delete;

        /// <summary> Inlined allocations must not be moved, because they are not aware of the type. </summary>
        auto operator=(Data&&)->Data & = delete;


        FORCE_INLINE void* Get()
        {
            return _useOther ? _other.Get() : (_data);
        }

        FORCE_INLINE const void* Get() const
        {
            return _useOther ? _other.Get() : (_data);
        }

        //TODO(mtszkarbowiak) Move this method to the allocation policy.
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

        /*FORCE_INLINE void Relocate(const int32 capacity, const int32 oldCount, const int32 newCount)
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
        }*/

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

using DefaultAllocation = HeapAllocation;

// Notes:
//
// Allocation Policy Interface:
// - type Data
// - (optional) type Context
//
// Allocation Policy Data Interface:
// - Get -> void*
// - Get const -> const void*
// - Allocate(const int32 capacity)
// - Free
// - default constructor
// - (optional) context constructor
// - (optional) move operators
//
// TODO(mtszkarbowiak) Remove the note.

template<typename T>
class AllocationOperation
{
public:
    /// <summary>
    /// Moves linearly-allocated elements from the source allocation to the target allocation.
    /// </summary>
    template<
        typename AllocationData = DefaultAllocation::Data,
        std::enable_if_t<std::is_move_assignable<AllocationData>::value, int> = 0
    >
    static void MoveLinearAllocation( //TODO(mtszkarbowiak) Handle changing capacity only.
        AllocationData* src,
        AllocationData* target, 
        const int32 count, 
        const int32 capacity
    )
    {
        *target = MoveTemp(*src);
    }

    /// <summary>
    /// Moves linearly-allocated elements from the source allocation to the target allocation.
    /// </summary>
    template<
        typename AllocationData = DefaultAllocation::Data,
        std::enable_if_t<!std::is_move_assignable<AllocationData>::value, int> = 0
    >
    static void MoveLinearAllocation(
        AllocationData* src,
        AllocationData* target,
        const int32 count,
        const int32 capacity
    )
    {
        target->Allocate(capacity);

        Memory::MoveItems<T, T>(
            reinterpret_cast<T*>(target->Get()),
            reinterpret_cast<T*>(src->Get()),
            count
        );

        Memory::DestructItems<T>(
            reinterpret_cast<T*>(src->Get()),
            count
        );  

        src->Free();
    }
};
