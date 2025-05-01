// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types/BaseTypes.h"
#include "Types/String.h"
#include "Types/Span.h"
#if _MSC_VER && PLATFORM_SIMD_SSE4_2
#include <intrin.h>
#endif

namespace Utilities
{
    struct Private
    {
        static FLAXENGINE_API Span<const Char*> BytesSizes;
        static FLAXENGINE_API Span<const Char*> HertzSizes;
    };

    // Round floating point value up to 1 decimal place
    template<typename T>
    FORCE_INLINE T RoundTo1DecimalPlace(T value)
    {
        return (T)round((double)value * 10) / (T)10;
    }

    // Round floating point value up to 2 decimal places
    template<typename T>
    FORCE_INLINE T RoundTo2DecimalPlaces(T value)
    {
        return (T)round((double)value * 100.0) / (T)100;
    }

    // Round floating point value up to 3 decimal places
    template<typename T>
    FORCE_INLINE T RoundTo3DecimalPlaces(T value)
    {
        return (T)round((double)value * 1000.0) / (T)1000;
    }

    /// <summary>
    /// Converts units to the best fitting human-readable denominator.
    /// </summary>
    /// <typeparam name="T">Value type.</typeparam>
    /// <param name="units">Units count</param>
    /// <param name="divider">Amount of units required for the next size.</param>
    /// <param name="sizes">Array with human-readable sizes to convert from.</param>
    /// <returns>The best fitting string of the units.</returns>
    template<typename T>
    String UnitsToText(T units, int32 divider, const Span<const Char*> sizes)
    {
        if (sizes.Length() == 0)
            return String::Format(TEXT("{0}"), units);
        int32 i = 0;
        double dblSUnits = static_cast<double>(units);
        for (; static_cast<uint64>(units / static_cast<double>(divider)) > 0; i++, units /= divider)
            dblSUnits = (double)units / (double)divider;
        if (i >= sizes.Length())
            i = 0;
        String text = String::Format(TEXT("{}"), RoundTo2DecimalPlaces(dblSUnits));
        const int32 dot = text.FindLast('.');
        if (dot != -1)
            text = text.Left(dot + 3);
        return String::Format(TEXT("{0} {1}"), text, sizes[i]);
    }

    /// <summary>
    /// Converts size of the data (in bytes) to the best fitting string.
    /// </summary>
    /// <typeparam name="T">Value type.</typeparam>
    /// <param name="bytes">Size of the data in bytes.</param>
    /// <returns>The best fitting string of the data size.</returns>
    template<typename T>
    String BytesToText(T bytes)
    {
        return UnitsToText(bytes, 1024, Private::BytesSizes);
    }

    /// <summary>
    /// Converts hertz to the best fitting string.
    /// </summary>
    /// <typeparam name="T">Value type.</typeparam>
    /// <param name="hertz">Value in hertz for conversion.</param>
    /// <returns>The best fitting string.</returns>
    template<typename T>
    String HertzToText(T hertz)
    {
        return UnitsToText(hertz, 1000, Private::HertzSizes);
    }

    // Returns the amount of set bits in 32-bit integer.
    inline int32 CountBits(uint32 x)
    {
#ifdef __GNUC_
        return __builtin_popcount(x);
#elif _MSC_VER && PLATFORM_SIMD_SSE4_2
        return __popcnt(x);
#else
        // [Reference: https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer]
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        x = (x + (x >> 4)) & 0x0F0F0F0F;
        return (x * 0x01010101) >> 24;
#endif  
    }

    // Returns the index of the highest set bit. Assumes input is non-zero.
    inline uint32 HighestSetBit(uint32 x)
    {
#if _MSC_VER
        unsigned long result;
        _BitScanReverse(&result, x);
        return result;
#elif __clang__
        return 31 - __builtin_clz(x);
#else
        // [Reference: http://graphics.stanford.edu/~seander/bithacks.html]
        static const uint32 MultiplyDeBruijnBitPosition[32] =
        {
            0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
            8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
        };
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return MultiplyDeBruijnBitPosition[(uint32)(v * 0x07C4ACDDU) >> 27];
#endif
    }

    // Returns the index of the lowest set bit. Assumes input is non-zero.
    inline uint32 LowestSetBit(uint32 v)
    {
#if _MSC_VER
        unsigned long result;
        _BitScanForward(&result, v);
        return result;
#elif __clang__
        return __builtin_ctz(v);
#else
        // [Reference: http://graphics.stanford.edu/~seander/bithacks.html]
        static const uint32 MultiplyDeBruijnBitPosition[32] =
        {
            0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
            31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
        };
        int32 w = v;
        return MultiplyDeBruijnBitPosition[(uint32)((w & -w) * 0x077CB531U) >> 27];
#endif
    }

    // Copy memory region but ignoring address sanatizer checks for memory regions.
    NO_SANITIZE_ADDRESS static void UnsafeMemoryCopy(void* dst, const void* src, uint64 size)
    {
#if BUILD_RELEASE
        memcpy(dst, src, static_cast<size_t>(size));
#else
        for (uint64 i = 0; i < size; i++)
            ((byte*)dst)[i] = ((byte*)src)[i];
#endif
    }
}
