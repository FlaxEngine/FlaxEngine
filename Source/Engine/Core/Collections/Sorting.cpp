// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Sorting.h"
#include "Engine/Threading/ThreadLocal.h"

// Use a cached storage for the sorting (one per thread to reduce locking)
ThreadLocal<Sorting::SortingStack, 64> SortingStacks;

Sorting::SortingStack& Sorting::SortingStack::Get()
{
    return SortingStacks.Get();
}
