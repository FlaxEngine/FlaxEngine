#include "LongFloat.h"
#include <iostream>
#include <string>
#include <cmath>
#include "InstructionGlue.h"

constexpr uint64_t LongFloat::SqrtSamples[128] =
{
    0ULL,                    // 0   0
    1ULL,                    // 0   1
    1ULL,                    // 1   2
    2ULL,                    // 2   4
    2ULL,                    // 3   8
    4ULL,                    // 4   16
    5ULL,                    // 5   32
    8ULL,                    // 6   64
    11ULL,                   // 7   128
    16ULL,                   // 8
    22ULL,                   // 9
    32ULL,                   // 10
    45ULL,                   // 11
    64ULL,                   // 12
    90ULL,                   // 13
    128ULL,                  // 14
    181ULL,                  // 15
    256ULL,                  // 16
    362ULL,                  // 17
    512ULL,                  // 18
    724ULL,                  // 19
    1024ULL,                 // 20
    1448ULL,                 // 21
    2048ULL,                 // 22
    2896ULL,                 // 23
    4096ULL,                 // 24
    5792ULL,                 // 25
    8192ULL,                 // 26
    11585ULL,                // 27
    16384ULL,                // 28
    23170ULL,                // 29
    32768ULL,                // 30
    46340ULL,                // 31
    65536ULL,                // 32
    92681ULL,                // 33
    131072ULL,               // 34
    185363ULL,               // 35
    262144ULL,               // 36
    370727ULL,               // 37
    524288ULL,               // 38
    741455ULL,               // 39
    1048576ULL,              // 40
    1482910ULL,              // 41
    2097152ULL,              // 42
    2965820ULL,              // 43
    4194304ULL,              // 44
    5931641ULL,              // 45
    8388608ULL,              // 46
    11863283ULL,             // 47
    16777216ULL,             // 48
    23726566ULL,             // 49
    33554432ULL,             // 50
    47453132ULL,             // 51
    67108864ULL,             // 52
    94906265ULL,             // 53
    134217728ULL,            // 54
    189812531ULL,            // 55
    268435456ULL,            // 56
    379625062ULL,            // 57
    536870912ULL,            // 58
    759250124ULL,            // 59
    1073741824ULL,           // 60
    1518500249ULL,           // 61
    2147483648ULL,           // 62
    3037000499ULL,           // 63
    4294967296ULL,           // 64
    6074000999ULL,           // 65
    8589934592ULL,           // 66
    12148001999ULL,          // 67
    17179869184ULL,          // 68
    24296003999ULL,          // 69
    34359738368ULL,          // 70
    48592007998ULL,          // 71
    68719476736ULL,          // 72
    97184015996ULL,          // 73
    137438953472ULL,         // 74
    194368031992ULL,         // 75
    274877906944ULL,         // 76
    388736063984ULL,         // 77
    549755813888ULL,         // 78
    777472127969ULL,         // 79
    1099511627776ULL,        // 80
    1554944255938ULL,        // 81
    2199023255552ULL,        // 82
    3109888511877ULL,        // 83
    4398046511104ULL,        // 84
    6219777023754ULL,        // 85
    8796093022208ULL,        // 86
    12439554047508ULL,       // 87
    17592186044416ULL,       // 88
    24879108095016ULL,       // 89
    35184372088832ULL,       // 90
    49758216190033ULL,       // 91
    70368744177664ULL,       // 92
    99516432380067ULL,       // 93
    140737488355328ULL,      // 94
    199032864760135ULL,      // 95
    281474976710656ULL,      // 96
    398065729520271ULL,      // 97
    562949953421312ULL,      // 98
    796131459040543ULL,      // 99
    1125899906842624ULL,     // 100
    1592262918081087ULL,     // 101
    2251799813685248ULL,     // 102
    3184525836162175ULL,     // 103
    4503599627370496ULL,     // 104
    6369051672324350ULL,     // 105
    9007199254740992ULL,     // 106
    12738103344648700ULL,    // 107
    18014398509481984ULL,    // 108
    25476206689297400ULL,    // 109
    36028797018963968ULL,    // 110
    50952413378594800ULL,    // 111
    72057594037927936ULL,    // 112
    101904826757189600ULL,   // 113
    144115188075855872ULL,   // 114
    203809653514379200ULL,   // 115
    288230376151711744ULL,   // 116
    407619307028758400ULL,   // 117
    576460752303423488ULL,   // 118
    815238614057516800ULL,   // 119
    1152921504606846976ULL,  // 120
    1630477228115033600ULL,  // 121
    2305843009213693952ULL,  // 122
    3260954456230067200ULL,  // 123
    4611686018427387904ULL,  // 124
    6521908912460134400ULL,  // 125
    9223372036854775808ULL   // 126
};
static bool Divide192By128(const uint32_t numerator[6], const uint32_t divisor[4], uint32_t quotient[6], uint32_t remainder[4])
{
    constexpr uint64_t B = 0x100000000ULL;

    uint32_t u[7]{};
    uint32_t v[4]{};

    for (int i = 0; i < 6; ++i)
        quotient[i] = 0;

    for (int i = 0; i < 4; ++i)
        remainder[i] = 0;

    // Determine the effective operand lengths.
    int numeratorLength = 6;
    int divisorLength = 4;

    while (numeratorLength > 0 &&
        numerator[numeratorLength - 1] == 0)
        --numeratorLength;

    while (divisorLength > 0 &&
        divisor[divisorLength - 1] == 0)
        --divisorLength;

    if (divisorLength == 0)
        return false;

    if (numeratorLength == 0)
        return true;

    // If numerator < divisor, the quotient is zero.
    if (numeratorLength < divisorLength)
    {
        for (int i = 0; i < numeratorLength; ++i)
            remainder[i] = numerator[i];

        return true;
    }

    if (numeratorLength == divisorLength)
    {
        bool less = false;
        bool greater = false;

        for (int i = numeratorLength - 1; i >= 0; --i)
        {
            if (numerator[i] < divisor[i])
            {
                less = true;
                break;
            }

            if (numerator[i] > divisor[i])
            {
                greater = true;
                break;
            }
        }

        if (less || (!greater && numeratorLength == 0))
        {
            for (int i = 0; i < numeratorLength; ++i)
                remainder[i] = numerator[i];

            return true;
        }

        if (!greater)
        {
            quotient[0] = 1;

            for (int i = 0; i < divisorLength; ++i)
                remainder[i] = 0;

            return true;
        }
    }

    // Normalize the divisor so its highest bit is set.
    unsigned long leadingBit;
    LongFloatGlue::BitScanReverse(
        &leadingBit,
        divisor[divisorLength - 1]);

    const unsigned shift = 31u - leadingBit;

    uint64_t carry = 0;

    for (int i = 0; i < divisorLength; ++i)
    {
        const uint64_t value =
            (static_cast<uint64_t>(divisor[i]) << shift) | carry;

        v[i] = static_cast<uint32_t>(value);
        carry = value >> 32;
    }

    // Normalize the numerator, retaining the extra high limb.
    carry = 0;

    for (int i = 0; i < numeratorLength; ++i)
    {
        const uint64_t value =
            (static_cast<uint64_t>(numerator[i]) << shift) | carry;

        u[i] = static_cast<uint32_t>(value);
        carry = value >> 32;
    }

    u[numeratorLength] = static_cast<uint32_t>(carry);

    const int n = divisorLength;
    const int m = numeratorLength - n;

    // Knuth-style normalized quotient estimation.
    for (int j = m; j >= 0; --j)
    {
        const uint64_t top =
            (static_cast<uint64_t>(u[j + n]) << 32) |
            u[j + n - 1];

        uint64_t qhat = top / v[n - 1];
        uint64_t rhat = top % v[n - 1];

        // The quotient estimate must fit in one 32-bit limb.
        if (qhat >= B)
        {
            qhat = B - 1;
            rhat = top - qhat * v[n - 1];
        }

        // Correct the estimate using the next divisor limb.
        if (n > 1)
        {
            while (rhat < B &&
                qhat * v[n - 2] >
                (rhat << 32) + u[j + n - 2])
            {
                --qhat;
                rhat += v[n - 1];
            }
        }

        // Subtract qhat * v from the corresponding numerator limbs.
        uint64_t productCarry = 0;

        for (int i = 0; i < n; ++i)
        {
            const uint64_t product =
                qhat * v[i] + productCarry;

            productCarry = product >> 32;

            const uint32_t productLow =
                static_cast<uint32_t>(product);

            const uint32_t oldValue = u[j + i];

            u[j + i] = oldValue - productLow;

            if (oldValue < productLow)
                ++productCarry;
        }

        const uint32_t oldTop = u[j + n];

        u[j + n] =
            oldTop - static_cast<uint32_t>(productCarry);

        const bool overestimated = oldTop < productCarry;

        // If the estimate was too large, decrement it and add v back.
        if (overestimated)
        {
            --qhat;

            uint64_t additionCarry = 0;

            for (int i = 0; i < n; ++i)
            {
                const uint64_t sum =
                    static_cast<uint64_t>(u[j + i]) +
                    v[i] +
                    additionCarry;

                u[j + i] = static_cast<uint32_t>(sum);
                additionCarry = sum >> 32;
            }

            u[j + n] += static_cast<uint32_t>(additionCarry);
        }

        quotient[j] = static_cast<uint32_t>(qhat);
    }

    // Undo normalization to recover the remainder.
    if (shift == 0)
    {
        for (int i = 0; i < n; ++i)
            remainder[i] = u[i];
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            remainder[i] =
                (u[i] >> shift) |
                (u[i + 1] << (32 - shift));
        }
    }

    return true;
}

