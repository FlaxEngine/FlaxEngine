#pragma once

#include <cstdint>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin0.inl.h>
#endif

namespace LongFloatGlue
{
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
    inline std::uint64_t UnsignedMultiply128(
        std::uint64_t multiplier,
        std::uint64_t multiplicand,
        std::uint64_t* highProduct)
    {
        const unsigned __int128 product =
            static_cast<unsigned __int128>(multiplier) * multiplicand;
        *highProduct = static_cast<std::uint64_t>(product >> 64);
        return static_cast<std::uint64_t>(product);
    }
#else
    inline std::uint64_t UnsignedMultiply128(
        std::uint64_t multiplier,
        std::uint64_t multiplicand,
        std::uint64_t* highProduct)
    {
        //Portable crap code :) say bye to performance

        const std::uint64_t a0 = static_cast<std::uint32_t>(multiplier);
        const std::uint64_t a1 = multiplier >> 32;
        const std::uint64_t b0 = static_cast<std::uint32_t>(multiplicand);
        const std::uint64_t b1 = multiplicand >> 32;
        const std::uint64_t p00 = a0 * b0;
        const std::uint64_t p01 = a0 * b1;
        const std::uint64_t p10 = a1 * b0;
        const std::uint64_t p11 = a1 * b1;
        const std::uint64_t middle = (p00 >> 32) +
            static_cast<std::uint32_t>(p01) + static_cast<std::uint32_t>(p10);
        *highProduct = p11 + (p01 >> 32) + (p10 >> 32) + (middle >> 32);
        return (middle << 32) | static_cast<std::uint32_t>(p00);
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
    inline std::uint64_t UnsignedDivide128(
        std::uint64_t highDividend,
        std::uint64_t lowDividend,
        std::uint64_t divisor,
        std::uint64_t* remainder)
    {
        const unsigned __int128 dividend =
            (static_cast<unsigned __int128>(highDividend) << 64) | lowDividend;
        const std::uint64_t quotient =
            static_cast<std::uint64_t>(dividend / divisor);
        *remainder = static_cast<std::uint64_t>(dividend % divisor);
        return quotient;
    }
#else
    // Restoring long division; requires divisor != 0 and highDividend < divisor.
    inline std::uint64_t UnsignedDivide128(
        std::uint64_t highDividend,
        std::uint64_t lowDividend,
        std::uint64_t divisor,
        std::uint64_t* remainder)
    {
        //Portable crap code :) say bye to performance

        // As with _udiv128, callers must ensure divisor != 0 and highDividend < divisor.
        std::uint64_t rem = highDividend;
        std::uint64_t quotient = 0;
        for (int bit = 63; bit >= 0; --bit)
        {
            const bool carry = (rem >> 63) != 0;
            rem = (rem << 1) | ((lowDividend >> bit) & 1);
            if (carry || rem >= divisor)
            {
                rem -= divisor;
                quotient |= std::uint64_t(1) << bit;
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
    inline unsigned char BitScanReverse64(unsigned long* index, std::uint64_t mask)
    {
        if (mask == 0) return 0;
        std::uint64_t bit = 0;
        while (mask >>= 1) ++bit;
        *index = bit;
        return 1;
    }
#endif
}