// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Matrix;
struct Matrix3x3;

/// <summary>
/// Represents a four dimensional mathematical quaternion. Euler angles are stored in: pitch, yaw, roll order (x, y, z).
/// </summary>
API_STRUCT() struct FLAXENGINE_API Quaternion
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Quaternion);

    /// <summary>
    /// Equality tolerance factor used when comparing quaternions via dot operation.
    /// </summary>
    API_FIELD() static constexpr Real Tolerance = 0.999999f;

public:
    union
    {
        struct
        {
            /// <summary>
            /// The X component of the quaternion.
            /// </summary>
            API_FIELD() float X;

            /// <summary>
            /// The Y component of the quaternion.
            /// </summary>
            API_FIELD() float Y;

            /// <summary>
            /// The Z component of the quaternion.
            /// </summary>
            API_FIELD() float Z;

            /// <summary>
            /// The W component of the quaternion.
            /// </summary>
            API_FIELD() float W;
        };

        /// <summary>
        /// The raw value.
        /// </summary>
        float Raw[4];
    };

public:
    /// <summary>
    /// Quaternion with all components equal 0.
    /// </summary>
    static Quaternion Zero;

    /// <summary>
    /// Quaternion with all components equal 1.
    /// </summary>
    static Quaternion One;

    /// <summary>
    /// Identity quaternion (represents no rotation).
    /// </summary>
    static Quaternion Identity;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Quaternion() = default;

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="x">X component value.</param>
    /// <param name="y">Y component value.</param>
    /// <param name="z">Z component value.</param>
    /// <param name="w">W component value.</param>
    Quaternion(const float x, const float y, const float z, const float w)
        : X(x)
        , Y(y)
        , Z(z)
        , W(w)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Vector to set value.</param>
    explicit Quaternion(const Float4& value);

public:
    String ToString() const;

public:
    /// <summary>
    /// Gets a value indicating whether this instance is equivalent to the identity quaternion.
    /// </summary>
    bool IsIdentity() const
    {
        return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z) && Math::IsOne(W);
    }

    /// <summary>
    /// Gets a value indicting whether this instance is normalized.
    /// </summary>
    bool IsNormalized() const
    {
        return Math::Abs((X * X + Y * Y + Z * Z + W * W) - 1.0f) < 1e-4f;
    }

    /// <summary>
    /// Returns true if quaternion has one or more components is not a number (NaN).
    /// </summary>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y) || isnan(Z) || isnan(W);
    }

    /// <summary>
    /// Returns true if quaternion has one or more components equal to +/- infinity.
    /// </summary>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y) || isinf(Z) || isinf(W);
    }

    /// <summary>
    /// Returns true if quaternion has one or more components equal to +/- infinity or NaN.
    /// </summary>
    bool IsNanOrInfinity() const
    {
        return IsInfinity() || IsNaN();
    }

    /// <summary>
    /// Gets the angle of the quaternion.
    /// </summary>
    float GetAngle() const;

    /// <summary>
    /// Gets the axis components of the quaternion.
    /// </summary>
    Float3 GetAxis() const;

    /// <summary>
    /// Calculates the length of the quaternion.
    /// </summary>
    float Length() const
    {
        return Math::Sqrt(X * X + Y * Y + Z * Z + W * W);
    }

    /// <summary>
    /// Calculates the squared length of the quaternion.
    /// </summary>
    float LengthSquared() const
    {
        return X * X + Y * Y + Z * Z + W * W;
    }

    /// <summary>
    /// Gets the euler angle (pitch, yaw, roll) in degrees.
    /// </summary>
    Float3 GetEuler() const;

    /// <summary>
    /// Conjugates the quaternion
    /// </summary>
    void Conjugate()
    {
        X = -X;
        Y = -Y;
        Z = -Z;
    }

    /// <summary>
    /// Gets the conjugated quaternion.
    /// </summary>
    Quaternion Conjugated() const
    {
        return { -X, -Y, -Z, W };
    }

    /// <summary>
    /// Conjugates and renormalizes the quaternion.
    /// </summary>
    void Invert()
    {
        float lengthSq = Math::Sqrt(X * X + Y * Y + Z * Z + W * W);
        if (!Math::IsZero(lengthSq))
        {
            lengthSq = 1.0f / lengthSq;
            X = -X * lengthSq;
            Y = -Y * lengthSq;
            Z = -Z * lengthSq;
            W = W * lengthSq;
        }
    }

    /// <summary>
    /// Converts the quaternion into a unit quaternion.
    /// </summary>
    void Normalize()
    {
        const float length = Math::Sqrt(X * X + Y * Y + Z * Z + W * W);
        if (!Math::IsZero(length))
        {
            const float inv = 1.0f / length;
            X *= inv;
            Y *= inv;
            Z *= inv;
            W *= inv;
        }
    }

    /// <summary>
    /// Scales a quaternion by the given value.
    /// </summary>
    /// <param name="scale">The amount by which to scale the quaternion.</param>
    void Multiply(float scale)
    {
        X *= scale;
        Y *= scale;
        Z *= scale;
        W *= scale;
    }

    /// <summary>
    /// Multiplies a quaternion by another.
    /// </summary>
    /// <param name="other">The other quaternion to multiply by.</param>
    void Multiply(const Quaternion& other);