// Experimental base-2^64 Knuth division for a 192-bit numerator by a
// 128-bit divisor. Kept alongside the base-2^32 implementation for A/B tests.
static
#if defined(_MSC_VER)
__forceinline
#else
inline
#endif
uint64_t Divide64Digit(
    uint64_t u[4], const uint64_t v[2], int j)
{
    uint64_t qhat;
    uint64_t rhat;
    bool rhatOverflow = false;
    if (u[j + 2] >= v[1])
    {
        qhat = UINT64_MAX;
        rhat = u[j + 1] + v[1];
        rhatOverflow = rhat < u[j + 1];
    }
    else
    {
        qhat = LongFloatGlue::UnsignedDivide128(u[j + 2], u[j + 1], v[1], &rhat);
    }

    uint64_t p0Low = 0;
    uint64_t p0High = 0;
    for (;;)
    {
        p0Low = LongFloatGlue::UnsignedMultiply128(qhat, v[0], &p0High);
        if (rhatOverflow || p0High < rhat ||
            (p0High == rhat && p0Low <= u[j]))
            break;

        --qhat;
        const uint64_t oldRhat = rhat;
        rhat += v[1];
        rhatOverflow = rhat < oldRhat;
    }

    uint64_t p1High = 0;
    const uint64_t p1Low = LongFloatGlue::UnsignedMultiply128(qhat, v[1], &p1High);
    const uint64_t productMiddle0 = p0High + p1Low;
    const uint64_t carry0 = productMiddle0 < p0High;
    const uint64_t oldU0 = u[j];
    u[j] -= p0Low;
    const uint64_t borrow0 = oldU0 < p0Low;

    const uint64_t productMiddle = productMiddle0 + borrow0;
    const uint64_t carry1 = productMiddle < productMiddle0;
    const uint64_t productTop = p1High + carry0 + carry1;

    const uint64_t oldU1 = u[j + 1];
    u[j + 1] -= productMiddle;
    const uint64_t borrow1 = oldU1 < productMiddle;

    const uint64_t oldU2 = u[j + 2];
    u[j + 2] -= productTop;
    const uint64_t borrow2a = oldU2 < productTop;
    const uint64_t beforeTopBorrow = u[j + 2];
    u[j + 2] -= borrow1;
    const bool overestimated = borrow2a || beforeTopBorrow < borrow1;

    if (overestimated)
    {
        --qhat;
        const uint64_t oldLow = u[j];
        u[j] += v[0];
        const uint64_t carry = u[j] < oldLow;
        const uint64_t oldHigh = u[j + 1];
        u[j + 1] += v[1];
        const uint64_t carryHigh0 = u[j + 1] < oldHigh;
        const uint64_t beforeCarry = u[j + 1];
        u[j + 1] += carry;
        const uint64_t carryHigh1 = u[j + 1] < beforeCarry;
        u[j + 2] += carryHigh0 | carryHigh1;
    }
    return qhat;
}

