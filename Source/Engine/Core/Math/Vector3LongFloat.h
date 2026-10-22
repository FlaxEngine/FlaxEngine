#pragma once
#include "Engine/Core/Types/BaseTypes.h"
#include "LongFloat.h"

API_STRUCT() struct FLAXENGINE_API Vector3LongFloat
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Vector3LongFloat);
public:
    API_FIELD() LongFloat X;
    API_FIELD() LongFloat Y;
    API_FIELD() LongFloat Z;

    static Vector3LongFloat Zero;
    static Vector3LongFloat One;
    static Vector3LongFloat Up;
    static Vector3LongFloat Down;
    static Vector3LongFloat Left;
    static Vector3LongFloat Right;
    static Vector3LongFloat Forward;
    static Vector3LongFloat Backward;

    Vector3LongFloat() : X(0.0), Y(0.0), Z(0.0) {}

    Vector3LongFloat(LongFloat X, LongFloat Y, LongFloat Z)
    {
        this->X = X;
        this->Y = Y;
        this->Z = Z;
    }

    Vector3LongFloat(LongFloat Scale)
    {
        this->X = Scale;
        this->Y = Scale;
        this->Z = Scale;
    }

    /// <summary>
    /// Quantizes Long flaot to the Float3.
    /// </summary>
    /// <returns>
    /// Quantized Float3 Vector
    /// </returns>

    [[nodiscard]] Float3 QuantizeToFloat3() const;

    /// <summary>
    /// Quantizes Long flaot to the Double3.
    /// </summary>
    /// <returns>
    /// Quantized Double3 Vector
    /// </returns>
    [[nodiscard]] Double3 QuantizeToDouble3() const;

    /// <summary>
    /// Quantizes Long flaot to the Real Vector.
    /// </summary>
    /// <returns>
    /// Quantized Real Vector
    /// </returns>
    [[nodiscard]] Vector3 Quantize() const;

    [[nodiscard]] static LongFloat DistanceSquared(const Vector3LongFloat& rhs, const Vector3LongFloat& lhs)
    {
        const LongFloat x = rhs.X - lhs.X;
        const LongFloat y = rhs.Y - lhs.Y;
        const LongFloat z = rhs.Z - lhs.Z;
        LongFloat squared = x * x + y * y + z * z;
        if (LongFloatMath::IsNAN(squared))
        {
            //set it to max overflow triggered probably
            //todo: for now it will return max value but if needed it can crash,remove this to do if decision was made
            return LongFloat::Max;
        }
        return squared;
    }



    [[nodiscard]] bool operator==(const Vector3LongFloat& rhs) const
    {
        return X == rhs.X && Y == rhs.Y && Z == rhs.Z;
    }

    [[nodiscard]] bool operator<(const Vector3LongFloat& rhs) const
    {
        return X < rhs.X && Y < rhs.Y && Z < rhs.Z;
    }

    [[nodiscard]] Vector3LongFloat operator+(const Vector3LongFloat& rhs) const
    {
        return Vector3LongFloat(
            LongFloatMath::Add(this->X, rhs.X),
            LongFloatMath::Add(this->Y, rhs.Y),
            LongFloatMath::Add(this->Z, rhs.Z)
        );
    }
    [[nodiscard]] Vector3LongFloat operator-(const Vector3LongFloat& rhs) const
    {
        return Vector3LongFloat(
            LongFloatMath::Subtract(this->X, rhs.X),
            LongFloatMath::Subtract(this->Y, rhs.Y),
            LongFloatMath::Subtract(this->Z, rhs.Z)
        );
    }
    [[nodiscard]] Vector3LongFloat operator*(const Vector3LongFloat& rhs) const
    {
        return Vector3LongFloat(
            LongFloatMath::Multiply(this->X, rhs.X),
            LongFloatMath::Multiply(this->Y, rhs.Y),
            LongFloatMath::Multiply(this->Z, rhs.Z)
        );
    }
    [[nodiscard]] Vector3LongFloat operator/(const Vector3LongFloat& rhs) const
    {
        return Vector3LongFloat(
            LongFloatMath::Divide(this->X, rhs.X),
            LongFloatMath::Divide(this->Y, rhs.Y),
            LongFloatMath::Divide(this->Z, rhs.Z)
        );
    }

    [[nodiscard]] Vector3LongFloat operator-() const
    {
        return Vector3LongFloat(-X, -Y, -Z);
    }
public:
    [[nodiscard]] bool operator!=(const Vector3LongFloat& rhs) const
    {
        return !(*this == rhs);
    }
    [[nodiscard]] bool operator>(const Vector3LongFloat& rhs) const
    {
        return rhs < *this;
    }

    [[nodiscard]] bool operator<=(const Vector3LongFloat& rhs) const
    {
        return !(rhs < *this);
    }
    [[nodiscard]] bool operator>=(const Vector3LongFloat& rhs) const
    {
        return !(*this < rhs);
    }
    void operator+=(const Vector3LongFloat& rhs)
    {
        (*this) = *this + rhs;
    }
    void operator-=(const Vector3LongFloat& rhs)
    {
        (*this) = *this - rhs;
    }
    void operator*=(const Vector3LongFloat& rhs)
    {
        (*this) = *this * rhs;
    }
    void operator/=(const Vector3LongFloat& rhs)
    {
        (*this) = *this / rhs;
    }
};
