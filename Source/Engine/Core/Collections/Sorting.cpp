// Copyright (c) Wojciech Figat. All rights reserved.

#include "Sorting.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Threading/ThreadLocal.h"

// Use a cached storage for the sorting (one per thread to reduce locking)
ThreadLocal<Sorting::SortingStack*> SortingStacks;

Sorting::SortingStack& Sorting::SortingStack::Get()
{
    SortingStack*& stack = SortingStacks.Get();
    if (!stack)
        stack = New<SortingStack>();
    return *stack;
}

Sorting::SortingStack::SortingStack()
{
}

Sorting::SortingStack::~SortingStack()
{
    Allocator::Free(Data);
}

void Sorting::SortingStack::SetCapacity(const int32 capacity)
{
    ASSERT(capacity >= 0);
    if (capacity == Capacity)
        return;
    int32* newData = nullptr;
    if (capacity > 0)
        newData = (int32*)Allocator::Allocate(capacity * sizeof(int32));
    const int32 newCount = Count < capacity ? Count : capacity;
    if (Data)
    {
        if (newData && newCount)
            Platform::MemoryCopy(newData, Data, newCount * sizeof(int32));
        Allocator::Free(Data);
    }
    Data = newData;
    Capacity = capacity;
    Count = newCount;
}

void Sorting::SortingStack::EnsureCapacity(int32 minCapacity)
{
    if (Capacity >= minCapacity)
        return;
    int32 num = Capacity == 0 ? 64 : Capacity * 2;
    if (num < minCapacity)
        num = minCapacity;
    SetCapacity(num);
}
