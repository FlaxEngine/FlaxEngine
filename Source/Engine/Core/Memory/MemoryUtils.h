// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"
#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Utility class for memory operations.
/// </summary>
/// <remarks>
/// Some methods may overlap with Math header.
/// This is intentional to avoid including Math.h in every file that needs these methods.
/// </remarks>
class MemoryUtils
{
public:
    template<typename T>
    FORCE_INLINE static bool IsPow2(T value)
    {
        static_assert(std::is_integral<T>::value, "T must be an integral type");
        return (value & (value - 1)) == 0;
    }

    FORCE_INLINE static int32 NextPow2(int32 value)
    {
        // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        ++value;
        return value;
    }

    FORCE_INLINE static uint32 NextPow2(uint32 value)
    {
        // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        ++value;
        return value;
    }

    FORCE_INLINE static uint64 NextPow2(uint64 value)
    {
        // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        ++value;
        return value;
    }



    FORCE_INLINE static uintptr AlignAddress(const uintptr address, const uintptr alignment)
    {
        ASSERT_LOW_LAYER(IsPow2(alignment));
        const uintptr mask = alignment - 1;
        return (address + mask) & ~mask;
    }

    template<typename T>
    FORCE_INLINE static T* Align(T* pointer, const uintptr alignment)
    {
        const uintptr address = reinterpret_cast<uintptr>(pointer);
        const uintptr aligned = AlignAddress(address, alignment);
        return reinterpret_cast<T*>(aligned);
    }
};
