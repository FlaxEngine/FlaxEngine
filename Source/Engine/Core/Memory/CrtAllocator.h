// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// The default implementation of the memory allocator using traditional ANSI allocation API under the hood (malloc/free).
/// </summary>
class CrtAllocator
{
public:

    /// <summary>
    /// Allocates memory on a specified alignment boundary.
    /// </summary>
    /// <param name="size">The size of the allocation (in bytes).</param>
    /// <param name="alignment">The memory alignment (in bytes). Must be an integer power of 2.</param>
    /// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    FORCE_INLINE static void* Allocate(uint64 size, uint64 alignment = 16)
    {
        return Platform::Allocate(size, alignment);
    }

    /// <summary>
    /// Frees a block of allocated memory.
    /// </summary>
    /// <param name="ptr">A pointer to the memory block to deallocate.</param>
    FORCE_INLINE static void Free(void* ptr)
    {
        Platform::Free(ptr);
    }

    /// <summary>
    /// Gets the name of the allocator.
    /// </summary>
    /// <returns>The name.</returns>
    static const Char* Name()
    {
        return TEXT("Crt");
    }
};
