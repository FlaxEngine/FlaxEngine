// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Vector2;
struct Vector3;
struct Vector4;
struct Matrix;

/// <summary>
/// Represents a four dimensional mathematical quaternion. Euler angles are stored in: pitch, yaw, roll order (x, y, z).
/// </summary>
API_STRUCT() struct FLAXENGINE_API Quaternion
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Quaternion);
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
    Quaternion()
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="xyzw">Value to assign to the all components.</param>
    Quaternion(const float xyzw)
        : X(xyzw)
        , Y(xyzw)
        , Z(xyzw)
        , W(xyzw)
    {
    }

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
    explicit Quaternion(const Vector4& value);

public:

    String ToString() const;

public:

    /// <summary>
    /// Gets a value indicating whether this instance is equivalent to the identity quaternion.
    /// </summary>
    /// <returns>True if quaternion is the identity quaternion, otherwise false.</returns>
    bool IsIdentity() const
    {
        return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z) && Math::IsOne(W);
    }

    /// <summary>
    /// Gets a value indicting whether this instance is normalized.
    /// </summary>
    /// <returns>True if is normalized, otherwise false.</returns>
    bool IsNormalized() const
    {
        return Math::IsOne(X * X + Y * Y + Z * Z + W * W);
    }

    /// <summary>
    /// Returns true if quaternion has one or more components is not a number (NaN).
    /// </summary>
    /// <returns>True if one or more components is not a number (NaN).</returns>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y) || isnan(Z) || isnan(W);
    }

    /// <summary>
    /// Returns true if quaternion has one or more components equal to +/- infinity.
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity.</returns>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y) || isinf(Z) || isinf(W);
    }

    /// <summary>
    /// Returns true if quaternion has one or more components equal to +/- infinity or NaN.
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity or NaN.</returns>
    bool IsNanOrInfinity() const
    {
        return IsInfinity() || IsNaN();
    }

    /// <summary>
    /// Gets the angle of the quaternion.
    /// </summary>
    /// <returns>The angle.</returns>
    float GetAngle() const
    {
        const float length = X * X + Y * Y + Z * Z;
        if (Math::IsZero(length))
            return 0.0f;
        return 2.0f * acosf(Math::Clamp(W, -1.0f, 1.0f));
    }

    /// <summary>
    /// Gets the axis components of the quaternion.
    /// </summary>
    /// <returns>The axis.</returns>
    Vector3 GetAxis() const;

    /// <summary>
    /// Calculates the length of the quaternion.
    /// </summary>
    /// <returns>The length of the quaternion.</returns>
    float Length() const
    {
        return Math::Sqrt(X * X + Y * Y + Z * Z + W * W);
    }

    /// <summary>
    /// Calculates the squared length of the quaternion.
    /// </summary>
    /// <returns>The squared length of the quaternion.</returns>
    float LengthSquared() const
    {
        return X * X + Y * Y + Z * Z + W * W;
    }

    /// <summary>
    /// Gets the euler angle (pitch, yaw, roll) in degrees.
    /// </summary>
    /// <returns>The euler angle</returns>
    Vector3 GetEuler() const;

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
        float lengthSq = LengthSquared();
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
    /// Reverses the direction of the quaternion.
    /// </summary>
    void Negate()
    {
        X = -X;
        Y = -Y;
        Z = -Z;
        W = -W;
    }

    /// <summary>
    /// Converts the quaternion into a unit quaternion.
    /// </summary>
    void Normalize()
    {
        const float length = Length();
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
    Vector3 operator*(const Vector3& vector) const;

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
        return Dot(*this, other) > 0.9999999f;
    }

    /// <summary>
    /// Determines whether the specified <see cref="Quaternion" /> is equal to this instance.
    /// </summary>
    /// <param name="other">The <see cref="Quaternion" /> to compare with this instance.</param>
    /// <returns><c>true</c> if the specified <see cref="Quaternion" /> isn't equal to this instance; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool operator!=(const Quaternion& other) const
    {
        return !(*this == other);
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
        return Dot(a, b) > 0.9999999f;
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
        Quaternion result;
        Invert(value, result);
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
        return dot > 0.9999999f ? 0 : Math::Acos(Math::Min(Math::Abs(dot), 1.0f)) * 2.0f * 57.29578f;
    }

    // Adds two quaternions
    // @param left The first quaternion to add
    // @param right The second quaternion to add
    // @param result When the method completes, contains the sum of the two quaternions
    static void Add(const Quaternion& left, const Quaternion& right, Quaternion& result)
    {
        result.X = left.X + right.X;
        result.Y = left.Y + right.Y;
        result.Z = left.Z + right.Z;
        result.W = left.W + right.W;
    }

    // Subtracts two quaternions
    // @param left The first quaternion to subtract
    // @param right The second quaternion to subtract
    // @param result When the method completes, contains the difference of the two quaternions
    static void Subtract(const Quaternion& left, const Quaternion& right, Quaternion& result)
    {
        result.X = left.X - right.X;
        result.Y = left.Y - right.Y;
        result.Z = left.Z - right.Z;
        result.W = left.W - right.W;
    }

    // Scales a quaternion by the given value
    // @param value The quaternion to scale
    // @param scale The amount by which to scale the quaternion
    // @param result When the method completes, contains the scaled quaternion
    static void Multiply(const Quaternion& value, float scale, Quaternion& result)
    {
        result.X = value.X * scale;
        result.Y = value.Y * scale;
        result.Z = value.Z * scale;
        result.W = value.W * scale;
    }

    // Multiplies a quaternion by another
    // @param left The first quaternion to multiply
    // @param right The second quaternion to multiply
    // @param result When the method completes, contains the multiplied quaternion
    static void Multiply(const Quaternion& left, const Quaternion& right, Quaternion& result);

    // Reverses the direction of a given quaternion
    // @param value The quaternion to negate
    // @param result When the method completes, contains a quaternion facing in the opposite direction
    static void Negate(const Quaternion& value, Quaternion& result)
    {
        result.X = -value.X;
        result.Y = -value.Y;
        result.Z = -value.Z;
        result.W = -value.W;
    }

    // Performs a linear interpolation between two quaternions
    // @param start Start quaternion
    // @param end End quaternion
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the linear interpolation of the two quaternions
    static void Lerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result);

    // Performs a linear interpolation between two quaternion
    // @param start Start quaternion
    // @param end End quaternion
    // @param amount Value between 0 and 1 indicating the weight of end
    // @returns The linear interpolation of the two quaternions
    static Quaternion Lerp(const Quaternion& start, const Quaternion& end, float amount)
    {
        Quaternion result;
        Lerp(start, end, amount, result);
        return result;
    }

    // Creates a quaternion given a rotation and an axis
    // @param axis The axis of rotation
    // @param angle The angle of rotation (in radians).
    // @param result When the method completes, contains the newly created quaternion
    static void RotationAxis(const Vector3& axis, float angle, Quaternion& result);

    // Creates a quaternion given a angle cosine and an axis
    // @param axis The axis of rotation
    // @param cos The angle cosine, it must be within [-1,1] range (in radians).
    // @param result When the method completes, contains the newly created quaternion
    static void RotationCosAxis(const Vector3& axis, float cos, Quaternion& result);

    // Creates a quaternion given a rotation matrix
    // @param matrix The rotation matrix
    // @param result When the method completes, contains the newly created quaternion
    static void RotationMatrix(const Matrix& matrix, Quaternion& result);

    // Creates a left-handed, look-at quaternion
    // @param eye The position of the viewer's eye
    // @param target The camera look-at target
    // @param up The camera's up vector
    // @param result When the method completes, contains the created look-at quaternion
    static void LookAt(const Vector3& eye, const Vector3& target, const Vector3& up, Quaternion& result);

    // Creates a left-handed, look-at quaternion
    // @param forward The camera's forward direction
    // @param up The camera's up vector
    // @param result When the method completes, contains the created look-at quaternion
    static void RotationLookAt(const Vector3& forward, const Vector3& up, Quaternion& result);

    // Creates a left-handed spherical billboard that rotates around a specified object position
    // @param objectPosition The position of the object around which the billboard will rotate
    // @param cameraPosition The position of the camera
    // @param cameraUpVector The up vector of the camera
    // @param cameraForwardVector The forward vector of the camera
    // @param result When the method completes, contains the created billboard quaternion
    static void Billboard(const Vector3& objectPosition, const Vector3& cameraPosition, const Vector3& cameraUpVector, const Vector3& cameraForwardVector, Quaternion& result);

    /// <summary>
    /// Creates a rotation with the specified forward and upwards directions.
    /// </summary>
    /// <param name="forward">The forward direction. Direction to orient towards.</param>
    /// <param name="up">The up direction. Constrains y axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
    /// <param name="result">The calculated quaternion.</param>
    static void LookRotation(const Vector3& forward, const Vector3& up, Quaternion& result);

    /// <summary>
    /// Creates a rotation with the specified forward and upwards directions.
    /// </summary>
    /// <param name="forward">The forward direction. Direction to orient towards.</param>
    /// <param name="up">Up direction. Constrains y axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
    /// <returns>The calculated quaternion.</returns>
    static Quaternion LookRotation(const Vector3& forward, const Vector3& up)
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
    static void GetRotationFromTo(const Vector3& from, const Vector3& to, Quaternion& result, const Vector3& fallbackAxis);

    /// <summary>
    /// Gets the shortest arc quaternion to rotate this vector to the destination vector.
    /// </summary>
    /// <param name="from">The source vector.</param>
    /// <param name="to">The destination vector.</param>
    /// <param name="fallbackAxis">The fallback axis.</param>
    /// <returns>The rotation.</returns>
    static Quaternion GetRotationFromTo(const Vector3& from, const Vector3& to, const Vector3& fallbackAxis)
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
    static void FindBetween(const Vector3& from, const Vector3& to, Quaternion& result);

    /// <summary>
    /// Gets the quaternion that will rotate vector from into vector to, around their plan perpendicular axis.The input vectors don't need to be normalized.
    /// </summary>
    /// <param name="from">The source vector.</param>
    /// <param name="to">The destination vector.</param>
    /// <returns>The rotation.</returns>
    static Quaternion FindBetween(const Vector3& from, const Vector3& to)
    {
        Quaternion result;
        FindBetween(from, to, result);
        return result;
    }

    // Creates a quaternion given a rotation matrix
    // @param matrix The rotation matrix
    // @returns The newly created quaternion
    static Quaternion RotationMatrix(const Matrix& matrix)
    {
        Quaternion result;
        RotationMatrix(matrix, result);
        return result;
    }

    // Interpolates between two quaternions, using spherical linear interpolation
    // @param start Start quaternion
    // @param end End quaternion
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the spherical linear interpolation of the two quaternions
    static void Slerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result);

    // Creates a quaternion given a yaw, pitch, and roll value (is using degrees)
    // @param x Roll (in degrees)
    // @param x Pitch (in degrees)
    // @param x Yaw (in degrees)
    // @returns Result rotation
    static Quaternion Euler(float x, float y, float z);

    // Creates a quaternion given a yaw, pitch, and roll value (is using degrees)
    // @param euler Euler angle rotation with values in order: X:roll, Y:pitch, Z:yaw (in degrees)
    // @returns Result rotation
    static Quaternion Euler(const Vector3& euler);

    // Creates a quaternion given a yaw, pitch, and roll value (is using radians)
    // @param yaw The yaw of rotation (in radians)
    // @param pitch The pitch of rotation (in radians)
    // @param roll The roll of rotation (in radians)
    // @param result When the method completes, contains the newly created quaternion
    static Quaternion RotationYawPitchRoll(float yaw, float pitch, float roll)
    {
        Quaternion result;
        RotationYawPitchRoll(yaw, pitch, roll, result);
        return result;
    }

    // Creates a quaternion given a yaw, pitch, and roll value (is using radians)
    // @param yaw The yaw of rotation (in radians)
    // @param pitch The pitch of rotation (in radians)
    // @param roll The roll of rotation (in radians)
    // @param result When the method completes, contains the newly created quaternion
    static void RotationYawPitchRoll(float yaw, float pitch, float roll, Quaternion& result);
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
