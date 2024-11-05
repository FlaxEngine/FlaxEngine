// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Memory/Allocation.h"
#include "Engine/Core/Math/Math.h"

/// <summary>
/// Template for ring buffer with variable capacity.
/// </summary>
template<typename T, typename AllocationType = HeapAllocation>
class RingBuffer
{
public:
    using ItemType = T;
    using AllocationData = typename AllocationType::template Data<T>;

private:
    int32 _front = 0, _back = 0, _count = 0, _capacity = 0;
    AllocationData _allocation;

public:
    ~RingBuffer()
    {
        Memory::DestructItems(Get() + Math::Min(_front, _back), _count);
    }

    FORCE_INLINE T* Get()
    {
        return _allocation.Get();
    }

    FORCE_INLINE int32 Count() const
    {
        return _count;
    }

    FORCE_INLINE int32 Capacity() const
    {
        return _capacity;
    }

    void PushBack(const T& data)
    {
        if (_capacity == 0 || _capacity == _count)
        {
            const int32 capacity = _allocation.CalculateCapacityGrow(_capacity, _count + 1);
            AllocationData alloc;
            alloc.Allocate(capacity);
            const int32 frontCount = Math::Min(_capacity - _front, _count);
            Memory::MoveItems(alloc.Get(), _allocation.Get() + _front, frontCount);
            Memory::DestructItems(_allocation.Get() + _front, frontCount);
            const int32 backCount = _count - frontCount;
            Memory::MoveItems(alloc.Get() + frontCount, _allocation.Get(), backCount);
            Memory::DestructItems(_allocation.Get(), backCount);
            _allocation.Swap(alloc);
            _front = 0;
            _back = _count;
            _capacity = capacity;
        }
        Memory::ConstructItems(_allocation.Get() + _back, &data, 1);
        _back = (_back + 1) % _capacity;
        ++_count;
    }

    FORCE_INLINE T& PeekFront()
    {
        return _allocation.Get()[_front];
    }

    FORCE_INLINE const T& PeekFront() const
    {
        return _allocation.Get()[_front];
    }

    FORCE_INLINE T& operator[](const int32 index)
    {
        ASSERT(index >= 0 && index < _count);
        return _allocation.Get()[(_front + index) % _capacity];
    }

    FORCE_INLINE const T& operator[](const int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _allocation.Get()[(_front + index) % _capacity];
    }

    void PopFront()
    {
        Memory::DestructItems(_allocation.Get() + _front, 1);
        _front = (_front + 1) % _capacity;
        --_count;
    }

    void Clear()
    {
        Memory::DestructItems(Get() + Math::Min(_front, _back), _count);
        _front = _back = _count = 0;
    }

    void Release()
    {
        Clear();
        _allocation.Free();
    }
};