static bool Divide192By128_64(
    const uint64_t numerator[3],
    const uint64_t divisor[2],
    uint64_t quotient[3])
{
    quotient[0] = quotient[1] = quotient[2] = 0;

    int numeratorLength = 3;
    while (numeratorLength > 0 && numerator[numeratorLength - 1] == 0)
        --numeratorLength;

    int divisorLength = divisor[1] == 0 ? (divisor[0] == 0 ? 0 : 1) : 2;
    if (divisorLength == 0)
        return false;
    if (numeratorLength == 0)
        return true;

    // A one-word divisor uses the processor's 128-by-64 divide directly.
    if (divisorLength == 1)
    {
        uint64_t rem = 0;
        for (int i = numeratorLength - 1; i >= 0; --i)
            quotient[i] = LongFloatGlue::UnsignedDivide128(rem, numerator[i], divisor[0], &rem);
        return true;
    }

    if (numeratorLength < 2 ||
        (numeratorLength == 2 &&
            (numerator[1] < divisor[1] ||
                (numerator[1] == divisor[1] && numerator[0] < divisor[0]))))
    {
        return true;
    }

    if (numeratorLength == 2 && numerator[1] == divisor[1] &&
        numerator[0] == divisor[0])
    {
        quotient[0] = 1;
        return true;
    }

    unsigned long topBit = 0;
    LongFloatGlue::BitScanReverse64(&topBit, divisor[1]);
    const unsigned shift = 63u - topBit;

    uint64_t v[2];
    uint64_t u[4]{};
    if (shift == 0)
    {
        v[0] = divisor[0];
        v[1] = divisor[1];
        u[0] = numerator[0];
        u[1] = numerator[1];
        u[2] = numerator[2];
        u[3] = 0;
    }
    else
    {
        v[0] = divisor[0] << shift;
        v[1] = (divisor[1] << shift) | (divisor[0] >> (64 - shift));
        u[0] = numerator[0] << shift;
        u[1] = (numerator[1] << shift) | (numerator[0] >> (64 - shift));
        u[2] = (numerator[2] << shift) | (numerator[1] >> (64 - shift));
        u[3] = numerator[2] >> (64 - shift);
    }

    // Most fixed-point divisions produce a quotient below 2^64. If the
    // numerator's top 128 bits are already below the divisor, the high
    // quotient limb is certainly zero; skip its expensive estimate/divide.
    const bool highQuotientLimbIsZero =
        numeratorLength == 3 &&
        (numerator[2] < divisor[1] ||
            (numerator[2] == divisor[1] && numerator[1] < divisor[0]));
    if (highQuotientLimbIsZero)
    {
        quotient[0] = Divide64Digit(u, v, 0);
    }
    else
    {
        const int m = numeratorLength - 2;
        for (int j = m; j >= 0; --j)
            quotient[j] = Divide64Digit(u, v, j);
    }

    return true;
}

