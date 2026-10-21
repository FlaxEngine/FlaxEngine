#pragma once
#include <Engine/Core/Types/BaseTypes.h>
#include <Engine/Core/Types/String.h>

struct LongFloat;

/// <summary>
/// Provides static helper functions for LongFloat operations.
/// </summary>
API_CLASS(Static, Namespace = "FlaxEngine.Utilities")
class FLAXENGINE_API LongFloatMath
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LongFloatMath);

    // do to limitations of the bindings
    // this crap is needed, put it back small functions to the struct when structs will support funcions finaly
public:

    /// <summary>
    /// Checks whether a value is negative.
    /// </summary>
    /// <param name="lhs">The value to check.</param>
    /// <returns>True if the value is negative; otherwise, false.</returns>
    [[nodiscard]] API_FUNCTION() static bool IsNegative(const LongFloat& lhs);
    
    /// <summary>
    /// Checks whether a value has a nonzero fractional part.
    /// </summary>
    /// <param name="lhs">The value to check.</param>
    /// <returns>True if the value has a nonzero fractional part; otherwise, false.</returns>
    [[nodiscard]] API_FUNCTION() static bool IsFractional(const LongFloat& lhs);
    
    /// <summary>
    /// Gets the sign of a value.
    /// </summary>
    /// <param name="lhs">The value to inspect.</param>
    /// <returns>-1 for a negative value, 0 for zero, or 1 for a positive value.</returns>
    [[nodiscard]] API_FUNCTION() static int32 Sign(const LongFloat& lhs);
    
    /// <summary>
    /// Checks whether a value is NaN.
    /// </summary>
    /// <param name="lhs">The value to check.</param>
    /// <returns>True if the value is NaN; otherwise, false.</returns>
    [[nodiscard]] API_FUNCTION() static bool IsNAN(const LongFloat& lhs);
    
    /// <summary>
    /// Gets the NaN value.
    /// </summary>
    /// <returns>The NaN value.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat GetNAN();
    
    /// <summary>
    /// Gets the zero value.
    /// </summary>
    /// <returns>A zero value.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat GetZero();
    
    /// <summary>
    /// Negates a value in place.
    /// </summary>
    /// <param name="lhs">The value to negate.</param>
    API_FUNCTION() static void Negate(LongFloat& lhs);

    /// <summary>
    /// Adds two values.
    /// </summary>
    /// <param name="lhs">The first value.</param>
    /// <param name="rhs">The second value.</param>
    /// <returns>The sum.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat Add(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Subtracts the second value from the first.
    /// </summary>
    /// <param name="lhs">The value to subtract from.</param>
    /// <param name="rhs">The value to subtract.</param>
    /// <returns>The difference.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat Subtract(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Divides the first value by the second.
    /// </summary>
    /// <param name="lhs">The dividend.</param>
    /// <param name="rhs">The divisor.</param>
    /// <returns>The quotient.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat Divide(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Multiplies two values.
    /// </summary>
    /// <param name="lhs">The first value.</param>
    /// <param name="rhs">The second value.</param>
    /// <returns>The product.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat Multiply(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Calculates the square root of a value.
    /// </summary>
    /// <param name="Input">The value whose square root is calculated.</param>
    /// <returns>The square root.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat SquareRoot(const LongFloat& Input);

    /// <summary>
    /// Adds two raw unsigned 128-bit values without fixed-point scaling.
    /// </summary>
    /// <param name="lhs">The first raw value.</param>
    /// <param name="rhs">The second raw value.</param>
    /// <returns>The raw sum.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat AddUInt128(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Subtracts the second raw unsigned 128-bit value from the first without fixed-point scaling.
    /// </summary>
    /// <param name="lhs">The raw value to subtract from.</param>
    /// <param name="rhs">The raw value to subtract.</param>
    /// <returns>The raw difference.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat SubtractUInt128(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Divides raw unsigned 128-bit values without fixed-point scaling.
    /// </summary>
    /// <param name="lhs">The dividend.</param>
    /// <param name="rhs">The divisor.</param>
    /// <returns>The raw quotient.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat DivideUInt128(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Multiplies raw unsigned 128-bit values without fixed-point scaling.
    /// </summary>
    /// <param name="lhs">The first raw value.</param>
    /// <param name="rhs">The second raw value.</param>
    /// <returns>The raw product.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat MultiplyUInt128(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Calculates the square root of a raw unsigned 128-bit value.
    /// </summary>
    /// <param name="Input">The raw value whose square root is calculated.</param>
    /// <returns>The raw square root.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat SquareRootUInt128(const LongFloat& Input);

    /// <summary>
    /// Performs a bitwise AND on the raw bits of two values.
    /// </summary>
    /// <param name="lhs">The first value.</param>
    /// <param name="rhs">The second value.</param>
    /// <returns>The result of the bitwise AND.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat BitwiseAND(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Performs a bitwise OR on the raw bits of two values.
    /// </summary>
    /// <param name="lhs">The first value.</param>
    /// <param name="rhs">The second value.</param>
    /// <returns>The result of the bitwise OR.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat BitwiseOR(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Performs a bitwise XOR on the raw bits of two values.
    /// </summary>
    /// <param name="lhs">The first value.</param>
    /// <param name="rhs">The second value.</param>
    /// <returns>The result of the bitwise XOR.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat BitwiseXOR(const LongFloat& lhs, const LongFloat& rhs);
    
    /// <summary>
    /// Inverts every bit in the raw value.
    /// </summary>
    /// <param name="lhs">The value whose bits are inverted.</param>
    /// <returns>The bitwise complement.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat BitwiseNOT(const LongFloat& lhs);
    
    /// <summary>
    /// Shifts the raw bits to the left.
    /// </summary>
    /// <param name="lhs">The value to shift.</param>
    /// <param name="Shift">The number of bit positions to shift.</param>
    /// <returns>The shifted value.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat ShiftLeft(const LongFloat& lhs, uint32 Shift);
    
    /// <summary>
    /// Shifts the raw bits to the right.
    /// </summary>
    /// <param name="lhs">The value to shift.</param>
    /// <param name="Shift">The number of bit positions to shift.</param>
    /// <returns>The shifted value.</returns>
    [[nodiscard]] API_FUNCTION() static LongFloat ShiftRight(const LongFloat& lhs, uint32 Shift);

    /// <summary>
    /// Converts the raw bits to a string of binary digits.
    /// </summary>
    /// <param name="lhs">The value to convert.</param>
    /// <param name="CodeMode">True to prefix the result with the binary literal prefix.</param>
    /// <returns>The binary representation of the raw bits.</returns>
    [[nodiscard]] API_FUNCTION() static String BitsToString(const LongFloat& lhs, bool CodeMode = false);
    
    /// <summary>
    /// Converts a value to its decimal string representation.
    /// </summary>
    /// <param name="lhs">The value to convert.</param>
    /// <returns>The decimal representation of the value.</returns>
    [[nodiscard]] API_FUNCTION() static String ToString(const LongFloat& lhs);
};

