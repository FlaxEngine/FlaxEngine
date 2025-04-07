// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Mathd.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

/// <summary>
/// Represents a three dimensional mathematical vector.
/// </summary>
template<typename T>
API_STRUCT(Template) struct Vector3Base
{
    typedef T Real;
    static FLAXENGINE_API struct ScriptingTypeInitializer TypeInitializer;

    union
    {
        struct
        {
            /// <summary>
            /// The X component.
            /// </summary>
            API_FIELD() T X;

            /// <summary>
            /// The Y component.
            /// </summary>
            API_FIELD() T Y;

            /// <summary>
            /// The Z component.
            /// </summary>
            API_FIELD() T Z;
        };

        /// <summary>
        /// The raw vector values (in XYZ order).
        /// </summary>
        T Raw[3];
    };

public:
    // Vector with all components equal zero (0, 0, 0).
    static FLAXENGINE_API const Vector3Base<T> Zero;

    // Vector with all components equal one (1, 1, 1).
    static FLAXENGINE_API const Vector3Base<T> One;

    // Vector with all components equal half (0.5, 0.5, 0.5).
    static FLAXENGINE_API const Vector3Base<T> Half;

    // The X unit vector (1, 0, 0).
    static FLAXENGINE_API const Vector3Base<T> UnitX;

    // The Y unit vector (0, 1, 0).
    static FLAXENGINE_API const Vector3Base<T> UnitY;

    // The Z unit vector (0, 0, 1).
    static FLAXENGINE_API const Vector3Base<T> UnitZ;

    // A unit vector designating up (0, 1, 0).
    static FLAXENGINE_API const Vector3Base<T> Up;

    // A unit vector designating down (0, -1, 0).
    static FLAXENGINE_API const Vector3Base<T> Down;

    // A unit vector designating a (-1, 0, 0).
    static FLAXENGINE_API const Vector3Base<T> Left;

    // A unit vector designating b (1, 0, 0).
    static FLAXENGINE_API const Vector3Base<T> Right;

    // A unit vector designating forward in a a-handed coordinate system (0, 0, 1).
    static FLAXENGINE_API const Vector3Base<T> Forward;

    // A unit vector designating backward in a a-handed coordinate system (0, 0, -1).
    static FLAXENGINE_API const Vector3Base<T> Backward;

    // Vector with all components equal maximum value.
    static FLAXENGINE_API const Vector3Base<T> Minimum;

    // Vector with all components equal minimum value.
    static FLAXENGINE_API const Vector3Base<T> Maximum;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Vector3Base() = default;

    FORCE_INLINE Vector3Base(T xyz)
        : X(xyz)
        , Y(xyz)
        , Z(xyz)
    {
    }

    FORCE_INLINE explicit Vector3Base(const T* xyz)
        : X(xyz[0])
        , Y(xyz[1])
        , Z(xyz[2])
    {
    }

    FORCE_INLINE Vector3Base(T x, T y, T z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    template<typename U = T, typename TEnableIf<TNot<TIsTheSame<T, U>>::Value>::Type...>
    FORCE_INLINE Vector3Base(const Vector3Base<U>& xyz)
        : X((T)xyz.X)
        , Y((T)xyz.Y)
        , Z((T)xyz.Z)
    {
    }

    FLAXENGINE_API Vector3Base(const Float2& xy, T z = 0);
    FLAXENGINE_API Vector3Base(const Double2& xy, T z = 0);
    FLAXENGINE_API Vector3Base(const Int2& xy, T z = 0);
    FLAXENGINE_API Vector3Base(const Int3& xyz);
    FLAXENGINE_API explicit Vector3Base(const Float4& xyz);
    FLAXENGINE_API explicit Vector3Base(const Double4& xyz);
    FLAXENGINE_API explicit Vector3Base(const Int4& xyz);
    FLAXENGINE_API explicit Vector3Base(const Color& color);

public:
    FLAXENGINE_API String ToString() const;

public:
    // Gets a value indicting whether this instance is normalized.
    bool IsNormalized() const
    {
        return Math::Abs((X * X + Y * Y + Z * Z) - 1.0f) < 1e-4f;
    }

    // Gets a value indicting whether this vector is zero.
    bool IsZero() const
    {
        return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z);
    }

    // Gets a value indicting whether any vector component is zero.
    bool IsAnyZero() const
    {
        return Math::IsZero(X) || Math::IsZero(Y) || Math::IsZero(Z);
    }

    // Gets a value indicting whether this vector is one.
    bool IsOne() const
    {
        return Math::IsOne(X) && Math::IsOne(Y) && Math::IsOne(Z);
    }

    // Calculates the length of the vector.
    T Length() const
    {
        return Math::Sqrt(X * X + Y * Y + Z * Z);
    }

    // Calculates the squared length of the vector.
    T LengthSquared() const
    {
        return X * X + Y * Y + Z * Z;
    }

    // Calculates inverted length of the vector (1 / length).
    T InvLength() const
    {
        return 1.0f / Length();
    }

    /// <summary>
    /// Returns the average arithmetic of all the components.
    /// </summary>
    T AverageArithmetic() const
    {
        return (X + Y + Z) * 0.333333334f;
    }

    /// <summary>
    /// Gets the sum of all vector components values.
    /// </summary>
    T SumValues() const
    {
        return X + Y + Z;
    }

    /// <summary>
    /// Returns the minimum value of all the components.
    /// </summary>
    T MinValue() const
    {
        return Math::Min(X, Y, Z);
    }

    /// <summary>
    /// Returns the maximum value of all the components.
    /// </summary>
    T MaxValue() const
    {
        return Math::Max(X, Y, Z);
    }

    /// <summary>
    /// Returns true if vector has one or more components is not a number (NaN).
    /// </summary>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y) || isnan(Z);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity.
    /// </summary>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y) || isinf(Z);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity or NaN.
    /// </summary>
    bool IsNanOrInfinity() const
    {
        return IsInfinity() || IsNaN();
    }

    /// <summary>
    /// Calculates a vector with values being absolute values of that vector.
    /// </summary>
    Vector3Base GetAbsolute() const
    {
        return Vector3Base(Math::Abs(X), Math::Abs(Y), Math::Abs(Z));
    }

    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector.
    /// </summary>
    Vector3Base GetNegative() const
    {
        return Vector3Base(-X, -Y, -Z);
    }

    /// <summary>
    /// Calculates a normalized vector that has length equal to 1.
    /// </summary>
    Vector3Base GetNormalized() const
    {
        Vector3Base result(X, Y, Z);
        result.Normalize();
        return result;
    }

public:
    /// <summary>
    /// Performs vector normalization (scales vector up to unit length).
    /// </summary>
    void Normalize()
    {
        const T length = Math::Sqrt(X * X + Y * Y + Z * Z);
        if (length >= ZeroTolerance)
        {
            const T inv = (T)1.0f / length;
            X *= inv;
            Y *= inv;
            Z *= inv;
        }
    }

    /// <summary>
    /// Performs fast vector normalization (scales vector up to unit length).
    /// </summary>
    void NormalizeFast()
    {
        const T inv = 1.0f / Math::Sqrt(X * X + Y * Y + Z * Z);
        X *= inv;
        Y *= inv;
        Z *= inv;
    }

public:
    Vector3Base operator+(const Vector3Base& b) const
    {
        return Vector3Base(X + b.X, Y + b.Y, Z + b.Z);
    }

    Vector3Base operator-(const Vector3Base& b) const
    {
        return Vector3Base(X - b.X, Y - b.Y, Z - b.Z);
    }

    Vector3Base operator*(const Vector3Base& b) const
    {
        return Vector3Base(X * b.X, Y * b.Y, Z * b.Z);
    }

    Vector3Base operator/(const Vector3Base& b) const
    {
        return Vector3Base(X / b.X, Y / b.Y, Z / b.Z);
    }

    Vector3Base operator-() const
    {
        return Vector3Base(-X, -Y, -Z);
    }

    Vector3Base operator+(T b) const
    {
        return Vector3Base(X + b, Y + b, Z + b);
    }

    Vector3Base operator-(T b) const
    {
        return Vector3Base(X - b, Y - b, Z - b);
    }

    Vector3Base operator*(T b) const
    {
        return Vector3Base(X * b, Y * b, Z * b);
    }

    Vector3Base operator/(T b) const
    {
        return Vector3Base(X / b, Y / b, Z / b);
    }

    Vector3Base operator+(typename TOtherFloat<T>::Type a) const
    {
        T b = (T)a;
        return Vector3Base(X + b, Y + b, Z + b);
    }

    Vector3Base operator-(typename TOtherFloat<T>::Type a) const
    {
        T b = (T)a;
        return Vector3Base(X - b, Y - b, Z - b);
    }

    Vector3Base operator*(typename TOtherFloat<T>::Type a) const
    {
        T b = (T)a;
        return Vector3Base(X * (T)b, Y * b, Z * b);
    }

    Vector3Base operator/(typename TOtherFloat<T>::Type a) const
    {
        T b = (T)a;
        return Vector3Base(X / b, Y / b, Z / b);
    }

    Vector3Base operator^(const Vector3Base& b) const
    {
        return Cross(*this, b);
    }

    T operator|(const Vector3Base& b) const
    {
        return Dot(*this, b);
    }

    Vector3Base& operator+=(const Vector3Base& b)
    {
        X += b.X;
        Y += b.Y;
        Z += b.Z;
        return *this;
    }

    Vector3Base& operator-=(const Vector3Base& b)
    {
        X -= b.X;
        Y -= b.Y;
        Z -= b.Z;
        return *this;
    }

    Vector3Base& operator*=(const Vector3Base& b)
    {
        X *= b.X;
        Y *= b.Y;
        Z *= b.Z;
        return *this;
    }

    Vector3Base& operator/=(const Vector3Base& b)
    {
        X /= b.X;
        Y /= b.Y;
        Z /= b.Z;
        return *this;
    }

    Vector3Base& operator+=(T b)
    {
        X += b;
        Y += b;
        Z += b;
        return *this;
    }

    Vector3Base& operator-=(T b)
    {
        X -= b;
        Y -= b;
        Z -= b;
        return *this;
    }

    Vector3Base& operator*=(T b)
    {
        X *= b;
        Y *= b;
        Z *= b;
        return *this;
    }

    Vector3Base& operator/=(T b)
    {
        X /= b;
        Y /= b;
        Z /= b;
        return *this;
    }

    bool operator==(const Vector3Base& b) const
    {
        return X == b.X && Y == b.Y && Z == b.Z;
    }

    bool operator!=(const Vector3Base& b) const
    {
        return X != b.X || Y != b.Y || Z != b.Z;
    }

    bool operator>(const Vector3Base& b) const
    {
        return X > b.X && Y > b.Y && Z > b.Z;
    }

    bool operator>=(const Vector3Base& b) const
    {
        return X >= b.X && Y >= b.Y && Z >= b.Z;
    }

    bool operator<(const Vector3Base& b) const
    {
        return X < b.X && Y < b.Y && Z < b.Z;
    }

    bool operator<=(const Vector3Base& b) const
    {
        return X <= b.X && Y <= b.Y && Z <= b.Z;
    }

public:
    static bool NearEqual(const Vector3Base& a, const Vector3Base& b)
    {
        return Math::NearEqual(a.X, b.X) && Math::NearEqual(a.Y, b.Y) && Math::NearEqual(a.Z, b.Z);
    }

    static bool NearEqual(const Vector3Base& a, const Vector3Base& b, T epsilon)
    {
        return Math::NearEqual(a.X, b.X, epsilon) && Math::NearEqual(a.Y, b.Y, epsilon) && Math::NearEqual(a.Z, b.Z, epsilon);
    }

public:
    static void Add(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
    }

    static void Subtract(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
    }

    static void Multiply(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
    }

    static void Divide(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.X / b.X, a.Y / b.Y, a.Z / b.Z);
    }

    static void Min(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }

    static void Max(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

public:
    static Vector3Base Min(const Vector3Base& a, const Vector3Base& b)
    {
        return Vector3Base(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }

    static Vector3Base Max(const Vector3Base& a, const Vector3Base& b)
    {
        return Vector3Base(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    static Vector3Base Mod(const Vector3Base& a, const Vector3Base& b)
    {
        return Vector3Base(Math::Mod(a.X, b.X), Math::Mod(a.Y, b.Y), Math::Mod(a.Z, b.Z));
    }

    static Vector3Base Floor(const Vector3Base& v)
    {
        return Vector3Base(Math::Floor(v.X), Math::Floor(v.Y), Math::Floor(v.Z));
    }

    static Vector3Base Frac(const Vector3Base& v)
    {
        return Vector3Base(v.X - (int32)v.X, v.Y - (int32)v.Y, v.Z - (int32)v.Z);
    }

    static Vector3Base Round(const Vector3Base& v)
    {
        return Vector3Base(Math::Round(v.X), Math::Round(v.Y), Math::Round(v.Z));
    }

    static Vector3Base Ceil(const Vector3Base& v)
    {
        return Vector3Base(Math::Ceil(v.X), Math::Ceil(v.Y), Math::Ceil(v.Z));
    }

    static Vector3Base Abs(const Vector3Base& v)
    {
        return Vector3Base(Math::Abs(v.X), Math::Abs(v.Y), Math::Abs(v.Z));
    }

public:
    // Restricts a value to be within a specified range (inclusive min/max).
    static Vector3Base Clamp(const Vector3Base& v, const Vector3Base& min, const Vector3Base& max)
    {
        Vector3Base result;
        Clamp(v, min, max, result);
        return result;
    }

    // Restricts a value to be within a specified range (inclusive min/max).
    static void Clamp(const Vector3Base& v, const Vector3Base& min, const Vector3Base& max, Vector3Base& result)
    {
        result = Vector3Base(Math::Clamp(v.X, min.X, max.X), Math::Clamp(v.Y, min.Y, max.Y), Math::Clamp(v.Z, min.Z, max.Z));
    }

    /// <summary>
    /// Makes sure that Length of the output vector is always below max and above 0.
    /// </summary>
    /// <param name="v">Input Vector.</param>
    /// <param name="max">Max Length</param>
    static Vector3Base ClampLength(const Vector3Base& v, float max)
    {
        return ClampLength(v, 0, max);
    }

    /// <summary>
    /// Makes sure that Length of the output vector is always below max and above min.
    /// </summary>
    /// <param name="v">Input Vector.</param>
    /// <param name="min">Min Length</param>
    /// <param name="max">Max Length</param>
    static Vector3Base ClampLength(const Vector3Base& v, float min, float max)
    {
        Vector3Base result;
        ClampLength(v, min, max, result);
        return result;
    }

    /// <summary>
    /// Makes sure that Length of the output vector is always below max and above min.
    /// </summary>
    /// <param name="v">Input Vector.</param>
    /// <param name="min">Min Length</param>
    /// <param name="max">Max Length</param>
    /// <param name="result">The result vector.</param>
    static void ClampLength(const Vector3Base& v, float min, float max, Vector3Base& result)
    {
        result = v;
        T lenSq = result.LengthSquared();
        if (lenSq > max * max)
        {
            T scaleFactor = max / (T)Math::Sqrt(lenSq);
            result.X *= scaleFactor;
            result.Y *= scaleFactor;
            result.Z *= scaleFactor;
        }
        if (lenSq < min * min)
        {
            T scaleFactor = min / (T)Math::Sqrt(lenSq);
            result.X *= scaleFactor;
            result.Y *= scaleFactor;
            result.Z *= scaleFactor;
        }
    }

    // Calculates the distance between two vectors.
    static T Distance(const Vector3Base& a, const Vector3Base& b)
    {
        const T x = a.X - b.X;
        const T y = a.Y - b.Y;
        const T z = a.Z - b.Z;
        return Math::Sqrt(x * x + y * y + z * z);
    }

    // Calculates the squared distance between two vectors.
    static T DistanceSquared(const Vector3Base& a, const Vector3Base& b)
    {
        const T x = a.X - b.X;
        const T y = a.Y - b.Y;
        const T z = a.Z - b.Z;
        return x * x + y * y + z * z;
    }

    // Performs vector normalization (scales vector up to unit length).
    static Vector3Base Normalize(const Vector3Base& v)
    {
        Vector3Base r = v;
        const T length = Math::Sqrt(r.X * r.X + r.Y * r.Y + r.Z * r.Z);
        if (length >= ZeroTolerance)
        {
            const T inv = (T)1.0f / length;
            r.X *= inv;
            r.Y *= inv;
            r.Z *= inv;
        }
        return r;
    }

    // Performs vector normalization (scales vector up to unit length). This is a faster version that does not perform check for length equal 0 (it assumes that input vector is not empty).
    static Vector3Base NormalizeFast(const Vector3Base& v)
    {
        const T inv = 1.0f / v.Length();
        return Vector3Base(v.X * inv, v.Y * inv, v.Z * inv);
    }

    // Performs vector normalization (scales vector up to unit length).
    static FORCE_INLINE void Normalize(const Vector3Base& input, Vector3Base& result)
    {
        result = Normalize(input);
    }

    // Calculates the dot product of two vectors.
    FORCE_INLINE static T Dot(const Vector3Base& a, const Vector3Base& b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    // Calculates the cross product of two vectors.
    static void Cross(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
    {
        result = Vector3Base(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
    }

    // Calculates the cross product of two vectors.
    static Vector3Base Cross(const Vector3Base& a, const Vector3Base& b)
    {
        return Vector3Base(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
    }

    // Performs a linear interpolation between two vectors.
    static void Lerp(const Vector3Base& start, const Vector3Base& end, T amount, Vector3Base& result)
    {
        result.X = Math::Lerp(start.X, end.X, amount);
        result.Y = Math::Lerp(start.Y, end.Y, amount);
        result.Z = Math::Lerp(start.Z, end.Z, amount);
    }

    // Performs a linear interpolation between two vectors.
    static Vector3Base Lerp(const Vector3Base& start, const Vector3Base& end, T amount)
    {
        Vector3Base result;
        Lerp(start, end, amount, result);
        return result;
    }

    // Performs a cubic interpolation between two vectors.
    static void SmoothStep(const Vector3Base& start, const Vector3Base& end, T amount, Vector3Base& result)
    {
        amount = Math::SmoothStep(amount);
        Lerp(start, end, amount, result);
    }

    /// <summary>
    /// Moves a value current towards target.
    /// </summary>
    /// <param name="current">The position to move from.</param>
    /// <param name="target">The position to move towards.</param>
    /// <param name="maxDistanceDelta">The maximum distance that can be applied to the value.</param>
    /// <returns>The new position.</returns>
    static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta)
    {
        const Vector3Base to = target - current;
        const T distanceSq = to.LengthSquared();
        if (distanceSq == 0 || (maxDistanceDelta >= 0 && distanceSq <= maxDistanceDelta * maxDistanceDelta))
            return target;
        const T scale = maxDistanceDelta / Math::Sqrt(distanceSq);
        return Vector3Base(current.X + to.X * scale, current.Y + to.Y * scale, current.Z + to.Z * scale);
    }

    // Performs a Hermite spline interpolation.
    static FLAXENGINE_API void Hermite(const Vector3Base& value1, const Vector3Base& tangent1, const Vector3Base& value2, const Vector3Base& tangent2, T amount, Vector3Base& result);

    // Returns the reflection of a vector off a surface that has the specified normal.
    static FLAXENGINE_API void Reflect(const Vector3Base& vector, const Vector3Base& normal, Vector3Base& result);

    // Transforms a 3D vector by the given Quaternion rotation.
    static FLAXENGINE_API void Transform(const Vector3Base& vector, const Quaternion& rotation, Vector3Base& result);

    // Transforms a 3D vector by the given Quaternion rotation.
    static FLAXENGINE_API Vector3Base Transform(const Vector3Base& vector, const Quaternion& rotation);

    // Transforms a 3D vector by the given matrix.
    static FLAXENGINE_API void Transform(const Vector3Base& vector, const Matrix& transform, Vector3Base& result);

    // Transforms a 3D vector by the given matrix.
    static FLAXENGINE_API void Transform(const Vector3Base& vector, const Matrix3x3& transform, Vector3Base& result);

    // Transforms a 3D vector by the given transformation.
    static FLAXENGINE_API void Transform(const Vector3Base& vector, const ::Transform& transform, Vector3Base& result);

    // Transforms a 3D vector by the given matrix.
    static FLAXENGINE_API Vector3Base Transform(const Vector3Base& vector, const Matrix& transform);

    // Transforms a 3D vector by the given transformation.
    static FLAXENGINE_API Vector3Base Transform(const Vector3Base& vector, const ::Transform& transform);

    // Transforms a 3D vector by the given matrix.
    static FLAXENGINE_API void Transform(const Vector3Base& vector, const Matrix& transform, Vector4Base<T>& result);

    // Performs a coordinate transformation using the given matrix.
    static FLAXENGINE_API void TransformCoordinate(const Vector3Base& coordinate, const Matrix& transform, Vector3Base& result);

    // Performs a normal transformation using the given matrix.
    static FLAXENGINE_API void TransformNormal(const Vector3Base& normal, const Matrix& transform, Vector3Base& result);

    /// <summary>
    /// Projects a vector onto another vector.
    /// </summary>
    /// <param name="vector">The vector to project.</param>
    /// <param name="onNormal">The projection normal vector.</param>
    /// <returns>The projected vector.</returns>
    static FLAXENGINE_API Vector3Base Project(const Vector3Base& vector, const Vector3Base& onNormal);

    /// <summary>
    /// Projects a vector onto a plane defined by a normal orthogonal to the plane.
    /// </summary>
    /// <param name="vector">The vector to project.</param>
    /// <param name="planeNormal">The plane normal vector.</param>
    /// <returns>The projected vector.</returns>
    static Vector3Base ProjectOnPlane(const Vector3Base& vector, const Vector3Base& planeNormal)
    {
        return vector - Project(vector, planeNormal);
    }

    // Projects a 3D vector from object space into screen space using the provided viewport and transformation matrix.
    static FLAXENGINE_API void Project(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3Base& result);

    // Projects a 3D vector from object space into screen space using the provided viewport and transformation matrix.
    static Vector3Base Project(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection)
    {
        Vector3Base result;
        Project(vector, x, y, width, height, minZ, maxZ, worldViewProjection, result);
        return result;
    }

    // Projects a 3D vector from screen space into object space using the provided viewport and transformation matrix.
    static FLAXENGINE_API void Unproject(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3Base& result);

    // Projects a 3D vector from screen space into object space using the provided viewport and transformation matrix.
    static Vector3Base Unproject(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection)
    {
        Vector3Base result;
        Unproject(vector, x, y, width, height, minZ, maxZ, worldViewProjection, result);
        return result;
    }

    /// <summary>
    /// Creates an orthonormal basis from a basis with at least two orthogonal vectors.
    /// </summary>
    /// <param name="xAxis">The X axis.</param>
    /// <param name="yAxis">The y axis.</param>
    /// <param name="zAxis">The z axis.</param>
    static FLAXENGINE_API void CreateOrthonormalBasis(Vector3Base& xAxis, Vector3Base& yAxis, Vector3Base& zAxis);

    /// <summary>
    /// Finds the best arbitrary axis vectors to represent U and V axes of a plane, by using this vector as the normal of the plane.
    /// </summary>
    /// <param name="firstAxis">The reference to first axis.</param>
    /// <param name="secondAxis">The reference to second axis.</param>
    FLAXENGINE_API void FindBestAxisVectors(Vector3Base& firstAxis, Vector3Base& secondAxis) const;

    /// <summary>
    /// Calculates the area of the triangle.
    /// </summary>
    /// <param name="v0">The first triangle vertex.</param>
    /// <param name="v1">The second triangle vertex.</param>
    /// <param name="v2">The third triangle vertex.</param>
    /// <returns>The triangle area.</returns>
    static FLAXENGINE_API T TriangleArea(const Vector3Base& v0, const Vector3Base& v1, const Vector3Base& v2);

    /// <summary>
    /// Calculates the angle (in degrees) between from and to vectors. This is always the smallest value.
    /// </summary>
    /// <param name="from">The first vector.</param>
    /// <param name="to">The second vector.</param>
    /// <returns>The angle (in degrees).</returns>
    static FLAXENGINE_API T Angle(const Vector3Base& from, const Vector3Base& to);

    /// <summary>
    /// Calculates the signed angle (in degrees) between from and to vectors. This is always the smallest value. The sign of the result depends on: the order of input vectors, and the direction of the axis vector.
    /// </summary>
    /// <param name="from">The first vector.</param>
    /// <param name="to">The second vector.</param>
    /// <param name="axis">The axis around which the vectors are rotated.</param>
    /// <returns>The angle (in degrees).</returns>
    static FLAXENGINE_API T SignedAngle(const Vector3Base& from, const Vector3Base& to, const Vector3Base& axis);

    /// <summary>
    /// Snaps the input position onto the grid.
    /// </summary>
    /// <param name="pos">The position to snap.</param>
    /// <param name="gridSize">The size of the grid.</param>
    /// <returns>The position snapped to the grid.</returns>
    static FLAXENGINE_API Vector3Base SnapToGrid(const Vector3Base& pos, const Vector3Base& gridSize);

    /// <summary>
    /// Snaps the <paramref name="point"/> onto the rotated grid. For world aligned grid snapping use <b><see cref="SnapToGrid"/></b> instead.
    /// </summary>
    /// <param name="point">The position to snap.</param>
    /// <param name="gridSize">The size of the grid.</param>
    /// <param name="gridOrigin">The center point of the grid.</param>
    /// <param name="gridOrientation">The rotation of the grid.</param>
    /// <param name="offset">The local position offset applied to the snapped position before grid rotation.</param>
    /// <returns>The position snapped to the grid.</returns>
    static FLAXENGINE_API Vector3Base SnapToGrid(const Vector3Base& point, const Vector3Base& gridSize, const Quaternion& gridOrientation, const Vector3Base& gridOrigin = Zero, const Vector3Base& offset = Zero);
};

template<typename T>
inline Vector3Base<T> operator+(T a, const Vector3Base<T>& b)
{
    return b + a;
}

template<typename T>
inline Vector3Base<T> operator-(T a, const Vector3Base<T>& b)
{
    return Vector3Base<T>(a) - b;
}

template<typename T>
inline Vector3Base<T> operator*(T a, const Vector3Base<T>& b)
{
    return b * a;
}

template<typename T>
inline Vector3Base<T> operator/(T a, const Vector3Base<T>& b)
{
    return Vector3Base<T>(a) / b;
}

template<typename T>
inline Vector3Base<T> operator+(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
{
    return b + a;
}

template<typename T>
inline Vector3Base<T> operator-(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
{
    return Vector3Base<T>(a) - b;
}

template<typename T>
inline Vector3Base<T> operator*(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
{
    return b * a;
}

template<typename T>
inline Vector3Base<T> operator/(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
{
    return Vector3Base<T>(a) / b;
}

template<typename T>
inline uint32 GetHash(const Vector3Base<T>& key)
{
    return (((*(uint32*)&key.X * 397) ^ *(uint32*)&key.Y) * 397) ^ *(uint32*)&key.Z;
}

namespace Math
{
    template<typename T>
    FORCE_INLINE static bool NearEqual(const Vector3Base<T>& a, const Vector3Base<T>& b)
    {
        return Vector3Base<T>::NearEqual(a, b);
    }

    template<typename T>
    FORCE_INLINE static Vector3Base<T> UnwindDegrees(const Vector3Base<T>& v)
    {
        return Vector3Base<T>(UnwindDegrees(v.X), UnwindDegrees(v.Y), UnwindDegrees(v.Z));
    }
}

template<>
struct TIsPODType<Float3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Float3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);

template<>
struct TIsPODType<Double3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Double3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);

template<>
struct TIsPODType<Int3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);

#if !defined(_MSC_VER) || defined(__clang__)
// Forward specializations for Clang
template<> FLAXENGINE_API const Float3 Float3::Zero;
template<> FLAXENGINE_API const Float3 Float3::One;
template<> FLAXENGINE_API const Float3 Float3::Half;
template<> FLAXENGINE_API const Float3 Float3::UnitX;
template<> FLAXENGINE_API const Float3 Float3::UnitY;
template<> FLAXENGINE_API const Float3 Float3::UnitZ;
template<> FLAXENGINE_API const Float3 Float3::Up;
template<> FLAXENGINE_API const Float3 Float3::Down;
template<> FLAXENGINE_API const Float3 Float3::Left;
template<> FLAXENGINE_API const Float3 Float3::Right;
template<> FLAXENGINE_API const Float3 Float3::Forward;
template<> FLAXENGINE_API const Float3 Float3::Backward;
template<> FLAXENGINE_API const Float3 Float3::Minimum;
template<> FLAXENGINE_API const Float3 Float3::Maximum;
template<> FLAXENGINE_API ScriptingTypeInitializer Float3::TypeInitializer;

template<> FLAXENGINE_API const Double3 Double3::Zero;
template<> FLAXENGINE_API const Double3 Double3::One;
template<> FLAXENGINE_API const Double3 Double3::Half;
template<> FLAXENGINE_API const Double3 Double3::UnitX;
template<> FLAXENGINE_API const Double3 Double3::UnitY;
template<> FLAXENGINE_API const Double3 Double3::UnitZ;
template<> FLAXENGINE_API const Double3 Double3::Up;
template<> FLAXENGINE_API const Double3 Double3::Down;
template<> FLAXENGINE_API const Double3 Double3::Left;
template<> FLAXENGINE_API const Double3 Double3::Right;
template<> FLAXENGINE_API const Double3 Double3::Forward;
template<> FLAXENGINE_API const Double3 Double3::Backward;
template<> FLAXENGINE_API const Double3 Double3::Minimum;
template<> FLAXENGINE_API const Double3 Double3::Maximum;
template<> FLAXENGINE_API ScriptingTypeInitializer Double3::TypeInitializer;

template<> FLAXENGINE_API const Int3 Int3::Zero;
template<> FLAXENGINE_API const Int3 Int3::One;
template<> FLAXENGINE_API const Int3 Int3::Half;
template<> FLAXENGINE_API const Int3 Int3::UnitX;
template<> FLAXENGINE_API const Int3 Int3::UnitY;
template<> FLAXENGINE_API const Int3 Int3::UnitZ;
template<> FLAXENGINE_API const Int3 Int3::Up;
template<> FLAXENGINE_API const Int3 Int3::Down;
template<> FLAXENGINE_API const Int3 Int3::Left;
template<> FLAXENGINE_API const Int3 Int3::Right;
template<> FLAXENGINE_API const Int3 Int3::Forward;
template<> FLAXENGINE_API const Int3 Int3::Backward;
template<> FLAXENGINE_API const Int3 Int3::Minimum;
template<> FLAXENGINE_API const Int3 Int3::Maximum;
template<> FLAXENGINE_API ScriptingTypeInitializer Int3::TypeInitializer;
#endif
