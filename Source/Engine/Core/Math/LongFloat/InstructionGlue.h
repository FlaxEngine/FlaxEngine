#pragma once
#include "Engine/Core/Types/BaseTypes.h"

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin0.inl.h>
#else
#include <cstdint>
#endif

namespace LongFloatGlue
{
#if defined(_MSC_VER) && defined(_M_X64)
    __forceinline unsigned char __cdecl AddCarryUnsignedInt64(
        unsigned char carryIn, unsigned __int64 lhs, unsigned __int64 rhs, unsigned __int64* result)
    {
        return _addcarry_u64(carryIn, lhs, rhs, result);
    }
#else
    inline unsigned char __cdecl AddCarryUnsignedInt64(
        unsigned char carryIn, uint64 lhs, uint64 rhs, uint64* result)
    {
        //Portable crap code :) say bye to performance

        const uint64 sum = lhs + rhs;
        const unsigned char carryFromAdd = sum < lhs;
        *result = sum + carryIn;
        const unsigned char carryFromCarryIn = *result < sum;
        return static_cast<unsigned char>(carryFromAdd | carryFromCarryIn);
    }
#endif

#if defined(_MSC_VER) && defined(_M_X64)
    __forceinline unsigned char __cdecl SubtractBorrowUnsignedInt64(
        unsigned char borrowIn, unsigned __int64 lhs, unsigned __int64 rhs, unsigned __int64* result)
    {
        return _subborrow_u64(borrowIn, lhs, rhs, result);
    }
#else
    inline unsigned char __cdecl SubtractBorrowUnsignedInt64(
        unsigned char borrowIn, uint64 lhs, uint64 rhs, uint64* result)
    {
        //Portable crap code :) say bye to performance

        const uint64 difference = lhs - rhs;
        const unsigned char borrowFromSubtract = lhs < rhs;
        *result = difference - borrowIn;
        const unsigned char borrowFromBorrowIn = difference < borrowIn;
        return static_cast<unsigned char>(borrowFromSubtract | borrowFromBorrowIn);
    }
#endif

#if defined(_MSC_VER) && defined(_M_X64)
    __forceinline unsigned char BitScanReverse(unsigned long* index, unsigned long mask)
    {
        return _BitScanReverse(index, mask);
    }
#else
    // Match _BitScanReverse: return zero for an empty mask, else store the top set-bit index.
    inline unsigned char BitScanReverse(unsigned long* index, unsigned long mask)
    {
        if (mask == 0) return 0;
        unsigned long bit = 0;
        while (mask >>= 1) ++bit;
        *index = bit;
        return 1;
    }
#endif
#if defined(_MSC_VER) && defined(_M_X64)
    __forceinline unsigned __int64 UnsignedMultiply128(
        unsigned __int64 _Multiplier,
        unsigned __int64 _Multiplicand,
        unsigned __int64* _HighProduct)
    {
        return _umul128(_Multiplier, _Multiplicand, _HighProduct);
    }
#elif defined(__SIZEOF_INT128__)
    // Native 128-bit multiplication yields both halves directly.
    inline uint64 UnsignedMultiply128(
        uint64 multiplier,
        uint64 multiplicand,
        uint64* highProduct)
    {
        const unsigned __int128 product =
            static_cast<unsigned __int128>(multiplier) * multiplicand;
        *highProduct = static_cast<uint64>(product >> 64);
        return static_cast<uint64>(product);
    }
#else
    inline uint64 UnsignedMultiply128(
        uint64 multiplier,
        uint64 multiplicand,
        uint64* highProduct)
    {
        //Portable crap code :) say bye to performance

        const uint64 a0 = static_cast<uint32>(multiplier);
        const uint64 a1 = multiplier >> 32;
        const uint64 b0 = static_cast<uint32>(multiplicand);
        const uint64 b1 = multiplicand >> 32;
        const uint64 p00 = a0 * b0;
        const uint64 p01 = a0 * b1;
        const uint64 p10 = a1 * b0;
        const uint64 p11 = a1 * b1;
        const uint64 middle = (p00 >> 32) +
            static_cast<uint32>(p01) + static_cast<uint32>(p10);
        *highProduct = p11 + (p01 >> 32) + (p10 >> 32) + (middle >> 32);
        return (middle << 32) | static_cast<uint32>(p00);
    }
#endif

#if defined(_MSC_VER) && defined(_M_X64)
    __forceinline unsigned __int64 UnsignedDivide128(
        unsigned __int64 highDividend,
        unsigned __int64 lowDividend,
        unsigned __int64 divisor,
        unsigned __int64* remainder)
    {
        return _udiv128(highDividend, lowDividend, divisor, remainder);
    }
#elif defined(__SIZEOF_INT128__)
    // Native 128-bit division produces the quotient and remainder.
    inline uint64 UnsignedDivide128(
        uint64 highDividend,
        uint64 lowDividend,
        uint64 divisor,
        uint64* remainder)
    {
        const unsigned __int128 dividend =
            (static_cast<unsigned __int128>(highDividend) << 64) | lowDividend;
        const uint64 quotient =
            static_cast<uint64>(dividend / divisor);
        *remainder = static_cast<uint64>(dividend % divisor);
        return quotient;
    }
#else
    // Restoring long division; requires divisor != 0 and highDividend < divisor.
    inline uint64 UnsignedDivide128(
        uint64 highDividend,
        uint64 lowDividend,
        uint64 divisor,
        uint64* remainder)
    {
        //Portable crap code :) say bye to performance

        // As with _udiv128, callers must ensure divisor != 0 and highDividend < divisor.
        uint64 rem = highDividend;
        uint64 quotient = 0;
        for (int bit = 63; bit >= 0; --bit)
        {
            const bool carry = (rem >> 63) != 0;
            rem = (rem << 1) | ((lowDividend >> bit) & 1);
            if (carry || rem >= divisor)
            {
                rem -= divisor;
                quotient |= uint64(1) << bit;
            }
        }
        *remainder = rem;
        return quotient;
    }
#endif

#if defined(_MSC_VER) && defined(_M_X64)
    __forceinline unsigned char BitScanReverse64(
        unsigned long* index, unsigned __int64 mask)
    {
        return _BitScanReverse64(index, mask);
    }
#else
    // Shift until the highest set bit reaches bit zero; zero has no index.
    inline unsigned char BitScanReverse64(unsigned long* index, uint64 mask)
    {
        if (mask == 0) return 0;
        uint64 bit = 0;
        while (mask >>= 1) ++bit;
        *index = bit;
        return 1;
    }
#endif
}