/// <summary>
/// Fixed-point floating-point number.
/// </summary>
API_STRUCT() struct FLAXENGINE_API LongFloat
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LongFloat);

    static const uint64 SqrtSamples[128];
    constexpr static int64 NumWords = 4;
    //maybe add a compile setting for it ?
    constexpr static int64 DecimalPoint = 15;

    /// <summary>
    /// The low 64 bits of the signed fixed-point value.
    /// </summary>
    API_FIELD() uint64 lowbits;
    /// <summary>
    /// The high 64 bits of the signed fixed-point value.
    /// </summary>
    API_FIELD() uint64 highbits;

public:
    constexpr static int64 ComputeScale()
    {
        int64 Scale = 1;

        for (int i = 0; i < DecimalPoint; ++i)
            Scale *= 10;

        return Scale;
    }
public:
    LongFloat() = default;
    LongFloat(uint64 highbits, uint64 lowbits);
    LongFloat(int32 Value);
    LongFloat(int64 Value);
    LongFloat(double Value);
    LongFloat(uint64 Value);
public:
    [[nodiscard]] bool operator==(const LongFloat& Other) const
    {
        return lowbits == Other.lowbits &&
            highbits == Other.highbits;
    }
    [[nodiscard]] bool operator!=(const LongFloat& Other) const
    {
        return !(*this == Other);
    }
    [[nodiscard]] bool operator<(const LongFloat& Other) const
    {
        const bool NegativeA = LongFloatMath::IsNegative(*this);
        const bool NegativeB = LongFloatMath::IsNegative(Other);

        if (NegativeA != NegativeB)
            return NegativeA;

        if (highbits != Other.highbits)
            return NegativeA
            ? highbits > Other.highbits
            : highbits < Other.highbits;

        return lowbits < Other.lowbits;
    }
    [[nodiscard]] bool operator>(const LongFloat& Other) const
    {
        return Other < *this;
    }

    [[nodiscard]] bool operator<=(const LongFloat& Other) const
    {
        return !(Other < *this);
    }
    [[nodiscard]] bool operator>=(const LongFloat& Other) const
    {
        return !(*this < Other);
    }
    void operator+=(const LongFloat& Other)
    {
        (*this) = LongFloatMath::Add(*this, Other);
    }
    void operator-=(const LongFloat& Other)
    {
        (*this) = LongFloatMath::Subtract(*this, Other);
    }
    void operator*=(const LongFloat& Other)
    {
        (*this) = LongFloatMath::Multiply(*this, Other);
    }
    void operator/=(const LongFloat& Other)
    {
        (*this) = LongFloatMath::Divide(*this, Other);
    }

    [[nodiscard]] LongFloat operator+(const LongFloat& Other)
    {
        return LongFloatMath::Add(*this, Other);
    }
    [[nodiscard]] LongFloat operator-(const LongFloat& Other)
    {
        return LongFloatMath::Subtract(*this, Other);
    }
    [[nodiscard]] LongFloat operator*(const LongFloat& Other)
    {
        return LongFloatMath::Multiply(*this, Other);
    }
    [[nodiscard]] LongFloat operator/(const LongFloat& Other)
    {
        return LongFloatMath::Divide(*this, Other);
    }
};
