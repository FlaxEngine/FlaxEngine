#include "Engine/Core/Math/Mathd.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/LongFloat.h"
#include "Engine/Core/Math/Vector3LongFloat.h"
#include "InstructionGlue.h"

constexpr uint64 LongFloat::SqrtSamples[128] =
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
static bool Divide192By128(const uint32 numerator[6], const uint32 divisor[4], uint32 quotient[6], uint32 remainder[4])
{
    constexpr uint64 B = 0x100000000ULL;

    uint32 u[7]{};
    uint32 v[4]{};

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

    uint64 carry = 0;

    for (int i = 0; i < divisorLength; ++i)
    {
        const uint64 value =
            (static_cast<uint64>(divisor[i]) << shift) | carry;

        v[i] = static_cast<uint32>(value);
        carry = value >> 32;
    }

    // Normalize the numerator, retaining the extra high limb.
    carry = 0;

    for (int i = 0; i < numeratorLength; ++i)
    {
        const uint64 value =
            (static_cast<uint64>(numerator[i]) << shift) | carry;

        u[i] = static_cast<uint32>(value);
        carry = value >> 32;
    }

    u[numeratorLength] = static_cast<uint32>(carry);

    const int n = divisorLength;
    const int m = numeratorLength - n;

    // Knuth-style normalized quotient estimation.
    for (int j = m; j >= 0; --j)
    {
        const uint64 top =
            (static_cast<uint64>(u[j + n]) << 32) |
            u[j + n - 1];

        uint64 qhat = top / v[n - 1];
        uint64 rhat = top % v[n - 1];

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
        uint64 productCarry = 0;

        for (int i = 0; i < n; ++i)
        {
            const uint64 product =
                qhat * v[i] + productCarry;

            productCarry = product >> 32;

            const uint32 productLow =
                static_cast<uint32>(product);

            const uint32 oldValue = u[j + i];

            u[j + i] = oldValue - productLow;

            if (oldValue < productLow)
                ++productCarry;
        }

        const uint32 oldTop = u[j + n];

        u[j + n] =
            oldTop - static_cast<uint32>(productCarry);

        const bool overestimated = oldTop < productCarry;

        // If the estimate was too large, decrement it and add v back.
        if (overestimated)
        {
            --qhat;

            uint64 additionCarry = 0;

            for (int i = 0; i < n; ++i)
            {
                const uint64 sum =
                    static_cast<uint64>(u[j + i]) +
                    v[i] +
                    additionCarry;

                u[j + i] = static_cast<uint32>(sum);
                additionCarry = sum >> 32;
            }

            u[j + n] += static_cast<uint32>(additionCarry);
        }

        quotient[j] = static_cast<uint32>(qhat);
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
uint64 Divide64Digit(
    uint64 u[4], const uint64 v[2], int j)
{
    uint64 qhat;
    uint64 rhat;
    bool rhatOverflow = false;
    if (u[j + 2] >= v[1])
    {
        qhat = MAX_uint64;
        rhat = u[j + 1] + v[1];
        rhatOverflow = rhat < u[j + 1];
    }
    else
    {
        qhat = LongFloatGlue::UnsignedDivide128(u[j + 2], u[j + 1], v[1], &rhat);
    }

    uint64 p0Low = 0;
    uint64 p0High = 0;
    for (;;)
    {
        p0Low = LongFloatGlue::UnsignedMultiply128(qhat, v[0], &p0High);
        if (rhatOverflow || p0High < rhat ||
            (p0High == rhat && p0Low <= u[j]))
            break;

        --qhat;
        const uint64 oldRhat = rhat;
        rhat += v[1];
        rhatOverflow = rhat < oldRhat;
    }

    uint64 p1High = 0;
    const uint64 p1Low = LongFloatGlue::UnsignedMultiply128(qhat, v[1], &p1High);
    const uint64 productMiddle0 = p0High + p1Low;
    const uint64 carry0 = productMiddle0 < p0High;
    const uint64 oldU0 = u[j];
    u[j] -= p0Low;
    const uint64 borrow0 = oldU0 < p0Low;

    const uint64 productMiddle = productMiddle0 + borrow0;
    const uint64 carry1 = productMiddle < productMiddle0;
    const uint64 productTop = p1High + carry0 + carry1;

    const uint64 oldU1 = u[j + 1];
    u[j + 1] -= productMiddle;
    const uint64 borrow1 = oldU1 < productMiddle;

    const uint64 oldU2 = u[j + 2];
    u[j + 2] -= productTop;
    const uint64 borrow2a = oldU2 < productTop;
    const uint64 beforeTopBorrow = u[j + 2];
    u[j + 2] -= borrow1;
    const bool overestimated = borrow2a || beforeTopBorrow < borrow1;

    if (overestimated)
    {
        --qhat;
        const uint64 oldLow = u[j];
        u[j] += v[0];
        const uint64 carry = u[j] < oldLow;
        const uint64 oldHigh = u[j + 1];
        u[j + 1] += v[1];
        const uint64 carryHigh0 = u[j + 1] < oldHigh;
        const uint64 beforeCarry = u[j + 1];
        u[j + 1] += carry;
        const uint64 carryHigh1 = u[j + 1] < beforeCarry;
        u[j + 2] += carryHigh0 | carryHigh1;
    }
    return qhat;
}

static bool Divide192By128_64(
    const uint64 numerator[3],
    const uint64 divisor[2],
    uint64 quotient[3])
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
        uint64 rem = 0;
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

    uint64 v[2];
    uint64 u[4]{};
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
    constexpr uint64 SignMask = 0x8000000000000000ULL;
    constexpr uint64 Scale = LongFloat::ComputeScale();
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
    uint64 DividendLow = lhs.lowbits;
    uint64 DividendHigh = lhs.highbits;

    uint64 DivisorLow = rhs.lowbits;
    uint64 DivisorHigh = rhs.highbits;

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
        uint64 NumeratorHigh = 0;
        const uint64 NumeratorLow =
            LongFloatGlue::UnsignedMultiply128(DividendLow, Scale, &NumeratorHigh);

        uint64 QuotientHigh = NumeratorHigh / DivisorLow;
        uint64 DivisionRemainder = NumeratorHigh % DivisorLow;
        uint64 QuotientLow = LongFloatGlue::UnsignedDivide128(
            DivisionRemainder,
            NumeratorLow,
            DivisorLow,
            &DivisionRemainder);

        // The minimum signed bit pattern is reserved for NaN.
        if ((QuotientHigh & SignMask) != 0)
        {
            return LongFloatMath::GetNAN();
        }

        if (resultNegative)
        {
            const uint64 MagnitudeLow = QuotientLow;
            const uint64 MagnitudeHigh = QuotientHigh;
            QuotientHigh = ~MagnitudeHigh + (MagnitudeLow == 0);
            QuotientLow = ~MagnitudeLow + 1;

            if (QuotientHigh == SignMask && QuotientLow == 0)
            {
                return LongFloatMath::GetNAN();
            }
        }

        out.highbits = QuotientHigh;
        out.lowbits = QuotientLow;
        return out;
    }

    // Construct abs(lhs) * Scale as a 192-bit numerator.
    uint64 ProductHigh;
    const uint64 ProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendLow, Scale, &ProductHigh);

    uint64 UpperProductHigh;
    const uint64 UpperProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendHigh, Scale, &UpperProductHigh);

    const uint64 Numerator0 = ProductLow;

    const uint64 Numerator1 = ProductHigh + UpperProductLow;
    const uint64 Carry = Numerator1 < ProductHigh;

    const uint64 Numerator2 = UpperProductHigh + Carry;

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
    const uint32 Numerator[6] =
    {
        static_cast<uint32>(Numerator0),
        static_cast<uint32>(Numerator0 >> 32),
        static_cast<uint32>(Numerator1),
        static_cast<uint32>(Numerator1 >> 32),
        static_cast<uint32>(Numerator2),
        static_cast<uint32>(Numerator2 >> 32)
    };

    const uint32 Divisor[4] =
    {
        static_cast<uint32>(DivisorLow),
        static_cast<uint32>(DivisorLow >> 32),
        static_cast<uint32>(DivisorHigh),
        static_cast<uint32>(DivisorHigh >> 32)
    };

    uint32 Quotient[6]{};
    uint32 Remainder[4]{};

    bool DivideSucceeded;
    if (Use64BitDivider)
    {
        const uint64 Numerator64[3] =
        {
            Numerator0, Numerator1, Numerator2
        };
        const uint64 Divisor64[2] =
        {
            DivisorLow, DivisorHigh
        };
        uint64 Quotient64[3]{};
        DivideSucceeded = Divide192By128_64(
            Numerator64, Divisor64, Quotient64);
        for (unsigned i = 0; i < 3; ++i)
        {
            Quotient[i * 2] = static_cast<uint32>(Quotient64[i]);
            Quotient[i * 2 + 1] =
                static_cast<uint32>(Quotient64[i] >> 32);
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

    uint64 QuotientLow =
        static_cast<uint64>(Quotient[0]) |
        (static_cast<uint64>(Quotient[1]) << 32);

    uint64 QuotientHigh =
        static_cast<uint64>(Quotient[2]) |
        (static_cast<uint64>(Quotient[3]) << 32);

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


LongFloat::LongFloat(uint64 highbits, uint64 lowbits)
{
    this->lowbits = lowbits;
    this->highbits = highbits;
}

LongFloat::LongFloat(int32 Value)
{
    lowbits = static_cast<uint64>(Value);
    highbits = Value < 0 ? MAX_uint64 : 0;

    //decimal shift by 10
    LongFloat Result = *this;
    for (int i = 0; i < DecimalPoint; ++i)
    {
        LongFloat A = Result;
        LongFloat B = Result;

        A = LongFloatMath::ShiftLeft(A, 3);
        B = LongFloatMath::ShiftLeft(B,1);

        A += B;
        Result = A;
    }
    *this = Result;
}

LongFloat::LongFloat(uint64 Value)
{
    lowbits = static_cast<uint64>(Value);
    highbits = Value < 0 ? MAX_uint64 : 0;

    //decimal shift by 10
    LongFloat Result = *this;
    for (int i = 0; i < DecimalPoint; ++i)
    {
        LongFloat A = Result;
        LongFloat B = Result;

        A = LongFloatMath::ShiftLeft(A,3);
        B = LongFloatMath::ShiftLeft(B,1);

        A += B;
        Result = A;
    }
    *this = Result;
}

LongFloat::LongFloat(double Value)
{
    constexpr uint64 DecimalScale = LongFloat::ComputeScale();

    const uint64 Whole = static_cast<uint64>(Value);
    const double Fraction = Value - static_cast<double>(Whole);
    const uint64 FractionScaled = static_cast<uint64>(
        Math::Round(Fraction * DecimalScale)
        );

    LongFloat f = Whole;

    const uint64 OldLow = f.lowbits;
    f.lowbits += static_cast<uint64>(FractionScaled);

    // Carry or borrow into the upper 64 bits.
    if (f.lowbits < OldLow)
        ++f.highbits;

    else if (FractionScaled < 0 && f.lowbits > OldLow)
        --f.highbits;

    *this = f;
}

bool LongFloatMath::IsNegative(const LongFloat& lhs)
{
    return (lhs.highbits & 0x8000000000000000ul) != 0;
}
bool LongFloatMath::IsFractional(const LongFloat& lhs)
{
    constexpr uint64 Scale = 1'000'000'000'000'000ULL;
    uint64 maskedHbits = lhs.highbits & 0b0111111111111111111111111111111111111111111111111111111111111111ul;

    if (maskedHbits != 0)
        return false;
    // The low word now contains the complete value
    // for positive values, or the two's-complement low word for negative ones.
    return (lhs.lowbits % Scale) != 0;
}
bool LongFloatMath::IsNAN(const LongFloat& lhs)
{
    return lhs.highbits == 0x8000000000000000ULL && lhs.lowbits == 0;
}


Vector3 LongFloatMath::Quantize(const Vector3LongFloat& value)
{
    return Float3(Quantize(value.X), Quantize(value.Y), Quantize(value.Z));
}

Float3 LongFloatMath::QuantizeToFloat3(const Vector3LongFloat& value)
{
    return Float3(QuantizeToFlaot(value.X), QuantizeToFlaot(value.Y), QuantizeToFlaot(value.Z));
}

Double3 LongFloatMath::QuantizeToDouble3(const Vector3LongFloat& value)
{
    return Double3(QuantizeToFlaot(value.X), QuantizeToFlaot(value.Y), QuantizeToFlaot(value.Z));
}
double LongFloatMath::QuantizeToDouble(const LongFloat& value)
{
    if (IsNAN(value))
        return NAN;

    const bool negative = IsNegative(value);
    uint64 high = value.highbits;
    uint64 low = value.lowbits;

    if (negative)
    {
        low = ~low + 1;
        high = ~high + (low == 0 ? 1 : 0);
    }

    constexpr double TwoTo64 = 18446744073709551616.0;
    const double raw =
        static_cast<double>(high) * TwoTo64 +
        static_cast<double>(low);

    constexpr double scale = static_cast<double>(LongFloat::ComputeScale());
    const double result = raw / scale;

    return negative ? -result : result;
}

LongFloat LongFloatMath::GetNAN()
{
    return LongFloat(0x8000000000000000ULL, 0);
}
LongFloat LongFloatMath::GetZero()
{
    return LongFloat(0, 0);
}
LongFloat LongFloatMath::Negate(const LongFloat& lhs)
{
    return LongFloat(~lhs.highbits + (lhs.lowbits == 0), ~lhs.lowbits + 1);
}
int32 LongFloatMath::Sign(const LongFloat& lhs)
{
    if (lhs.highbits == 0 && lhs.lowbits == 0)
        return 0;

    return IsNegative(lhs) ? -1 : 1;
}

#pragma region Aretmatic
LongFloat LongFloatMath::AddUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out{};
    unsigned char Carry = LongFloatGlue::AddCarryUnsignedInt64(
        0,
        lhs.lowbits,
        rhs.lowbits,
        &out.lowbits
    );
    auto carryFinal = LongFloatGlue::AddCarryUnsignedInt64(
        Carry,
        lhs.highbits,
        rhs.highbits,
        &out.highbits
    );
    return out;
}
LongFloat LongFloatMath::SubtractUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out{};
    unsigned char Borrow = LongFloatGlue::SubtractBorrowUnsignedInt64(
        0,
        lhs.lowbits,
        rhs.lowbits,
        &out.lowbits
    );
    LongFloatGlue::SubtractBorrowUnsignedInt64(
        Borrow,
        lhs.highbits,
        rhs.highbits,
        &out.highbits
    );
    return out;
}
LongFloat LongFloatMath::DivideUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    constexpr uint64 SignMask = 0x8000000000000000ULL;
    auto ilhs = lhs;
    auto irhs = rhs;
    LongFloat out{};

    if (irhs.highbits == 0)
    {
        if (irhs.lowbits == 0)
        {
            // divisor is zero
            return GetNAN();
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
    uint64 DividendLow = ilhs.lowbits;
    uint64 DividendHigh = ilhs.highbits;

    if (NegativeA)
    {
        DividendLow = ~DividendLow + 1;
        DividendHigh = ~DividendHigh + (DividendLow == 0);
    }

    // Convert divisor to unsigned magnitude.
    uint64 DivisorLow = irhs.lowbits;
    uint64 DivisorHigh = irhs.highbits;

    if (NegativeB)
    {
        DivisorLow = ~DivisorLow + 1;
        DivisorHigh = ~DivisorHigh + (DivisorLow == 0);
    }

    uint64 QuotientHigh = 0;
    uint64 QuotientLow = 0;
    uint64 Remainder = 0;

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
        uint64 RemainderHigh = 0;
        uint64 RemainderLow = 0;

        for (int i = 127; i >= 0; --i)
        {
            const uint64 Bit = (i >= 64)
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
                const uint64 OldLow = RemainderLow;

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
LongFloat LongFloatMath::MultiplyUInt128(const LongFloat& lhs, const LongFloat& rhs)
{
    auto ilhs = lhs;
    auto irhs = rhs;
    LongFloat out{};

    out.lowbits = LongFloatGlue::UnsignedMultiply128(ilhs.lowbits, irhs.lowbits, &out.highbits);

    out.highbits += ilhs.lowbits * irhs.highbits;
    out.highbits += ilhs.highbits * irhs.lowbits;
    return out;
}
LongFloat LongFloatMath::SquareRootUInt128(const LongFloat& Input)
{
    // Operates on nonnegative values. Exclude the high sign bit.
    const uint64 inputHigh = Input.highbits & 0x7FFFFFFFFFFFFFFFULL;
    const uint64 inputLow = Input.lowbits;

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
    uint64 low = LongFloat::SqrtSamples[bit + 1];
    uint64 high = LongFloat::SqrtSamples[bit + 2] + 1;

    // Use linear interpolation to choose a starting point.
    LongFloat lowValueOfRoot(0, 1);
    lowValueOfRoot = LongFloatMath::ShiftLeft(lowValueOfRoot,bit);
    const LongFloat distanceLeft =
        LongFloatMath::SubtractUInt128(Input, lowValueOfRoot);
    const uint64 rootSpan = high - low - 1;

    uint64 lowProductHigh = 0;
    const uint64 lowProduct = LongFloatGlue::UnsignedMultiply128(
        distanceLeft.lowbits, rootSpan, &lowProductHigh);

    uint64 highProductHigh = 0;
    const uint64 highProductLow = LongFloatGlue::UnsignedMultiply128(
        distanceLeft.highbits, rootSpan, &highProductHigh);

    const uint64 middleProduct = lowProductHigh + highProductLow;
    const uint64 upperProduct = highProductHigh +
        (middleProduct < lowProductHigh);

    uint64 interpolatedOffset;
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

    const uint64 pivot = low + interpolatedOffset;

    uint64 pivotSquareHigh = 0;
    const uint64 pivotSquareLow =
        LongFloatGlue::UnsignedMultiply128(pivot, pivot, &pivotSquareHigh);

    if (pivotSquareHigh < inputHigh ||
        (pivotSquareHigh == inputHigh && pivotSquareLow <= inputLow))
    {
        low = pivot;

        // Bound the remaining interval using the interpolation error.
        const uint64 interpolationErrorBound =
            (rootSpan + 19) / 20 + 2;
        const uint64 narrowedHigh =
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
        const uint64 candidate = low + (high - low) / 2;

        uint64 squareHigh = 0;
        const uint64 squareLow =
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

LongFloat  LongFloatMath::Add(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out = LongFloatMath::AddUInt128(lhs, rhs);
    auto overflow = (lhs.highbits ^ out.highbits) & (rhs.highbits ^ out.highbits) & 0b1000000000000000000000000000000000000000000000000000000000000000ul;
    if (overflow)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
    }
    return out;
}
LongFloat LongFloatMath::Subtract(const LongFloat& lhs, const LongFloat& rhs)
{
    LongFloat out = LongFloatMath::SubtractUInt128(lhs, rhs);
    auto overflow = (lhs.highbits ^ rhs.highbits) & (lhs.highbits ^ out.highbits) & 0b1000000000000000000000000000000000000000000000000000000000000000ul;
    if (overflow)
    {
        //set to NAN
        out.highbits = 0x8000000000000000ULL;
        out.lowbits = 0;
    }
    return out;
}
LongFloat LongFloatMath::Divide(const LongFloat& lhs, const LongFloat& rhs)
{
#if 0
    constexpr uint64 SignMask = 0x8000000000000000ULL;
    constexpr uint64 Scale = ComputeScale();
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
    uint64 DividendLow = lhs.lowbits;
    uint64 DividendHigh = lhs.highbits;

    uint64 DivisorLow = rhs.lowbits;
    uint64 DivisorHigh = rhs.highbits;

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
    uint64 ProductHigh;
    const uint64 ProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendLow, Scale, &ProductHigh);

    uint64 UpperProductHigh;
    const uint64 UpperProductLow =
        LongFloatGlue::UnsignedMultiply128(DividendHigh, Scale, &UpperProductHigh);

    const uint64 Numerator0 = ProductLow;

    const uint64 Numerator1 = ProductHigh + UpperProductLow;
    const uint64 Carry = Numerator1 < ProductHigh;

    const uint64 Numerator2 = UpperProductHigh + Carry;

    // Convert numerator and divisor to base-2^32 limbs.
    const uint32 Numerator[6] =
    {
        static_cast<uint32>(Numerator0),
        static_cast<uint32>(Numerator0 >> 32),
        static_cast<uint32>(Numerator1),
        static_cast<uint32>(Numerator1 >> 32),
        static_cast<uint32>(Numerator2),
        static_cast<uint32>(Numerator2 >> 32)
    };

    const uint32 Divisor[4] =
    {
        static_cast<uint32>(DivisorLow),
        static_cast<uint32>(DivisorLow >> 32),
        static_cast<uint32>(DivisorHigh),
        static_cast<uint32>(DivisorHigh >> 32)
    };

    uint32 Quotient[6]{};
    uint32 Remainder[4]{};

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

    uint64 QuotientLow =
        static_cast<uint64>(Quotient[0]) |
        (static_cast<uint64>(Quotient[1]) << 32);

    uint64 QuotientHigh =
        static_cast<uint64>(Quotient[2]) |
        (static_cast<uint64>(Quotient[3]) << 32);

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

LongFloat LongFloatMath::Multiply(const LongFloat& lhs,const LongFloat& rhs) {
    constexpr uint64 Scale = LongFloat::ComputeScale();

    // ---------------------------------------------------------
    // 1. Extract signs
    // ---------------------------------------------------------

    const bool lhsNegative = (lhs.highbits >> 63) != 0;
    const bool rhsNegative = (rhs.highbits >> 63) != 0;

    const bool negative = lhsNegative != rhsNegative;

    // ---------------------------------------------------------
    // 2. Load operands
    // ---------------------------------------------------------

    uint64 aLow = lhs.lowbits;
    uint64 aHigh = lhs.highbits;

    uint64 bLow = rhs.lowbits;
    uint64 bHigh = rhs.highbits;

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

    uint64 p00High;
    uint64 p01High;
    uint64 p10High;
    uint64 p11High;

    const uint64 p00Low =
        LongFloatGlue::UnsignedMultiply128(aLow, bLow, &p00High);

    const uint64 p01Low =
        LongFloatGlue::UnsignedMultiply128(aLow, bHigh, &p01High);

    const uint64 p10Low =
        LongFloatGlue::UnsignedMultiply128(aHigh, bLow, &p10High);

    const uint64 p11Low =
        LongFloatGlue::UnsignedMultiply128(aHigh, bHigh, &p11High);

    // ---------------------------------------------------------
    // 5. Combine partial products
    //
    // This section corresponds conceptually to the LEA, ADD,
    // CMP, SETB and ADC instructions in your assembly.
    //
    // The carry assembly must be validated independently.
    // ---------------------------------------------------------

    uint64 product[4];

    product[0] = p00Low;

    product[1] = p00High;
    product[1] += p01Low;

    const uint64 carry1a = product[1] < p00High;

    const uint64 temp1 = product[1];

    product[1] += p10Low;

    const uint64 carry1b = product[1] < temp1;

    product[2] = p01High;
    product[2] += p10High;

    const uint64 carry2a = product[2] < p01High;

    const uint64 temp2 = product[2];

    product[2] += p11Low;

    const uint64 carry2b = product[2] < temp2;

    const uint64 carry1 = carry1a + carry1b;

    const uint64 temp3 = product[2];

    product[2] += carry1;

    const uint64 carry2c = product[2] < temp3;

    product[3] = p11High + carry2a + carry2b + carry2c;

    // ---------------------------------------------------------
    // 6. Fixed-point scaling
    //
    // Divide the 256-bit product by Scale.
    //
    // This corresponds to the repeated reciprocal-multiply,
    // shift, multiply-by-Scale and subtract instructions.
    // ---------------------------------------------------------

    uint64 remainder = 0;

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
LongFloat LongFloatMath::SquareRoot(const LongFloat& Input)
{
    constexpr uint64 Scale = LongFloat::ComputeScale();

    if (LongFloatMath::IsNegative(Input))
    {
        return LongFloatMath::GetNAN();
    }

    if (Input.highbits == 0 && Input.lowbits == 0)
        return LongFloat(0, 0);

    LongFloat in = Input;
    uint64 remainder = 0;
    in.highbits = LongFloatGlue::UnsignedDivide128(0, in.highbits, Scale, &remainder);
    in.lowbits = LongFloatGlue::UnsignedDivide128(remainder, in.lowbits, Scale, &remainder);

    // Inputs below one have no whole part; sqrt(double) retains enough
    // precision for the fractional input in this bounded range.
    if (in.highbits == 0 && in.lowbits == 0)
    {
        const double fractionalInput =
            static_cast<double>(Input.lowbits) / static_cast<double>(Scale);
        return LongFloat(Math::Sqrt(fractionalInput));
    }

    // Find k = floor(sqrt(floor(Input))).
    const LongFloat wholeRoot = LongFloatMath::SquareRootUInt128(in);
    const LongFloat scaleValue(0, Scale);
    const LongFloat wholeRootScaled =
        LongFloatMath::MultiplyUInt128(wholeRoot, scaleValue);

    // D = Input - k^2. The fractional root f satisfies
    //     f^2 + 2*k*f = D.
    const LongFloat wholeRootSquared =
        LongFloatMath::MultiplyUInt128(wholeRoot, wholeRoot);
    const LongFloat wholeRootSquaredScaled =
        LongFloatMath::MultiplyUInt128(wholeRootSquared, scaleValue);
    const LongFloat rootRemainder =
        LongFloatMath::SubtractUInt128(Input, wholeRootSquaredScaled);

    // Exact whole-root inputs need no fractional refinement.
    if (rootRemainder.highbits == 0 && rootRemainder.lowbits == 0)
        return wholeRootScaled;

    const uint64 k = wholeRoot.lowbits;
    const double remainderRaw =
        static_cast<double>(rootRemainder.highbits) * Math::Exp2(64.0) +
        static_cast<double>(rootRemainder.lowbits);
    const double remainderValue =
        remainderRaw / static_cast<double>(Scale);

    // Stable solution of f^2 + 2*k*f = remainderValue.
    const double kDouble = static_cast<double>(k);
    double fraction = remainderValue /
        (Math::Sqrt(kDouble * kDouble + remainderValue) + kDouble);

    if (fraction < 0.0)
        fraction = 0.0;
    if (fraction >= 1.0)
        fraction = 1.0;

    uint64 fractionRaw = static_cast<uint64>(fraction * Scale);
    if (fractionRaw >= Scale)
        fractionRaw = Scale - 1;

    // Check a raw fractional candidate exactly.
    const auto FractionFits = [&](uint64 f) -> bool
        {
            uint64 squareQuotient;
            uint64 squareRemainder;

            if (LongFloat::ComputeScale() <= MAX_uint32)
            {
                const uint64 square = f * f;
                squareQuotient = square / Scale;
                squareRemainder = square % Scale;
            }
            else
            {
                uint64 squareHigh = 0;
                const uint64 squareLow = LongFloatGlue::UnsignedMultiply128(f, f, &squareHigh);
                squareQuotient =
                    LongFloatGlue::UnsignedDivide128(squareHigh, squareLow, Scale, &squareRemainder);
            }

            uint64 productHigh = 0;
            uint64 productLow = LongFloatGlue::UnsignedMultiply128(2 * k, f, &productHigh);
            const uint64 oldLow = productLow;
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
    return LongFloatMath::AddUInt128(wholeRootScaled, fractionalRoot);
}
#pragma endregion
#pragma region Bitwise
LongFloat LongFloatMath::BitwiseAND(const LongFloat& lhs, const LongFloat& rhs)
{
    return LongFloat(lhs.highbits & rhs.highbits, lhs.lowbits & rhs.lowbits);
}

LongFloat LongFloatMath::BitwiseOR(const LongFloat& lhs, const LongFloat& rhs)
{
    return LongFloat(lhs.highbits | rhs.highbits, lhs.lowbits | rhs.lowbits);
}

LongFloat LongFloatMath::BitwiseXOR(const LongFloat& lhs, const LongFloat& rhs)
{
    return LongFloat(lhs.highbits ^ rhs.highbits, lhs.lowbits ^ rhs.lowbits);
}

LongFloat LongFloatMath::BitwiseNOT(const LongFloat& value)
{
    return LongFloat(~value.highbits, ~value.lowbits);
}

LongFloat LongFloatMath::ShiftLeft(const LongFloat& lhs, uint32 Shift)
{
    LongFloat result = lhs;

    if (Shift >= 128)
    {
        result = LongFloatMath::GetZero();
    }
    else if (Shift >= 64)
    {
        result.highbits = result.lowbits << (Shift - 64);
        result.lowbits = 0;
    }
    else if (Shift != 0)
    {
        result.highbits = (result.highbits << Shift) |
            (result.lowbits >> (64 - Shift));
        result.lowbits <<= Shift;
    }

    return result;
}

LongFloat LongFloatMath::ShiftRight(const LongFloat& lhs, uint32 Shift)
{
    LongFloat result = lhs;

    if (Shift >= 128)
    {
        result = LongFloatMath::GetZero();
    }
    else if (Shift >= 64)
    {
        result.lowbits = result.highbits >> (Shift - 64);
        result.highbits = 0;
    }
    else if (Shift != 0)
    {
        result.lowbits = (result.lowbits >> Shift) |
            (result.highbits << (64 - Shift));
        result.highbits >>= Shift;
    }

    return result;
}
#pragma endregion

String LongFloatMath::BitsToString(const LongFloat& lhs,bool CodeMode)
{
    union
    {
        uint32 Bits[4];
        struct
        {
            uint64 lb;
            uint64 hb;
        };
    };

    hb = lhs.highbits;
    lb = lhs.lowbits;

    String Result;
    if (CodeMode)
        Result = "0b";

    for (size_t word = LongFloat::NumWords; word-- > 0;)
    {
        for (int bit = 31; bit >= 0; --bit)
        {
            const Char digit = static_cast<Char>(
                '0' + ((Bits[word] >> bit) & 1));
            Result += digit;
        }

        if (!CodeMode)
            Result += static_cast<Char>(' ');
    }

    Result += static_cast<Char>('\n');
    return Result;
}

String LongFloatMath::ToString(const LongFloat& lhs)
{
    // Detect the sign using the highest bit.
    const bool Negative = LongFloatMath::IsNegative(lhs);

    // Work on a copy so the original value remains unchanged.
    union
    {
        uint32 Magnitude[4];
        struct
        {
            uint64 lb;
            uint64 hb;
        };
    };

    hb = lhs.highbits;
    lb = lhs.lowbits;

    // Convert negative two's-complement to unsigned magnitude.
    if (Negative)
    {
        for (int32_t i = 0; i < 4; ++i)
            Magnitude[i] = ~Magnitude[i];

        uint64 Carry = 1;

        for (int32_t i = 0; i < 4 && Carry != 0; ++i)
        {
            const uint64 Sum =
                static_cast<uint64>(Magnitude[i]) + Carry;

            Magnitude[i] = static_cast<uint32>(Sum);
            Carry = Sum >> 32;
        }
    }

    // Check whether the magnitude is zero.
    bool IsZero = true;

    for (int32_t i = 0; i < LongFloat::NumWords; ++i)
    {
        if (Magnitude[i] != 0)
        {
            IsZero = false;
            break;
        }
    }

    if (IsZero)
        return String("0");

    // Convert the unsigned magnitude to decimal.
    Char ResultBuffer[40];
    int32 ResultLength = 0;

    while (!IsZero)
    {
        uint64 Remainder = 0;

        // Divide the entire 128-bit number by 10.
        for (int32_t i = LongFloat::NumWords - 1; i >= 0; --i)
        {
            const uint64 Current =
                (Remainder << 32) | Magnitude[i];

            Magnitude[i] = static_cast<uint32>(Current / 10);
            Remainder = Current % 10;
        }

        // Each remainder is one decimal digit.
        ResultBuffer[ResultLength++] = static_cast<Char>('0' + Remainder);

        // Check whether the quotient is zero.
        IsZero = true;

        for (int32_t i = 0; i < LongFloat::NumWords; ++i)
        {
            if (Magnitude[i] != 0)
            {
                IsZero = false;
                break;
            }
        }
    }

    for (int32 i = 0; i < ResultLength / 2; ++i)
    {
        const Char temp = ResultBuffer[i];
        ResultBuffer[i] = ResultBuffer[ResultLength - i - 1];
        ResultBuffer[ResultLength - i - 1] = temp;
    }
    String Result(ResultBuffer, ResultLength);

    // Insert the virtual decimal point.
    if (LongFloat::DecimalPoint > 0)
    {
        const size_t Point = static_cast<size_t>(LongFloat::DecimalPoint);

        if (Result.Length() <= Point)
        {
            for (int32 i = Result.Length(); i <= Point; ++i)
                Result.Insert(0, String("0"));
        }

        Result.Insert(Result.Length() - Point, String("."));
    }

    // Restore the sign.
    if (Negative)
        Result.Insert(0, String("-"));

    if (LongFloatMath::IsNAN(lhs))
        return String("NaN { ") + Result + String(" } ");

    return Result;
}