public:
    /// <summary>
    /// Adds two quaternions.
    /// </summary>
    /// <param name="b">The quaternion to add.</param>
    /// <returns>The sum of the two quaternions.</returns>
    inline Quaternion operator+(const Quaternion& b) const
    {
        return Quaternion(X + b.X, Y + b.Y, Z + b.Z, W + b.W);
    }

    /// <summary>
    /// Subtracts two quaternions.
    /// </summary>
    /// <param name="b">The quaternion to subtract.</param>
    /// <returns>The difference of the two quaternions.</returns>
    inline Quaternion operator-(const Quaternion& b) const
    {
        return Quaternion(X - b.X, Y - b.Y, Z - b.Z, W - b.W);
    }

    /// <summary>
    /// Multiplies two quaternions.
    /// </summary>
    /// <param name="b">The quaternion to multiply.</param>
    /// <returns>The multiplied quaternion.</returns>
    inline Quaternion operator*(const Quaternion& b) const
    {
        Quaternion result;
        Multiply(*this, b, result);
        return result;
    }

    /// <summary>
    /// Adds two quaternions.
    /// </summary>
    /// <param name="b">The quaternion to add.</param>
    /// <returns>The sum of the two quaternions.</returns>
    inline Quaternion& operator+=(const Quaternion& b)
    {
        X += b.X;
        Y += b.Y;
        Z += b.Z;
        W += b.W;
        return *this;
    }

    /// <summary>
    /// Subtracts two quaternions.
    /// </summary>
    /// <param name="b">The quaternion to subtract.</param>
    /// <returns>The difference of the two quaternions.</returns>
    inline Quaternion& operator-=(const Quaternion& b)
    {
        X -= b.X;
        Y -= b.Y;
        Z -= b.Z;
        W -= b.W;
        return *this;
    }

    /// <summary>
    /// Multiplies two quaternions.
    /// </summary>
    /// <param name="b">The quaternion to multiply.</param>
    /// <returns>The multiplied quaternion.</returns>
    inline Quaternion& operator*=(const Quaternion& b)
    {
        Multiply(b);
        return *this;
    }

    /// <summary>
    /// Scales a quaternion by the given value.
    /// </summary>
    /// <param name="scale">The amount by which to scale the quaternion.</param>
    /// <returns>The scaled quaternion.</returns>
    inline Quaternion operator*(float scale) const
    {
        Quaternion q = *this;
        q.Multiply(scale);
        return q;
    }

    /// <summary>
    /// Transforms a vector by the given rotation.
    /// </summary>
    /// <param name="vector">The vector to transform.</param>
    /// <returns>The scaled vector.</returns>
    Float3 operator*(const Float3& vector) const;

    /// <summary>
    /// Scales a quaternion by the given value.
    /// </summary>
    /// <param name="scale">The amount by which to scale the quaternion.</param>
    /// <returns>This instance.</returns>
    FORCE_INLINE Quaternion& operator*=(float scale)
    {
        Multiply(scale);
        return *this;
    }

    /// <summary>
    /// Determines whether the specified <see cref="Quaternion" /> is equal to this instance.
    /// </summary>
    /// <param name="other">The <see cref="Quaternion" /> to compare with this instance.</param>
    /// <returns><c>true</c> if the specified <see cref="Quaternion" /> is equal to this instance; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool operator==(const Quaternion& other) const
    {
        return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
    }

    /// <summary>
    /// Determines whether the specified <see cref="Quaternion" /> is equal to this instance.
    /// </summary>
    /// <param name="other">The <see cref="Quaternion" /> to compare with this instance.</param>
    /// <returns><c>true</c> if the specified <see cref="Quaternion" /> isn't equal to this instance; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool operator!=(const Quaternion& other) const
    {
        return X != other.X || Y != other.Y || Z != other.Z || W != other.W;
    }