template<bool Use64BitDivider>
static LongFloat DivideWithAlgorithm(
    const LongFloat& lhs,
    const LongFloat& rhs)
{
    constexpr uint64_t SignMask = 0x8000000000000000ULL;
    constexpr uint64_t Scale = LongFloat::ComputeScale();
    LongFloat out;
    // Division by zero.
    if (rhs.lowbits == 0 && rhs.highbits == 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    const bool lhsNegative = (lhs.highbits & SignMask) != 0;
    const bool rhsNegative = (rhs.highbits & SignMask) != 0;
    const bool resultNegative = lhsNegative != rhsNegative;

    // Convert operands to unsigned magnitudes.
    uint64_t DividendLow = lhs.lowbits;
    uint64_t DividendHigh = lhs.highbits;

    uint64_t DivisorLow = rhs.lowbits;
    uint64_t DivisorHigh = rhs.highbits;

    if (lhsNegative)
    {
        DividendLow = ~DividendLow + 1;
        DividendHigh = ~DividendHigh + (DividendLow == 0);
    }

    if (rhsNegative)
    {
        DivisorLow = ~DivisorLow + 1;
        DivisorHigh = ~DivisorHigh + (DivisorLow == 0);
    }

    // Common case: both magnitudes fit in one word. Scale the dividend into
    // 128 bits and divide directly by the 64-bit divisor instead of building
    // limb arrays and entering the general 192-by-128 divider.
    if (DividendHigh == 0 && DivisorHigh == 0)
    {
        uint64_t NumeratorHigh = 0;
        const uint64_t NumeratorLow =
            LongFloatGlue::UnsignedMultiply128(DividendLow, Scale, &NumeratorHigh);

        uint64_t QuotientHigh = NumeratorHigh / DivisorLow;
        uint64_t DivisionRemainder = NumeratorHigh % DivisorLow;
        uint64_t QuotientLow = LongFloatGlue::UnsignedDivide128(
            DivisionRemainder,
            NumeratorLow,
            DivisorLow,
            &DivisionRemainder);

        // The minimum signed bit pattern is reserved for NaN.
        if ((QuotientHigh & SignMask) != 0)
        {
            out.SetNAN();
            return out;
        }

        if (resultNegative)
        {
            const uint64_t MagnitudeLow = QuotientLow;
            const uint64_t MagnitudeHigh = QuotientHigh;
            QuotientHigh = ~MagnitudeHigh + (MagnitudeLow == 0);
            QuotientLow = ~MagnitudeLow + 1;

            if (QuotientHigh == SignMask && QuotientLow == 0)
            {
                out.SetNAN();
                return out;
            }
        }

        out.highbits = QuotientHigh;
        out.lowbits = QuotientLow;
        return out;
    }

    // Construct abs(lhs) * Scale as a 192-bit numerator.
    uint64_t ProductHigh;
    const uint64_t ProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendLow, Scale, &ProductHigh);

    uint64_t UpperProductHigh;
    const uint64_t UpperProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendHigh, Scale, &UpperProductHigh);

    const uint64_t Numerator0 = ProductLow;

    const uint64_t Numerator1 = ProductHigh + UpperProductLow;
    const uint64_t Carry = Numerator1 < ProductHigh;

    const uint64_t Numerator2 = UpperProductHigh + Carry;

    // The fixed-point quotient truncates toward zero. If the scaled
    // numerator is smaller than the divisor, the answer is exactly zero.
    // Check this before entering the multi-limb division routine.
    if (Numerator2 == 0 &&
        (Numerator1 < DivisorHigh ||
            (Numerator1 == DivisorHigh && Numerator0 < DivisorLow)))
    {
        out.lowbits = 0;
        out.highbits = 0;
        return out;
    }

    // Convert numerator and divisor to base-2^32 limbs.
    const uint32_t Numerator[6] =
    {
        static_cast<uint32_t>(Numerator0),
        static_cast<uint32_t>(Numerator0 >> 32),
        static_cast<uint32_t>(Numerator1),
        static_cast<uint32_t>(Numerator1 >> 32),
        static_cast<uint32_t>(Numerator2),
        static_cast<uint32_t>(Numerator2 >> 32)
    };

    const uint32_t Divisor[4] =
    {
        static_cast<uint32_t>(DivisorLow),
        static_cast<uint32_t>(DivisorLow >> 32),
        static_cast<uint32_t>(DivisorHigh),
        static_cast<uint32_t>(DivisorHigh >> 32)
    };

    uint32_t Quotient[6]{};
    uint32_t Remainder[4]{};

    bool DivideSucceeded;
    if constexpr (Use64BitDivider)
    {
        const uint64_t Numerator64[3] =
        {
            Numerator0, Numerator1, Numerator2
        };
        const uint64_t Divisor64[2] =
        {
            DivisorLow, DivisorHigh
        };
        uint64_t Quotient64[3]{};
        DivideSucceeded = Divide192By128_64(
            Numerator64, Divisor64, Quotient64);
        for (unsigned i = 0; i < 3; ++i)
        {
            Quotient[i * 2] = static_cast<uint32_t>(Quotient64[i]);
            Quotient[i * 2 + 1] =
                static_cast<uint32_t>(Quotient64[i] >> 32);
        }
    }
    else
    {
        DivideSucceeded =
            Divide192By128(Numerator, Divisor, Quotient, Remainder);
    }

    if (!DivideSucceeded)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    // The signed result must fit in 128 bits.
    // Quotient limbs 4 and 5 must be zero.
    if (Quotient[4] != 0 || Quotient[5] != 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    uint64_t QuotientLow =
        static_cast<uint64_t>(Quotient[0]) |
        (static_cast<uint64_t>(Quotient[1]) << 32);

    uint64_t QuotientHigh =
        static_cast<uint64_t>(Quotient[2]) |
        (static_cast<uint64_t>(Quotient[3]) << 32);

    // The signed minimum bit pattern is reserved for NaN.
    if ((QuotientHigh & SignMask) != 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    // Restore the sign using two's complement.
    if (resultNegative)
    {
        QuotientLow = ~QuotientLow + 1;
        QuotientHigh = ~QuotientHigh + (QuotientLow == 0);
    }

    // Reject the reserved NaN representation.
    if (QuotientHigh == SignMask && QuotientLow == 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }


    out.lowbits = QuotientLow;
    out.highbits = QuotientHigh;

    return out;
}


LongFloat::LongFloat(uint64_t highbits, uint64_t lowbits)
{
    this->lowbits = lowbits;
    this->highbits = highbits;
}

LongFloat::LongFloat(int32_t Value)
{
    lowbits = static_cast<uint64_t>(Value);
    highbits = Value < 0 ? UINT64_MAX : 0;

    //decimal shift by 10
    LongFloat Result = *this;
    for (int i = 0; i < DecimalPoint; ++i)
    {
        LongFloat A = Result;
        LongFloat B = Result;

        A.ShiftLeft(3);
        B.ShiftLeft(1);

        A += B;
        Result = A;
    }
    *this = Result;
}

LongFloat::LongFloat(int64_t Value)
{
    lowbits = static_cast<uint64_t>(Value);
    highbits = Value < 0 ? UINT64_MAX : 0;

    //decimal shift by 10
    LongFloat Result = *this;
    for (int i = 0; i < DecimalPoint; ++i)
    {
        LongFloat A = Result;
        LongFloat B = Result;

        A.ShiftLeft(3);
        B.ShiftLeft(1);

        A += B;
        Result = A;
    }
    *this = Result;
}

LongFloat::LongFloat(double Value)
{
    constexpr int64_t DecimalScale = LongFloat::ComputeScale();

    const int64_t Whole = static_cast<int64_t>(Value);
    const double Fraction = Value - static_cast<double>(Whole);
    const int64_t FractionScaled = static_cast<int64_t>(
        std::round(Fraction * DecimalScale)
        );

    LongFloat f = Whole;

    const uint64_t OldLow = f.lowbits;
    f.lowbits += static_cast<uint64_t>(FractionScaled);

    // Carry or borrow into the upper 64 bits.
    if (f.lowbits < OldLow)
        ++f.highbits;

    else if (FractionScaled < 0 && f.lowbits > OldLow)
        --f.highbits;

    *this = f;
}

LongFloat::LongFloat(uint64_t Value)
{
    lowbits = Value;
    highbits = Value < 0 ? UINT64_MAX : 0;

    for (int64_t i = 0; i < DecimalPoint; ++i)
    {
        uint64_t Carry;
        lowbits = LongFloatGlue::UnsignedMultiply128(lowbits, 10, &Carry);
        highbits = highbits * 10 + Carry;
    }
}

bool LongFloat::IsNegative() const
{
    return (highbits & 0x8000000000000000ul) != 0;
}
bool LongFloat::IsFractional() const
{
    constexpr uint64_t Scale = 1'000'000'000'000'000ULL;
    uint64_t maskedHbits = highbits & 0b0111111111111111111111111111111111111111111111111111111111111111ul;

    if (maskedHbits != 0)
        return false;
    // The low word now contains the complete value
    // for positive values, or the two's-complement low word for negative ones.
    return (lowbits % Scale) != 0;
}
bool LongFloat::IsNAN() const
{
    return highbits == 0x8000000000000000ULL && lowbits == 0;
}
void LongFloat::SetNAN()
{
    highbits = 0x8000000000000000ULL;
    lowbits = 0;
}
void LongFloat::Zero()
{
    highbits = 0;
    lowbits = 0;
}
void LongFloat::Negate()
{
    lowbits = ~lowbits + 1;
    highbits = ~highbits + (lowbits == 0);
}
int32_t LongFloat::Sign() const
{
    if (highbits == 0 && lowbits == 0)
        return 0;

    return IsNegative() ? -1 : 1;
}

#pragma region Aretmatic
LongFloat LongFloat::AddUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out{};
    unsigned char Carry = _addcarry_u64(
        0,
        lhs.lowbits,
        rhs.lowbits,
        &out.lowbits
    );
    auto carryFinal = _addcarry_u64(
        Carry,
        lhs.highbits,
        rhs.highbits,
        &out.highbits
    );
    return out;
}
LongFloat LongFloat::SubstractUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out{};
    unsigned char Borrow = _subborrow_u64(
        0,
        lhs.lowbits,
        rhs.lowbits,
        &out.lowbits
    );
    _subborrow_u64(
        Borrow,
        lhs.highbits,
        rhs.highbits,
        &out.highbits
    );
    return out;
}
LongFloat LongFloat::DivideUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    constexpr uint64_t SignMask = 0x8000000000000000ULL;
    auto ilhs = lhs;
    auto irhs = rhs;
    LongFloat out{};

    if (irhs.highbits == 0)
    {
        if (irhs.lowbits == 0)
        {
            out.SetNAN(); // divisor is zero
            return out;
        }
        if (ilhs.highbits == 0)
        {
            out.lowbits = ilhs.lowbits / irhs.lowbits;
            return out;
        }
    }

    // Determine result sign.
    const bool NegativeA = (ilhs.highbits & SignMask) != 0;
    const bool NegativeB = (irhs.highbits & SignMask) != 0;
    const bool NegativeResult = NegativeA != NegativeB;

    // Convert dividend to unsigned magnitude.
    uint64_t DividendLow = ilhs.lowbits;
    uint64_t DividendHigh = ilhs.highbits;

    if (NegativeA)
    {
        DividendLow = ~DividendLow + 1;
        DividendHigh = ~DividendHigh + (DividendLow == 0);
    }

    // Convert divisor to unsigned magnitude.
    uint64_t DivisorLow = irhs.lowbits;
    uint64_t DivisorHigh = irhs.highbits;

    if (NegativeB)
    {
        DivisorLow = ~DivisorLow + 1;
        DivisorHigh = ~DivisorHigh + (DivisorLow == 0);
    }

    uint64_t QuotientHigh = 0;
    uint64_t QuotientLow = 0;
    uint64_t Remainder = 0;

    if (DivisorHigh == 0)
    {
        // Two 64-bit divisions handle a 128-bit dividend / 64-bit divisor.
        QuotientHigh = DividendHigh / DivisorLow;

        QuotientLow = LongFloatGlue::UnsignedDivide128(
            DividendHigh % DivisorLow,
            DividendLow,
            DivisorLow,
            &Remainder
        );
    }
    else
    {
        // General 128-bit / 128-bit long division.
        // The quotient can contain at most 128 bits.
        uint64_t RemainderHigh = 0;
        uint64_t RemainderLow = 0;

        for (int i = 127; i >= 0; --i)
        {
            const uint64_t Bit = (i >= 64)
                ? ((DividendHigh >> (i - 64)) & 1ULL)
                : ((DividendLow >> i) & 1ULL);

            const bool Carry = (RemainderHigh >> 63) != 0;

            RemainderHigh = (RemainderHigh << 1) |
                (RemainderLow >> 63);

            RemainderLow = (RemainderLow << 1) | Bit;

            if (Carry ||
                RemainderHigh > DivisorHigh ||
                (RemainderHigh == DivisorHigh &&
                    RemainderLow >= DivisorLow))
            {
                const uint64_t OldLow = RemainderLow;

                RemainderLow -= DivisorLow;

                RemainderHigh -= DivisorHigh + (OldLow < DivisorLow);

                if (i >= 64)
                    QuotientHigh |= 1ULL << (i - 64);
                else
                    QuotientLow |= 1ULL << i;
            }
        }
    }

    // Restore two's-complement sign.
    if (NegativeResult)
    {
        QuotientLow = ~QuotientLow + 1;
        QuotientHigh = ~QuotientHigh + (QuotientLow == 0);
    }
    out.highbits = QuotientHigh;
    out.lowbits = QuotientLow;
    return out;
}
LongFloat LongFloat::MultiplayUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    auto ilhs = lhs;
    auto irhs = rhs;
    LongFloat out{};

    out.lowbits = LongFloatGlue::UnsignedMultiply128(ilhs.lowbits, irhs.lowbits, &out.highbits);

    out.highbits += ilhs.lowbits * irhs.highbits;
    out.highbits += ilhs.highbits * irhs.lowbits;
    return out;
}
LongFloat LongFloat::SquareRootUInt128(const LongFloat& Input)
{
    // Operates on nonnegative values. Exclude the high sign bit.
    const uint64_t inputHigh = Input.highbits & 0x7FFFFFFFFFFFFFFFULL;
    const uint64_t inputLow = Input.lowbits;

    unsigned long bit = 0;
    unsigned long index = 0;

    if (inputHigh != 0)
    {
        LongFloatGlue::BitScanReverse64(&index, inputHigh);
        bit = index + 64;
    }
    else if (inputLow != 0)
    {
        LongFloatGlue::BitScanReverse64(&index, inputLow);
        bit = index;
    }
    else
    {
        return LongFloat(0, 0);
    }

    // The samples bracket the integer root; high is exclusive.
    uint64_t low = SqrtSamples[bit + 1];
    uint64_t high = SqrtSamples[bit + 2] + 1;

    // Use linear interpolation to choose a starting point.
    LongFloat lowValueOfRoot(0, 1);
    lowValueOfRoot.ShiftLeft(bit);
    const LongFloat distanceLeft =
        SubstractUInt128(Input, lowValueOfRoot);
    const uint64_t rootSpan = high - low - 1;

    uint64_t lowProductHigh = 0;
    const uint64_t lowProduct = LongFloatGlue::UnsignedMultiply128(
        distanceLeft.lowbits, rootSpan, &lowProductHigh);

    uint64_t highProductHigh = 0;
    const uint64_t highProductLow = LongFloatGlue::UnsignedMultiply128(
        distanceLeft.highbits, rootSpan, &highProductHigh);

    const uint64_t middleProduct = lowProductHigh + highProductLow;
    const uint64_t upperProduct = highProductHigh +
        (middleProduct < lowProductHigh);

    uint64_t interpolatedOffset;
    if (bit == 0)
    {
        interpolatedOffset = lowProduct;
    }
    else if (bit < 64)
    {
        interpolatedOffset =
            (lowProduct >> bit) | (middleProduct << (64 - bit));
    }
    else if (bit == 64)
    {
        interpolatedOffset = middleProduct;
    }
    else
    {
        const unsigned shift = bit - 64;
        interpolatedOffset =
            (middleProduct >> shift) | (upperProduct << (64 - shift));
    }

    const uint64_t pivot = low + interpolatedOffset;

    uint64_t pivotSquareHigh = 0;
    const uint64_t pivotSquareLow =
        LongFloatGlue::UnsignedMultiply128(pivot, pivot, &pivotSquareHigh);

    if (pivotSquareHigh < inputHigh ||
        (pivotSquareHigh == inputHigh && pivotSquareLow <= inputLow))
    {
        low = pivot;

        // Bound the remaining interval using the interpolation error.
        const uint64_t interpolationErrorBound =
            (rootSpan + 19) / 20 + 2;
        const uint64_t narrowedHigh =
            pivot + interpolationErrorBound + 1;

        if (narrowedHigh < high)
            high = narrowedHigh;
    }
    else
    {
        high = pivot;
    }

    // Exact binary search for floor(sqrt(Input)).
    while (high - low > 1)
    {
        const uint64_t candidate = low + (high - low) / 2;

        uint64_t squareHigh = 0;
        const uint64_t squareLow =
            LongFloatGlue::UnsignedMultiply128(candidate, candidate, &squareHigh);

        if (squareHigh < inputHigh ||
            (squareHigh == inputHigh && squareLow <= inputLow))
        {
            low = candidate;
        }
        else
        {
            high = candidate;
        }
    }

    return LongFloat(0, low);
}

