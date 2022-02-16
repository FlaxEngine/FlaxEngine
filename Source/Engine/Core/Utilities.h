// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types/BaseTypes.h"
#include "Types/String.h"
#if _MSC_VER && PLATFORM_SIMD_SSE4_2
#include <intrin.h>
#endif

namespace Utilities
{
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

    // Converts size of the file (in bytes) to the best fitting string
    // @param bytes Size of the file in bytes
    // @return The best fitting string of the file size
    template<typename T>
    String BytesToText(T bytes)
    {
        static const Char* sizes[] = { TEXT("B"), TEXT("KB"), TEXT("MB"), TEXT("GB"), TEXT("TB") };
        uint64 i = 0;
        double dblSByte = static_cast<double>(bytes);
        for (; static_cast<uint64>(bytes / 1024.0) > 0; i++, bytes /= 1024)
            dblSByte = bytes / 1024.0;
        if (i >= ARRAY_COUNT(sizes))
            return String::Empty;
        return String::Format(TEXT("{0} {1}"), RoundTo2DecimalPlaces(dblSByte), sizes[i]);
    }

    // Returns the amount of set bits in 32-bit integer.
    inline int32 CountBits(uint32 x)
    {
        // [Reference: https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer]
#ifdef __GNUC_
        return __builtin_popcount(x);
#elif _MSC_VER && PLATFORM_SIMD_SSE4_2
        return __popcnt(x);
#else
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        x = (x + (x >> 4)) & 0x0F0F0F0F;
        return (x * 0x01010101) >> 24;
#endif  
    }
}