public:
    /// <summary>
    /// Determines whether the specified <see cref="Quaternion" /> structures are equal.
    /// </summary>
    /// <param name="a">The first <see cref="Quaternion" /> to compare.</param>
    /// <param name="b">The second <see cref="Quaternion" /> to compare.</param>
    /// <returns><c>true</c> if the specified <see cref="Quaternion" /> structures are equal; otherwise, <c>false</c>.</returns>
    static bool NearEqual(const Quaternion& a, const Quaternion& b)
    {
        return Dot(a, b) > Tolerance;
    }

    /// <summary>
    /// Determines whether the specified <see cref="Quaternion" /> structures are equal.
    /// </summary>
    /// <param name="a">The first <see cref="Quaternion" /> to compare.</param>
    /// <param name="b">The second <see cref="Quaternion" /> to compare.</param>
    /// <param name="epsilon">The comparision threshold value.</param>
    /// <returns><c>true</c> if the specified <see cref="Quaternion" /> structures are equal within the specified epsilon range; otherwise, <c>false</c>.</returns>
    static bool NearEqual(const Quaternion& a, const Quaternion& b, float epsilon)
    {
        return Dot(a, b) > 1.0f - epsilon;
    }

public:
    /// <summary>
    /// Calculates the inverse of the specified quaternion.
    /// </summary>
    /// <param name="value">The quaternion whose inverse is to be calculated.</param>
    /// <returns>The inverse of the specified quaternion.</returns>
    static Quaternion Invert(const Quaternion& value)
    {
        Quaternion result = value;
        result.Invert();
        return result;
    }

    /// <summary>
    /// Calculates the inverse of the specified quaternion.
    /// </summary>
    /// <param name="value">The quaternion whose inverse is to be calculated.</param>
    /// <param name="result">When the method completes, contains the inverse of the specified quaternion.</param>
    static void Invert(const Quaternion& value, Quaternion& result)
    {
        result = value;
        result.Invert();
    }

    /// <summary>
    /// Calculates the dot product of two quaternions.
    /// </summary>
    /// <param name="left">The first source quaternion.</param>
    /// <param name="right">The second source quaternion.</param>
    /// <returns>The dot product of the two quaternions.</returns>
    static float Dot(const Quaternion& left, const Quaternion& right)
    {
        return left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
    }

    /// <summary>
    /// Calculates the angle between two quaternions.
    /// </summary>
    /// <param name="a">First source quaternion.</param>
    /// <param name="b">Second source quaternion.</param>
    /// <returns>Returns the angle in degrees between two rotations a and b.</returns>
    static float AngleBetween(const Quaternion& a, const Quaternion& b)
    {
        const float dot = Dot(a, b);
        return dot > Tolerance ? 0 : Math::Acos(Math::Min(Math::Abs(dot), 1.0f)) * 2.0f * 57.29578f;
    }

    // Adds two quaternions.
    static void Add(const Quaternion& left, const Quaternion& right, Quaternion& result);

    // Subtracts two quaternions.
    static void Subtract(const Quaternion& left, const Quaternion& right, Quaternion& result);

    // Scales a quaternion by the given value.
    static void Multiply(const Quaternion& value, float scale, Quaternion& result)
    {
        result.X = value.X * scale;
        result.Y = value.Y * scale;
        result.Z = value.Z * scale;
        result.W = value.W * scale;
    }

    // Multiplies a quaternion by another.
    static void Multiply(const Quaternion& left, const Quaternion& right, Quaternion& result);

    // Reverses the direction of a given quaternion.
    static void Negate(const Quaternion& value, Quaternion& result);

    // Performs a linear interpolation between two quaternions.
    static void Lerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result);

    // Performs a linear interpolation between two quaternion.
    static Quaternion Lerp(const Quaternion& start, const Quaternion& end, float amount)
    {
        Quaternion result;
        Lerp(start, end, amount, result);
        return result;
    }

    // Creates a quaternion given an angle cosine (radians in range [-1,1]) and an axis of rotation (normalized).
    static void RotationAxis(const Float3& axis, float angle, Quaternion& result);

    // Creates a quaternion given an angle cosine (radians in range [-1,1]) and an axis of rotation (normalized).
    static void RotationCosAxis(const Float3& axis, float cos, Quaternion& result);

    // Creates a quaternion given a rotation matrix.
    static void RotationMatrix(const Matrix& matrix, Quaternion& result);

    // Creates a quaternion given a rotation matrix.
    static void RotationMatrix(const Matrix3x3& matrix, Quaternion& result);

    // Creates a left-handed, look-at quaternion.
    static void LookAt(const Float3& eye, const Float3& target, const Float3& up, Quaternion& result);

    // Creates a left-handed, look-at quaternion.
    static void RotationLookAt(const Float3& forward, const Float3& up, Quaternion& result);

    // Creates a left-handed spherical billboard that rotates around a specified object position.
    static void Billboard(const Float3& objectPosition, const Float3& cameraPosition, const Float3& cameraUpFloat, const Float3& cameraForwardFloat, Quaternion& result);

    /// <summary>
    /// Calculates the orientation from the direction vector.
    /// </summary>
    /// <param name="direction">The direction vector (normalized).</param>
    /// <returns>The orientation.</returns>
    static Quaternion FromDirection(const Float3& direction);

    /// <summary>
    /// Creates a rotation with the specified forward and upwards directions.
    /// </summary>
    /// <param name="forward">The forward direction. Direction to orient towards.</param>
    /// <param name="up">The up direction. Constrains y axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
    /// <param name="result">The calculated quaternion.</param>
    static void LookRotation(const Float3& forward, const Float3& up, Quaternion& result);

    /// <summary>
    /// Creates a rotation with the specified forward and upwards directions.
    /// </summary>
    /// <param name="forward">The forward direction. Direction to orient towards.</param>
    /// <param name="up">Up direction. Constrains y axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
    /// <returns>The calculated quaternion.</returns>
    static Quaternion LookRotation(const Float3& forward, const Float3& up)
    {
        Quaternion result;
        LookRotation(forward, up, result);
        return result;
    }

    /// <summary>
    /// Gets the shortest arc quaternion to rotate this vector to the destination vector.
    /// </summary>
    /// <param name="from">The source vector.</param>
    /// <param name="to">The destination vector.</param>
    /// <param name="result">The result.</param>
    /// <param name="fallbackAxis">The fallback axis.</param>
    static void GetRotationFromTo(const Float3& from, const Float3& to, Quaternion& result, const Float3& fallbackAxis);

    /// <summary>
    /// Gets the shortest arc quaternion to rotate this vector to the destination vector.
    /// </summary>
    /// <param name="from">The source vector.</param>
    /// <param name="to">The destination vector.</param>
    /// <param name="fallbackAxis">The fallback axis.</param>
    /// <returns>The rotation.</returns>
    static Quaternion GetRotationFromTo(const Float3& from, const Float3& to, const Float3& fallbackAxis)
    {
        Quaternion result;
        GetRotationFromTo(from, to, result, fallbackAxis);
        return result;
    }

    /// <summary>
    /// Gets the quaternion that will rotate vector from into vector to, around their plan perpendicular axis.The input vectors don't need to be normalized.
    /// </summary>
    /// <param name="from">The source vector.</param>
    /// <param name="to">The destination vector.</param>
    /// <param name="result">The result.</param>
    static void FindBetween(const Float3& from, const Float3& to, Quaternion& result);

    /// <summary>
    /// Gets the quaternion that will rotate vector from into vector to, around their plan perpendicular axis.The input vectors don't need to be normalized.
    /// </summary>
    /// <param name="from">The source vector.</param>
    /// <param name="to">The destination vector.</param>
    /// <returns>The rotation.</returns>
    static Quaternion FindBetween(const Float3& from, const Float3& to)
    {
        Quaternion result;
        FindBetween(from, to, result);
        return result;
    }

    // Creates a quaternion given a rotation matrix.
    static Quaternion RotationMatrix(const Matrix& matrix)
    {
        Quaternion result;
        RotationMatrix(matrix, result);
        return result;
    }

    // Interpolates between two quaternions, using spherical linear interpolation.
    static Quaternion Slerp(const Quaternion& start, const Quaternion& end, float amount)
    {
        Quaternion result;
        Slerp(start, end, amount, result);
        return result;
    }

    // Interpolates between two quaternions, using spherical linear interpolation.
    static void Slerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result);

    // Creates a quaternion given a yaw, pitch, and roll value (in degrees).
    static Quaternion Euler(float x, float y, float z);

    // Creates a quaternion given a yaw, pitch, and roll value in order: X:roll, Y:pitch, Z:yaw (in degrees).
    static Quaternion Euler(const Float3& euler);

    // Creates a quaternion given a yaw, pitch, and roll value (in radians).
    static Quaternion RotationYawPitchRoll(float yaw, float pitch, float roll)
    {
        Quaternion result;
        RotationYawPitchRoll(yaw, pitch, roll, result);
        return result;
    }

    // Creates a quaternion given a yaw, pitch, and roll value (in radians).
    static void RotationYawPitchRoll(float yaw, float pitch, float roll, Quaternion& result);

    /// <summary>
    /// Gets rotation from a normal in relation to a transform. This function is especially useful for axis aligned faces, and with <seealso cref="Physics::RayCast"/>.
    /// </summary>
    /// <param name="normal">The normal vector.</param>
    /// <param name="reference">The reference transform.</param>
    /// <returns>The rotation from the normal vector.</returns>
    static Quaternion GetRotationFromNormal(const Vector3& normal, const Transform& reference);
};

/// <summary>
/// Scales a quaternion by the given value.
/// </summary>
/// <param name="a">The amount by which to scale the quaternion.</param>
/// <param name="b">The quaternion to scale.</param>
/// <returns>The scaled quaternion.</returns>
inline Quaternion operator*(float a, const Quaternion& b)
{
    return b * a;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Quaternion& a, const Quaternion& b)
    {
        return Quaternion::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Quaternion>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Quaternion, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);