LongFloat LongFloat::Add(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out = AddUInt128(lhs, rhs);
    auto overflow = (lhs.highbits ^ out.highbits) & (rhs.highbits ^ out.highbits) & 0b1000000000000000000000000000000000000000000000000000000000000000ul;
    if (overflow)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
    }
    return out;
}
LongFloat LongFloat::Substract(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out = SubstractUInt128(lhs, rhs);
    auto overflow = (lhs.highbits ^ rhs.highbits) & (lhs.highbits ^ out.highbits) & 0b1000000000000000000000000000000000000000000000000000000000000000ul;
    if (overflow)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
    }
    return out;
}
LongFloat LongFloat::Divide(const LongFloat& lhs, const LongFloat& rhs)
{
#if 0
    constexpr uint64_t SignMask = 0x8000000000000000ULL;
    constexpr uint64_t Scale = ComputeScale();
    LongFloat out;
    // Division by zero.
    if (rhs.lowbits == 0 && rhs.highbits == 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    const bool lhsNegative = (lhs.highbits & SignMask) != 0;
    const bool rhsNegative = (rhs.highbits & SignMask) != 0;
    const bool resultNegative = lhsNegative != rhsNegative;

    // Convert operands to unsigned magnitudes.
    uint64_t DividendLow = lhs.lowbits;
    uint64_t DividendHigh = lhs.highbits;

    uint64_t DivisorLow = rhs.lowbits;
    uint64_t DivisorHigh = rhs.highbits;

    if (lhsNegative)
    {
        DividendLow = ~DividendLow + 1;
        DividendHigh = ~DividendHigh + (DividendLow == 0);
    }

    if (rhsNegative)
    {
        DivisorLow = ~DivisorLow + 1;
        DivisorHigh = ~DivisorHigh + (DivisorLow == 0);
    }

    // Construct abs(lhs) * Scale as a 192-bit numerator.
    uint64_t ProductHigh;
    const uint64_t ProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendLow, Scale, &ProductHigh);

    uint64_t UpperProductHigh;
    const uint64_t UpperProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendHigh, Scale, &UpperProductHigh);

    const uint64_t Numerator0 = ProductLow;

    const uint64_t Numerator1 = ProductHigh + UpperProductLow;
    const uint64_t Carry = Numerator1 < ProductHigh;

    const uint64_t Numerator2 = UpperProductHigh + Carry;

    // Convert numerator and divisor to base-2^32 limbs.
    const uint32_t Numerator[6] =
    {
        static_cast<uint32_t>(Numerator0),
        static_cast<uint32_t>(Numerator0 >> 32),
        static_cast<uint32_t>(Numerator1),
        static_cast<uint32_t>(Numerator1 >> 32),
        static_cast<uint32_t>(Numerator2),
        static_cast<uint32_t>(Numerator2 >> 32)
    };

    const uint32_t Divisor[4] =
    {
        static_cast<uint32_t>(DivisorLow),
        static_cast<uint32_t>(DivisorLow >> 32),
        static_cast<uint32_t>(DivisorHigh),
        static_cast<uint32_t>(DivisorHigh >> 32)
    };

    uint32_t Quotient[6]{};
    uint32_t Remainder[4]{};

    if (!Divide192By128(Numerator, Divisor, Quotient, Remainder))
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    // The signed result must fit in 128 bits.
    // Quotient limbs 4 and 5 must be zero.
    if (Quotient[4] != 0 || Quotient[5] != 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    uint64_t QuotientLow =
        static_cast<uint64_t>(Quotient[0]) |
        (static_cast<uint64_t>(Quotient[1]) << 32);

    uint64_t QuotientHigh =
        static_cast<uint64_t>(Quotient[2]) |
        (static_cast<uint64_t>(Quotient[3]) << 32);

    // The signed minimum bit pattern is reserved for NaN.
    if ((QuotientHigh & SignMask) != 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    // Restore the sign using two's complement.
    if (resultNegative)
    {
        QuotientLow = ~QuotientLow + 1;
        QuotientHigh = ~QuotientHigh + (QuotientLow == 0);
    }

    // Reject the reserved NaN representation.
    if (QuotientHigh == SignMask && QuotientLow == 0)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }


    out.lowbits = QuotientLow;
    out.highbits = QuotientHigh;

    return out;
#else
    return DivideWithAlgorithm<true>(lhs, rhs);
#endif // 0
}

