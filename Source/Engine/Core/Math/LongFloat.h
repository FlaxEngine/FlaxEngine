#pragma once
#include <cstdint>
#include <string>


/// <summary>
/// Fixed-point floating-point number.
/// </summary>
struct LongFloat
{
    constexpr static int64_t NumWords = 4;
    constexpr static int64_t DecimalPoint = 6;

    constexpr static int64_t ComputeScale()
    {
        int64_t Scale = 1;

        for (int i = 0; i < DecimalPoint; ++i)
            Scale *= 10;

        return Scale;
    }

    static const uint64_t SqrtSamples[128];
    uint64_t lowbits;
    uint64_t highbits;

    static void FromUInt64Unscaled(LongFloat& ref,uint64_t Value)
    {
        ref.lowbits = Value;
        ref.highbits = Value < 0 ? UINT64_MAX : 0;
    }
public:
    LongFloat() = default;
    LongFloat(uint64_t highbits, uint64_t lowbits);
    LongFloat(int32_t Value);
    LongFloat(int64_t Value);
    LongFloat(double Value);
    LongFloat(uint64_t Value);

    [[nodiscard]] bool IsNegative() const;
    [[nodiscard]] bool IsFractional() const;
    [[nodiscard]] int32_t Sign() const;
    [[nodiscard]] bool IsNAN() const;
    void SetNAN();
    void Zero();
    void Negate();
public:
    [[nodiscard]] static LongFloat Add(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat Substract(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat Divide(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat Multiplay(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat SquareRoot(const LongFloat& Input);

    //treets values as UInt128 / UInt128 dose not care abaut overflow
    [[nodiscard]] static LongFloat AddUInt128(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat SubstractUInt128(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat DivideUInt128(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat MultiplayUInt128(const LongFloat& lhs, const LongFloat& rhs);
    [[nodiscard]] static LongFloat SquareRootUInt128(const LongFloat& Input);
public:
    void BitwiseAND(const LongFloat& Other);
    void BitwiseOR(const LongFloat& Other);
    void BitwiseXOR(const LongFloat& Other);
    void BitwiseNOT();
    void ShiftLeft(uint32_t Shift);
    void ShiftRight(uint32_t Shift);
public:
    void PrintBits(bool CodeMode = false);
    [[nodiscard]] std::string ToString() const;
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
        const bool NegativeA = IsNegative();
        const bool NegativeB = Other.IsNegative();

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
        (*this) = Add(*this, Other);
    }
    void operator-=(const LongFloat& Other)
    {
        (*this) = Substract(*this, Other);
    }
    void operator*=(const LongFloat& Other)
    {
        (*this) = Multiplay(*this, Other);
    }
    void operator/=(const LongFloat& Other)
    {
        (*this) = Divide(*this, Other);
    }


    [[nodiscard]] LongFloat operator+(const LongFloat& Other)
    {
        return Add(*this, Other);
    }
    [[nodiscard]] LongFloat operator-(const LongFloat& Other)
    {
        return Substract(*this, Other);
    }
    [[nodiscard]] LongFloat operator*(const LongFloat& Other)
    {
        return Multiplay(*this, Other);
    }
    [[nodiscard]] LongFloat operator/(const LongFloat& Other)
    {
        return Divide(*this, Other);
    }
};
