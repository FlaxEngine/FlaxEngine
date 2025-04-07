// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Allocation.h"
#include "Engine/Core/Templates.h"

namespace AllocationUtils
{
    // Moves the data from the source allocation to the destination allocation.
    template<typename T, typename AllocationType>
    inline void MoveToEmpty(typename AllocationType::template Data<T>& to, typename AllocationType::template Data<T>& from, const int32 fromCount, const int32 fromCapacity)
    {
        if IF_CONSTEXPR (AllocationType::HasSwap)
            to.Swap(from);
        else
        {
            to.Allocate(fromCapacity);
            Memory::MoveItems(to.Get(), from.Get(), fromCount);
            Memory::DestructItems(from.Get(), fromCount);
            from.Free();
        }
    }
}