LongFloat LongFloat::Multiplay(const LongFloat& lhs,const LongFloat& rhs) {
    constexpr uint64_t Scale = ComputeScale();

    // ---------------------------------------------------------
    // 1. Extract signs
    // ---------------------------------------------------------

    const bool lhsNegative = (lhs.highbits >> 63) != 0;
    const bool rhsNegative = (rhs.highbits >> 63) != 0;

    const bool negative = lhsNegative != rhsNegative;

    // ---------------------------------------------------------
    // 2. Load operands
    // ---------------------------------------------------------

    uint64_t aLow = lhs.lowbits;
    uint64_t aHigh = lhs.highbits;

    uint64_t bLow = rhs.lowbits;
    uint64_t bHigh = rhs.highbits;

    // ---------------------------------------------------------
    // 3. Convert signed operands to unsigned magnitudes
    // ---------------------------------------------------------

    if (lhsNegative)
    {
        aLow = ~aLow + 1;
        aHigh = ~aHigh + (aLow == 0 ? 1 : 0);
    }

    if (rhsNegative)
    {
        bLow = ~bLow + 1;
        bHigh = ~bHigh + (bLow == 0 ? 1 : 0);
    }

    // ---------------------------------------------------------
    // 4. Four 64 x 64-bit partial products
    // ---------------------------------------------------------

    uint64_t p00High;
    uint64_t p01High;
    uint64_t p10High;
    uint64_t p11High;

    const uint64_t p00Low =
        LongFloatGlue::UnsignedMultiply128(aLow, bLow, &p00High);

    const uint64_t p01Low =
        LongFloatGlue::UnsignedMultiply128(aLow, bHigh, &p01High);

    const uint64_t p10Low =
        LongFloatGlue::UnsignedMultiply128(aHigh, bLow, &p10High);

    const uint64_t p11Low =
        LongFloatGlue::UnsignedMultiply128(aHigh, bHigh, &p11High);

    // ---------------------------------------------------------
    // 5. Combine partial products
    //
    // This section corresponds conceptually to the LEA, ADD,
    // CMP, SETB and ADC instructions in your assembly.
    //
    // The carry assembly must be validated independently.
    // ---------------------------------------------------------

    uint64_t product[4];

    product[0] = p00Low;

    product[1] = p00High;
    product[1] += p01Low;

    const uint64_t carry1a = product[1] < p00High;

    const uint64_t temp1 = product[1];

    product[1] += p10Low;

    const uint64_t carry1b = product[1] < temp1;

    product[2] = p01High;
    product[2] += p10High;

    const uint64_t carry2a = product[2] < p01High;

    const uint64_t temp2 = product[2];

    product[2] += p11Low;

    const uint64_t carry2b = product[2] < temp2;

    const uint64_t carry1 = carry1a + carry1b;

    const uint64_t temp3 = product[2];

    product[2] += carry1;

    const uint64_t carry2c = product[2] < temp3;

    product[3] = p11High + carry2a + carry2b + carry2c;

    // ---------------------------------------------------------
    // 6. Fixed-point scaling
    //
    // Divide the 256-bit product by Scale.
    //
    // This corresponds to the repeated reciprocal-multiply,
    // shift, multiply-by-Scale and subtract instructions.
    // ---------------------------------------------------------

    uint64_t remainder = 0;

    for (int32_t i = 3; i >= 0; --i)
    {
        product[i] = LongFloatGlue::UnsignedDivide128(
            remainder,
            product[i],
            Scale,
            &remainder);
    }

    // ---------------------------------------------------------
    // 7. Keep the low 128 bits of the scaled result
    // ---------------------------------------------------------

    LongFloat out{};
    const auto overflow = product[2] | product[3] | (product[1] >> 63);
    if (overflow)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
        return out;
    }

    out.lowbits = product[0];
    out.highbits = product[1];

    // ---------------------------------------------------------
    // 8. Restore the sign
    // ---------------------------------------------------------

    if (negative)
    {
        out.lowbits = ~out.lowbits + 1;

        out.highbits =
            ~out.highbits + (out.lowbits == 0 ? 1 : 0);
    }
    return out;
}
LongFloat LongFloat::SquareRoot(const LongFloat& Input)
{
    constexpr uint64_t Scale = ComputeScale();

    if (Input.IsNegative())
    {
        LongFloat out{};
        out.SetNAN();
        return out;
    }

    if (Input.highbits == 0 && Input.lowbits == 0)
        return LongFloat(0, 0);

    LongFloat in = Input;
    uint64_t remainder = 0;
    in.highbits = LongFloatGlue::UnsignedDivide128(0, in.highbits, Scale, &remainder);
    in.lowbits = LongFloatGlue::UnsignedDivide128(remainder, in.lowbits, Scale, &remainder);

    // Inputs below one have no whole part; sqrt(double) retains enough
    // precision for the fractional input in this bounded range.
    if (in.highbits == 0 && in.lowbits == 0)
    {
        const double fractionalInput =
            static_cast<double>(Input.lowbits) / static_cast<double>(Scale);
        return LongFloat(std::sqrt(fractionalInput));
    }

    // Find k = floor(sqrt(floor(Input))).
    const LongFloat wholeRoot = SquareRootUInt128(in);
    const LongFloat scaleValue(0, Scale);
    const LongFloat wholeRootScaled =
        MultiplayUInt128(wholeRoot, scaleValue);

    // D = Input - k^2. The fractional root f satisfies
    //     f^2 + 2*k*f = D.
    const LongFloat wholeRootSquared =
        MultiplayUInt128(wholeRoot, wholeRoot);
    const LongFloat wholeRootSquaredScaled =
        MultiplayUInt128(wholeRootSquared, scaleValue);
    const LongFloat rootRemainder =
        SubstractUInt128(Input, wholeRootSquaredScaled);

    // Exact whole-root inputs need no fractional refinement.
    if (rootRemainder.highbits == 0 && rootRemainder.lowbits == 0)
        return wholeRootScaled;

    const uint64_t k = wholeRoot.lowbits;
    const double remainderRaw =
        std::ldexp(static_cast<double>(rootRemainder.highbits), 64) +
        static_cast<double>(rootRemainder.lowbits);
    const double remainderValue =
        remainderRaw / static_cast<double>(Scale);

    // Stable solution of f^2 + 2*k*f = remainderValue.
    const double kDouble = static_cast<double>(k);
    double fraction = remainderValue /
        (std::sqrt(kDouble * kDouble + remainderValue) + kDouble);

    if (fraction < 0.0)
        fraction = 0.0;
    if (fraction >= 1.0)
        fraction = 1.0;

    uint64_t fractionRaw = static_cast<uint64_t>(fraction * Scale);
    if (fractionRaw >= Scale)
        fractionRaw = Scale - 1;

    // Check a raw fractional candidate exactly.
    const auto FractionFits = [&](uint64_t f) -> bool
        {
            uint64_t squareQuotient;
            uint64_t squareRemainder;

            if constexpr (ComputeScale() <= UINT32_MAX)
            {
                const uint64_t square = f * f;
                squareQuotient = square / Scale;
                squareRemainder = square % Scale;
            }
            else
            {
                uint64_t squareHigh = 0;
                const uint64_t squareLow = LongFloatGlue::UnsignedMultiply128(f, f, &squareHigh);
                squareQuotient =
                    LongFloatGlue::UnsignedDivide128(squareHigh, squareLow, Scale, &squareRemainder);
            }

            uint64_t productHigh = 0;
            uint64_t productLow = LongFloatGlue::UnsignedMultiply128(2 * k, f, &productHigh);
            const uint64_t oldLow = productLow;
            productLow += squareQuotient;
            productHigh += (productLow < oldLow);

            if (productHigh != rootRemainder.highbits)
                return productHigh < rootRemainder.highbits;
            if (productLow != rootRemainder.lowbits)
                return productLow < rootRemainder.lowbits;
            return squareRemainder == 0;
        };

    // Adjust the estimate to the exact floor at LongFloat's scale.
    while (fractionRaw > 0 && !FractionFits(fractionRaw))
        --fractionRaw;
    while (fractionRaw + 1 < Scale && FractionFits(fractionRaw + 1))
        ++fractionRaw;

    const LongFloat fractionalRoot(0, fractionRaw);
    return AddUInt128(wholeRootScaled, fractionalRoot);
}
#pragma endregion
#pragma region Bitwise
void LongFloat::BitwiseAND(const LongFloat& Other)
{
    lowbits &= Other.lowbits;
    highbits &= Other.highbits;
}
void LongFloat::BitwiseOR(const LongFloat& Other)
{
    lowbits |= Other.lowbits;
    highbits |= Other.highbits;
}
void LongFloat::BitwiseXOR(const LongFloat& Other)
{
    lowbits ^= Other.lowbits;
    highbits ^= Other.highbits;
}
void LongFloat::BitwiseNOT()
{
    lowbits = ~lowbits;
    highbits = ~highbits;
}
void LongFloat::ShiftLeft(uint32_t Shift)
{
    if (Shift >= 128)
    {
        Zero();
    }
    else if (Shift >= 64)
    {
        highbits = lowbits << (Shift - 64);
        lowbits = 0;
    }
    else if (Shift != 0)
    {
        highbits = (highbits << Shift) |
            (lowbits >> (64 - Shift));

        lowbits <<= Shift;
    }
}
void LongFloat::ShiftRight(uint32_t Shift)
{
    if (Shift >= 128)
    {
        Zero();
    }
    else if (Shift >= 64)
    {
        lowbits = highbits >> (Shift - 64);
        highbits = 0;
    }
    else if (Shift != 0)
    {
        lowbits = (lowbits >> Shift) |
            (highbits << (64 - Shift));

        highbits >>= Shift;
    }
}
#pragma endregion

