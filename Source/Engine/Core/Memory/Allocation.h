// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory.h"
#include "Engine/Core/Core.h"
#include "Engine/Core/Math/Math.h"


// --- Memory Allocation Policy Usage ---
// 1. Allocation policy uses subclass of Data to store a handle to one (or zero) memory allocations.
// 2. Allocation itself must never be zero-sized.
// 3. Tracking the capacity is the responsibility of the collection, not the allocation policy.
// 4. Resultant capacity of the allocation can exceed the requested. Allocation shares information about the actual capacity.
// 5. Allocation policy is not responsible for tracking valid elements in the allocation.
// 6. Accessing the data can be only between Allocate and Free calls.
// 7. Requesting to allocate numbers exceeding the limits of the policy is undefined behavior.
// 8. Data can be move-able or not, determining it swap-ability.


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
        constexpr static int32 MinCapacity = 1, MaxCapacity = INT32_MAX;

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
template<int Capacity, typename FallbackAllocation = HeapAllocation>
class InlinedAllocation
{
public:
    template<typename T>
    class alignas(sizeof(void*)) Data
    {
    private:
        using FallbackData = typename FallbackAllocation::template Data<T>;
        
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

using DefaultAllocation = HeapAllocation;
