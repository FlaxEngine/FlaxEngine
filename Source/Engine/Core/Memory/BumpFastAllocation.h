// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Allocation.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/BitArray.h"

 // This flag enables safety checks for freed memory.
#define ASSERT_SAFE_BUMP_ALLOC 0

#if ASSERT_SAFE_BUMP_ALLOC
#include "Engine/Core/Collections/HashSet.h"
#endif

/// <summary>
/// Memory allocation policy with the fastest possible allocation and deallocation.
/// It uses a simple bump allocator that allocates memory in a linear fashion with alignment support.
/// It does not synchronize access to the memory block, so it is not thread-safe.
/// Context must be reset manually between uses.
/// </summary>
class BumpFastAllocation
{
public:
    enum { HasSwap = true };
    enum { HasContext = true };

    class Context
    {
    private:
        byte* _data = nullptr;
        byte* _end = nullptr;
        byte* _bound = nullptr;

#if ASSERT_SAFE_BUMP_ALLOC
        HashSet<byte*> _allocations;
#endif

    public:
        Context() = delete;

        explicit Context(
            const uintptr capacity, 
            const uintptr alignment = sizeof(byte*)
        )
        {
            _data = static_cast<byte*>(Allocator::Allocate(capacity, alignment));
            _bound = _data;
            _end = _data + capacity;
        }

        /// <summary>
        /// Copying allocation context is not allowed to prevent pointer decay.
        /// </summary>
        Context(const Context&) = delete;

        /// <summary>
        /// Moving allocation context is not allowed to prevent pointer decay.
        /// </summary>
        Context(Context&& other) = delete;

        /// <summary>
        /// Copying allocation context is not allowed to prevent pointer decay.
        /// </summary>
        auto operator=(const Context&) -> Context& = delete;

        /// <summary>
        /// Moving allocation context is not allowed to prevent pointer decay.
        /// </summary>
        auto operator=(Context&&) -> Context& = delete;


        FORCE_INLINE uintptr GetUsed() const
        {
            return static_cast<uintptr>(_bound - _data);
        }

        FORCE_INLINE uintptr GetCapacity() const
        {
            return static_cast<uintptr>(_end - _data);
        }


        ~Context()
        {
            if (_data != nullptr)
                Allocator::Free(_data);
        }

        /// <summary>
        /// Tries to allocate memory of the specified size and alignment.
        /// </summary>
        /// <returns>Pointer to the allocated memory or <c>nullptr</c> if the allocation failed.</returns>
        auto BumpAllocate(const uintptr size, const uintptr alignment = sizeof(byte*)) -> void*
        {
            // Calculate the aligned address.
            byte* aligned = MemoryUtils::Align(_bound, alignment);

            // Calculate the new bound.
            byte* newBound = aligned + size;

            // Check if the allocation fits in the memory block.
            if (newBound > _end) 
            {
                // If not, we failed to allocate the memory :(
                return nullptr;
            }

            // Update the bound.
            _bound = newBound;
            return aligned;
        }

#if ASSERT_SAFE_BUMP_ALLOC
    private:
        HashSet<void*> _allocations;

    public:
        void MarkFreed(void* pointer)
        {
            if (_allocations.Contains(pointer))
            {
                _allocations.Remove(pointer);
            }
        }
#endif

        /// <summary>
        /// Resets the pointer to the beginning of the memory block.
        /// </summary>
        /// <param name="clear">If <c>true</c> the memory will be zeroed.</param>
        /// <remarks>
        /// This method assumes, that all memory allocated from this context is no longer needed.
        /// </remarks>
        void Reset(const bool clear = false)
        {
            if (clear)
            {
                Platform::MemoryClear(_data, GetUsed());
            }

#if ASSERT_SAFE_BUMP_ALLOC
            if (_allocations.Count() > 0)
            {
                LOG(Error, "Memory leak detected! Count: {}", _allocations.Count());
                CRASH;
            }
#endif

            // Just move the pointer to the beginning of the memory block.
            _bound = _data;
        }
    };

    template<typename T>
    class Data
    {
    private:
        Context* _context = nullptr;
        T* _data = nullptr;

    public:
        Data()
        {
        }

        FORCE_INLINE explicit Data(Context& context)
        {
            _context = &context;
        }

        FORCE_INLINE T* Get()
        {
            return _data;
        }

        FORCE_INLINE const T* Get() const
        {
            return _data;
        }

        FORCE_INLINE int32 CalculateCapacityGrow(const int32 capacity, const int32 minCapacity) const
        {
            if (capacity < minCapacity)
                return minCapacity;

            if (capacity < 8)
                return 8;

            return MemoryUtils::NextPow2(capacity);
        }

        FORCE_INLINE void Allocate(const uintptr capacity)
        {
            ASSERT_LOW_LAYER(_context); // Allocating without context is not allowed.
            ASSERT_LOW_LAYER(!_data); // Allocating already allocated memory is not allowed.

            _data = static_cast<T*>(_context->BumpAllocate(capacity * sizeof(T)));

            // If the allocation failed, try to allocate using the backup allocator.
            if (_data == nullptr)
                _data = static_cast<T*>(Allocator::Allocate(capacity));

            if (!_data)
                OUT_OF_MEMORY;
        }

        FORCE_INLINE void Relocate(const uintptr capacity, const uintptr oldCount, const uintptr newCount)
        {
            const bool isNewEmpty = newCount == 0;

            T* newData = nullptr;

            if (!isNewEmpty)
                newData = static_cast<T*>(_context->BumpAllocate(capacity * sizeof(T)));

            if (newData == nullptr)
                newData = static_cast<T*>(Allocator::Allocate(capacity));

            if (newData == nullptr)
                OUT_OF_MEMORY;

            if (oldCount > 0)
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
            if (!_context)
            {
                Allocator::Free(_data);
            }

#if ASSERT_SAFE_BUMP_ALLOC
            else
            {
                _context->MarkFreed(_data);
            }
#endif
        }

        FORCE_INLINE void Swap(Data& other)
        {
            ::Swap(_context, other._context);
            ::Swap(_data, other._data);
        }
    };
};