void LongFloat::PrintBits(bool CodeMode)
{
    union
    {
        uint32_t Bits[4];
        struct
        {
            uint64_t lb;
            uint64_t hb;
        };
    };
    hb = highbits;
    lb = lowbits;


    if (CodeMode)
    {
        std::cout << "0b";
    }
    for (size_t word = NumWords; word-- > 0;)
    {

        for (int bit = 31; bit >= 0; --bit)
        {
            std::cout << ((Bits[word] >> bit) & 1);
        }
        if (!CodeMode)
        {
            std::cout << ' ';
        }
    }

    std::cout << '\n';
}
std::string LongFloat::ToString() const
{
    // Detect the sign using the highest bit.
    const bool Negative = IsNegative();

    // Work on a copy so the original value remains unchanged.
    union
    {
        uint32_t Magnitude[4];
        struct
        {
            uint64_t lb;
            uint64_t hb;
        };
    };

    hb = highbits;
    lb = lowbits;

    // Convert negative two's-complement to unsigned magnitude.
    if (Negative)
    {
        for (int32_t i = 0; i < 4; ++i)
            Magnitude[i] = ~Magnitude[i];

        uint64_t Carry = 1;

        for (int32_t i = 0; i < 4 && Carry != 0; ++i)
        {
            const uint64_t Sum =
                static_cast<uint64_t>(Magnitude[i]) + Carry;

            Magnitude[i] = static_cast<uint32_t>(Sum);
            Carry = Sum >> 32;
        }
    }

    // Check whether the magnitude is zero.
    bool IsZero = true;

    for (int32_t i = 0; i < NumWords; ++i)
    {
        if (Magnitude[i] != 0)
        {
            IsZero = false;
            break;
        }
    }

    if (IsZero)
        return "0";

    // Convert the unsigned magnitude to decimal.
    std::string Result;
    Result.reserve(40);

    while (!IsZero)
    {
        uint64_t Remainder = 0;

        // Divide the entire 128-bit number by 10.
        for (int32_t i = NumWords - 1; i >= 0; --i)
        {
            const uint64_t Current =
                (Remainder << 32) | Magnitude[i];

            Magnitude[i] = static_cast<uint32_t>(Current / 10);
            Remainder = Current % 10;
        }

        // Each remainder is one decimal digit.
        Result += static_cast<char>('0' + Remainder);

        // Check whether the quotient is zero.
        IsZero = true;

        for (int32_t i = 0; i < NumWords; ++i)
        {
            if (Magnitude[i] != 0)
            {
                IsZero = false;
                break;
            }
        }
    }

    std::reverse(Result.begin(), Result.end());

    // Insert the virtual decimal point.
    if (DecimalPoint > 0)
    {
        const size_t Point = static_cast<size_t>(DecimalPoint);

        if (Result.size() <= Point)
            Result.insert(0, Point - Result.size() + 1, '0');

        Result.insert(Result.size() - Point, 1, '.');
    }

    // Restore the sign.
    if (Negative)
        Result.insert(Result.begin(), '-');

    if (IsNAN())
        return std::string("NaN { ") + Result + std::string(" } ");

    return Result;
}