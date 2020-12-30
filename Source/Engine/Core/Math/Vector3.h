// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Quaternion;
struct Matrix;
struct Vector2;
struct Vector4;
struct Color;
class String;

/// <summary>
/// Represents a three dimensional mathematical vector.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Vector3
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Vector3);
public:

    union
    {
        struct
        {
            /// <summary>
            /// The X component.
            /// </summary>
            API_FIELD() float X;

            /// <summary>
            /// The Y component.
            /// </summary>
            API_FIELD() float Y;

            /// <summary>
            /// The Z component.
            /// </summary>
            API_FIELD() float Z;
        };

        // Raw values
        float Raw[3];
    };

public:

    // Vector with all components equal zero (0, 0, 0)
    static const Vector3 Zero;

    // Vector with all components equal one (1, 1, 1)
    static const Vector3 One;

    // Vector with all components equal half (0.5, 0.5, 0.5)
    static const Vector3 Half;

    // The X unit vector (1, 0, 0)
    static const Vector3 UnitX;

    // The Y unit vector (0, 1, 0)
    static const Vector3 UnitY;

    // The Z unit vector (0, 0, 1)
    static const Vector3 UnitZ;

    // A unit vector designating up (0, 1, 0)
    static const Vector3 Up;

    // A unit vector designating down (0, -1, 0)
    static const Vector3 Down;

    // A unit vector designating a (-1, 0, 0)
    static const Vector3 Left;

    // A unit vector designating b (1, 0, 0)
    static const Vector3 Right;

    // A unit vector designating forward in a a-handed coordinate system (0, 0, 1)
    static const Vector3 Forward;

    // A unit vector designating backward in a a-handed coordinate system (0, 0, -1)
    static const Vector3 Backward;

    // A minimum Vector3
    static const Vector3 Minimum;

    // A maximum Vector3
    static const Vector3 Maximum;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    Vector3()
    {
    }

    // Init
    // @param xyz Value to assign to the all components
    Vector3(float xyz)
        : X(xyz)
        , Y(xyz)
        , Z(xyz)
    {
    }

    // Init
    // @param x X component value
    // @param y Y component value
    // @param z Z component value
    Vector3(float x, float y, float z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="v">X, Y and Z components in an array</param>
    explicit Vector3(const float* xyz)
        : X(xyz[0])
        , Y(xyz[1])
        , Z(xyz[2])
    {
    }

    // Init
    // @param xy Vector2 with X and Y components values
    // @param z Z component value
    Vector3(const Vector2& xy, float z);

    // Init
    // @param xy Vector3 value
    explicit Vector3(const Vector2& xy);

    // Init
    // @param xyz Int3 value
    explicit Vector3(const Int3& xyz);

    // Init
    // @param xyz Vector4 value
    explicit Vector3(const Vector4& xyz);

    // Init
    // @param color Color value
    explicit Vector3(const Color& color);

public:

    String ToString() const;

public:

    // Gets a value indicting whether this instance is normalized
    bool IsNormalized() const
    {
        return Math::IsOne(X * X + Y * Y + Z * Z);
    }

    // Gets a value indicting whether this vector is zero
    bool IsZero() const
    {
        return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z);
    }

    // Gets a value indicting whether any vector component is zero
    bool IsAnyZero() const
    {
        return Math::IsZero(X) || Math::IsZero(Y) || Math::IsZero(Z);
    }

    // Gets a value indicting whether this vector is one
    bool IsOne() const
    {
        return Math::IsOne(X) && Math::IsOne(Y) && Math::IsOne(Z);
    }

    // Calculates length of the vector
    // @returns Length of the vector
    float Length() const
    {
        return Math::Sqrt(X * X + Y * Y + Z * Z);
    }

    // Calculates the squared length of the vector
    // @returns The squared length of the vector
    float LengthSquared() const
    {
        return X * X + Y * Y + Z * Z;
    }

    // Calculates inverted length of the vector (1 / Length())
    float InvLength() const
    {
        return 1.0f / Length();
    }

    /// <summary>
    /// Calculates a vector with values being absolute values of that vector
    /// </summary>
    /// <returns>Absolute vector</returns>
    Vector3 GetAbsolute() const
    {
        return Vector3(Math::Abs(X), Math::Abs(Y), Math::Abs(Z));
    }

    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector
    /// </summary>
    /// <returns>Negative vector</returns>
    Vector3 GetNegative() const
    {
        return Vector3(-X, -Y, -Z);
    }

    /// <summary>
    /// Calculates a normalized vector that has length equal to 1.
    /// </summary>
    /// <returns>The normalized vector.</returns>
    Vector3 GetNormalized() const
    {
        const float rcp = 1.0f / Length();
        return Vector3(X * rcp, Y * rcp, Z * rcp);
    }

    /// <summary>
    /// Returns average arithmetic of all the components
    /// </summary>
    /// <returns>Average arithmetic of all the components</returns>
    float AverageArithmetic() const
    {
        return (X + Y + Z) * 0.333333334f;
    }

    /// <summary>
    /// Gets sum of all vector components values
    /// </summary>
    /// <returns>Sum of X,Y and Z</returns>
    float SumValues() const
    {
        return X + Y + Z;
    }

    /// <summary>
    /// Returns minimum value of all the components
    /// </summary>
    /// <returns>Minimum value</returns>
    float MinValue() const
    {
        return Math::Min(X, Y, Z);
    }

    /// <summary>
    /// Returns maximum value of all the components
    /// </summary>
    /// <returns>Maximum value</returns>
    float MaxValue() const
    {
        return Math::Max(X, Y, Z);
    }

    /// <summary>
    /// Returns true if vector has one or more components is not a number (NaN)
    /// </summary>
    /// <returns>True if one or more components is not a number (NaN)</returns>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y) || isnan(Z);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity</returns>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y) || isinf(Z);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity or NaN
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity or NaN</returns>
    bool IsNanOrInfinity() const
    {
        return IsInfinity() || IsNaN();
    }

public:

    /// <summary>
    /// Performs vector normalization (scales vector up to unit length)
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
        }
    }

    /// <summary>
    /// Performs fast vector normalization (scales vector up to unit length)
    /// </summary>
    void NormalizeFast()
    {
        const float inv = 1.0f / Length();
        X *= inv;
        Y *= inv;
        Z *= inv;
    }

    /// <summary>
    /// Sets all vector components to the absolute values
    /// </summary>
    void Absolute()
    {
        X = Math::Abs(X);
        Y = Math::Abs(Y);
        Z = Math::Abs(Z);
    }

    /// <summary>
    /// Negates all components of that vector
    /// </summary>
    void Negate()
    {
        X = -X;
        Y = -Y;
        Z = -Z;
    }

    /// <summary>
    /// When this vector contains Euler angles (degrees), ensure that angles are between +/-180
    /// </summary>
    void UnwindEuler();

public:

    // Arithmetic operators with Vector3
    Vector3 operator+(const Vector3& b) const
    {
        return Add(*this, b);
    }

    Vector3 operator-(const Vector3& b) const
    {
        return Subtract(*this, b);
    }

    Vector3 operator*(const Vector3& b) const
    {
        return Multiply(*this, b);
    }

    Vector3 operator/(const Vector3& b) const
    {
        return Divide(*this, b);
    }

    Vector3 operator-() const
    {
        return Vector3(-X, -Y, -Z);
    }

    Vector3 operator^(const Vector3& b) const
    {
        return Cross(*this, b);
    }

    float operator|(const Vector3& b) const
    {
        return Dot(*this, b);
    }

    // op= operators with Vector3
    Vector3& operator+=(const Vector3& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Vector3& operator-=(const Vector3& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Vector3& operator*=(const Vector3& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Vector3& operator/=(const Vector3& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with float
    Vector3 operator+(float b) const
    {
        return Add(*this, b);
    }

    Vector3 operator-(float b) const
    {
        return Subtract(*this, b);
    }

    Vector3 operator*(float b) const
    {
        return Multiply(*this, b);
    }

    Vector3 operator/(float b) const
    {
        return Divide(*this, b);
    }

    // op= operators with float
    Vector3& operator+=(float b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Vector3& operator-=(float b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Vector3& operator*=(float b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Vector3& operator/=(float b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison operators
    bool operator==(const Vector3& b) const
    {
        return X == b.X && Y == b.Y && Z == b.Z;
    }

    bool operator!=(const Vector3& b) const
    {
        return X != b.X || Y != b.Y || Z != b.Z;
    }

    bool operator>(const Vector3& b) const
    {
        return X > b.X && Y > b.Y && Z > b.Z;
    }

    bool operator>=(const Vector3& b) const
    {
        return X >= b.X && Y >= b.Y && Z >= b.Z;
    }

    bool operator<(const Vector3& b) const
    {
        return X < b.X && Y < b.Y && Z < b.Z;
    }

    bool operator<=(const Vector3& b) const
    {
        return X <= b.X && Y <= b.Y && Z <= b.Z;
    }

public:

    static bool NearEqual(const Vector3& a, const Vector3& b)
    {
        return Math::NearEqual(a.X, b.X) && Math::NearEqual(a.Y, b.Y) && Math::NearEqual(a.Z, b.Z);
    }

    static bool NearEqual(const Vector3& a, const Vector3& b, float epsilon)
    {
        return Math::NearEqual(a.X, b.X, epsilon) && Math::NearEqual(a.Y, b.Y, epsilon) && Math::NearEqual(a.Z, b.Z, epsilon);
    }

public:

    static void Add(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
        result.Z = a.Z + b.Z;
    }

    static Vector3 Add(const Vector3& a, const Vector3& b)
    {
        Vector3 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
        result.Z = a.Z - b.Z;
    }

    static Vector3 Subtract(const Vector3& a, const Vector3& b)
    {
        Vector3 result;
        Subtract(a, b, result);
        return result;
    }

    static Vector3 Multiply(const Vector3& a, const Vector3& b)
    {
        return Vector3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
    }

    static void Multiply(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result = Vector3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
    }

    static Vector3 Multiply(const Vector3& a, float b)
    {
        return Vector3(a.X * b, a.Y * b, a.Z * b);
    }

    static Vector3 Divide(const Vector3& a, const Vector3& b)
    {
        return Vector3(a.X / b.X, a.Y / b.Y, a.Z / b.Z);
    }

    static void Divide(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result = Vector3(a.X / b.X, a.Y / b.Y, a.Z / b.Z);
    }

    static Vector3 Divide(const Vector3& a, float b)
    {
        return Vector3(a.X / b, a.Y / b, a.Z / b);
    }

    static Vector3 Floor(const Vector3& v);
    static Vector3 Frac(const Vector3& v);

    static float ScalarProduct(const Vector3& a, const Vector3& b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

public:

    // Restricts a value to be within a specified range
    // @param value The value to clamp
    // @param min The minimum value,
    // @param max The maximum value
    // @returns Clamped value
    static Vector3 Clamp(const Vector3& value, const Vector3& min, const Vector3& max);

    // Restricts a value to be within a specified range
    // @param value The value to clamp
    // @param min The minimum value,
    // @param max The maximum value
    // @param result When the method completes, contains the clamped value
    static void Clamp(const Vector3& value, const Vector3& min, const Vector3& max, Vector3& result);

    // Calculates the distance between two vectors
    // @param value1 The first vector
    // @param value2 The second vector
    // @returns The distance between the two vectors
    static float Distance(const Vector3& value1, const Vector3& value2);

    // Calculates the squared distance between two vectors
    // @param value1 The first vector
    // @param value2 The second vector
    // @returns The squared distance between the two vectors
    static float DistanceSquared(const Vector3& value1, const Vector3& value2);

    // Performs vector normalization (scales vector up to unit length)
    // @param inout Input vector to normalize
    // @returns Output vector that is normalized (has unit length)
    static Vector3 Normalize(const Vector3& input);

    // Performs vector normalization (scales vector up to unit length). This is a faster version that does not performs check for length equal 0 (it assumes that input vector is not empty).
    // @param inout Input vector to normalize (cannot be zero).
    // @returns Output vector that is normalized (has unit length)
    static Vector3 NormalizeFast(const Vector3& input)
    {
        const float inv = 1.0f / input.Length();
        return Vector3(input.X * inv, input.Y * inv, input.Z * inv);
    }

    // Performs vector normalization (scales vector up to unit length)
    // @param inout Input vector to normalize
    // @param output Output vector that is normalized (has unit length)
    static void Normalize(const Vector3& input, Vector3& result);

    // dot product with another vector
    static float Dot(const Vector3& a, const Vector3& b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    // Calculates the cross product of two vectors
    // @param a First source vector
    // @param b Second source vector
    // @param result When the method completes, contains the cross product of the two vectors
    static void Cross(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result = Vector3(
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X);
    }

    // Calculates the cross product of two vectors
    // @param a First source vector
    // @param b Second source vector
    // @returns Cross product of the two vectors
    static Vector3 Cross(const Vector3& a, const Vector3& b)
    {
        return Vector3(
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X);
    }

    // Performs a linear interpolation between two vectors
    // @param start Start vector
    // @param end End vector
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the linear interpolation of the two vectors
    static void Lerp(const Vector3& start, const Vector3& end, float amount, Vector3& result)
    {
        result.X = Math::Lerp(start.X, end.X, amount);
        result.Y = Math::Lerp(start.Y, end.Y, amount);
        result.Z = Math::Lerp(start.Z, end.Z, amount);
    }

    // <summary>
    // Performs a linear interpolation between two vectors.
    // </summary>
    // @param start Start vector,
    // @param end End vector,
    // @param amount Value between 0 and 1 indicating the weight of @paramref end"/>,
    // @returns The linear interpolation of the two vectors
    static Vector3 Lerp(const Vector3& start, const Vector3& end, float amount)
    {
        Vector3 result;
        Lerp(start, end, amount, result);
        return result;
    }

    // Performs a cubic interpolation between two vectors
    // @param start Start vector
    // @param end End vector
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the cubic interpolation of the two vectors
    static void SmoothStep(const Vector3& start, const Vector3& end, float amount, Vector3& result)
    {
        amount = Math::SmoothStep(amount);
        Lerp(start, end, amount, result);
    }

    // Performs a Hermite spline interpolation.
    // @param value1 First source position vector
    // @param tangent1 First source tangent vector
    // @param value2 Second source position vector
    // @param tangent2 Second source tangent vector
    // @param amount Weighting factor,
    // @param result When the method completes, contains the result of the Hermite spline interpolation,
    static void Hermite(const Vector3& value1, const Vector3& tangent1, const Vector3& value2, const Vector3& tangent2, float amount, Vector3& result);

    // Returns the reflection of a vector off a surface that has the specified normal
    // @param vector The source vector
    // @param normal Normal of the surface
    // @param result When the method completes, contains the reflected vector
    static void Reflect(const Vector3& vector, const Vector3& normal, Vector3& result);

    // Transforms a 3D vector by the given Quaternion rotation
    // @param vector The vector to rotate
    // @param rotation The Quaternion rotation to apply
    // @param result When the method completes, contains the transformed Vector4
    static void Transform(const Vector3& vector, const Quaternion& rotation, Vector3& result);

    // Transforms a 3D vector by the given Quaternion rotation
    // @param vector The vector to rotate
    // @param rotation The Quaternion rotation to apply
    // @returns The transformed Vector4
    static Vector3 Transform(const Vector3& vector, const Quaternion& rotation);

    // Transforms a 3D vector by the given matrix
    // @param vector The source vector
    // @param transform The transformation matrix
    // @param result When the method completes, contains the transformed Vector3
    static void Transform(const Vector3& vector, const Matrix& transform, Vector3& result);

    // Transforms a 3D vectors by the given matrix
    // @param vectors The source vectors
    // @param transform The transformation matrix
    // @param results When the method completes, contains the transformed Vector3s
    // @param vectorsCount Amount of vectors to transform
    static void Transform(const Vector3* vectors, const Matrix& transform, Vector3* results, int32 vectorsCount);

    // Transforms a 3D vector by the given matrix
    // @param vector The source vector
    // @param transform The transformation matrix
    // @returns Transformed Vector3
    static Vector3 Transform(const Vector3& vector, const Matrix& transform);

    // Transforms a 3D vector by the given matrix
    // @param vector The source vector
    // @param transform The transformation matrix
    // @param result When the method completes, contains the transformed Vector4
    static void Transform(const Vector3& vector, const Matrix& transform, Vector4& result);

    // Performs a coordinate transformation using the given matrix
    // @param coordinate The coordinate vector to transform
    // @param transform The transformation matrix
    // @param result When the method completes, contains the transformed coordinates
    static void TransformCoordinate(const Vector3& coordinate, const Matrix& transform, Vector3& result);

    // Performs a normal transformation using the given matrix
    // @param normal The normal vector to transform
    // @param transform The transformation matrix
    // @param result When the method completes, contains the transformed normal
    static void TransformNormal(const Vector3& normal, const Matrix& transform, Vector3& result);

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the largest components of the source vectors
    static Vector3 Max(const Vector3& a, const Vector3& b)
    {
        return Vector3(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
    static Vector3 Min(const Vector3& a, const Vector3& b)
    {
        return Vector3(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the largest components of the source vectors
    static void Max(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result = Vector3(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
    static void Min(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result = Vector3(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }

    /// <summary>
    /// Projects a vector onto another vector.
    /// </summary>
    /// <param name="vector">The vector to project.</param>
    /// <param name="onNormal">The projection normal vector.</param>
    /// <returns>The projected vector.</returns>
    static Vector3 Project(const Vector3& vector, const Vector3& onNormal);

    /// <summary>
    /// Projects a vector onto a plane defined by a normal orthogonal to the plane.
    /// </summary>
    /// <param name="vector">The vector to project.</param>
    /// <param name="planeNormal">The plane normal vector.</param>
    /// <returns>The projected vector.</returns>
    static Vector3 ProjectOnPlane(const Vector3& vector, const Vector3& planeNormal)
    {
        return vector - Project(vector, planeNormal);
    }

    // Projects a 3D vector from object space into screen space
    // @param vector The vector to project
    // @param x The X position of the viewport
    // @param y The Y position of the viewport
    // @param width The width of the viewport
    // @param height The height of the viewport
    // @param minZ The minimum depth of the viewport
    // @param maxZ The maximum depth of the viewport
    // @param worldViewProjection The combined world-view-projection matrix
    // @param result When the method completes, contains the vector in screen space
    static void Project(const Vector3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3& result);

    // Projects a 3D vector from object space into screen space
    // @param vector The vector to project
    // @param x The X position of the viewport
    // @param y The Y position of the viewport
    // @param width The width of the viewport
    // @param height The height of the viewport
    // @param minZ The minimum depth of the viewport
    // @param maxZ The maximum depth of the viewport
    // @param worldViewProjection The combined world-view-projection matrix
    // @returns The vector in screen space
    static Vector3 Project(const Vector3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection)
    {
        Vector3 result;
        Project(vector, x, y, width, height, minZ, maxZ, worldViewProjection, result);
        return result;
    }

    // Projects a 3D vector from screen space into object space
    // @param vector The vector to project
    // @param x The X position of the viewport
    // @param y The Y position of the viewport
    // @param width The width of the viewport
    // @param height The height of the viewport
    // @param minZ The minimum depth of the viewport
    // @param maxZ The maximum depth of the viewport
    // @param worldViewProjection The combined world-view-projection matrix
    // @param result When the method completes, contains the vector in object space
    static void Unproject(const Vector3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3& result);

    // Projects a 3D vector from screen space into object space
    // @param vector The vector to project
    // @param x The X position of the viewport
    // @param y The Y position of the viewport
    // @param width The width of the viewport
    // @param height The height of the viewport
    // @param minZ The minimum depth of the viewport
    // @param maxZ The maximum depth of the viewport
    // @param worldViewProjection The combined world-view-projection matrix
    // @returns The vector in object space
    static Vector3 Unproject(const Vector3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection)
    {
        Vector3 result;
        Unproject(vector, x, y, width, height, minZ, maxZ, worldViewProjection, result);
        return result;
    }

    /// <summary>
    /// Creates an orthonormal basis from a basis with at least two orthogonal vectors.
    /// </summary>
    /// <param name="xAxis">The X axis.</param>
    /// <param name="yAxis">The y axis.</param>
    /// <param name="zAxis">The z axis.</param>
    static void CreateOrthonormalBasis(Vector3& xAxis, Vector3& yAxis, Vector3& zAxis);

    /// <summary>
    /// Finds the best arbitrary axis vectors to represent U and V axes of a plane, by using this vector as the normal of the plane.
    /// </summary>
    /// <param name="firstAxis">The reference to first axis.</param>
    /// <param name="secondAxis">The reference to second axis.</param>
    void FindBestAxisVectors(Vector3& firstAxis, Vector3& secondAxis) const;

    static Vector3 Round(const Vector3& v)
    {
        return Vector3(
            Math::Round(v.X),
            Math::Round(v.Y),
            Math::Round(v.Z)
        );
    }

    static Vector3 Ceil(const Vector3& v)
    {
        return Vector3(
            Math::Ceil(v.X),
            Math::Ceil(v.Y),
            Math::Ceil(v.Z)
        );
    }

    static Vector3 Abs(const Vector3& v)
    {
        return Vector3(Math::Abs(v.X), Math::Abs(v.Y), Math::Abs(v.Z));
    }

    /// <summary>
    /// Calculates the area of the triangle.
    /// </summary>
    /// <param name="v0">The first triangle vertex.</param>
    /// <param name="v1">The second triangle vertex.</param>
    /// <param name="v2">The third triangle vertex.</param>
    /// <returns>The triangle area.</returns>
    static float TriangleArea(const Vector3& v0, const Vector3& v1, const Vector3& v2);
};

inline Vector3 operator+(float a, const Vector3& b)
{
    return b + a;
}

inline Vector3 operator-(float a, const Vector3& b)
{
    return Vector3(a) - b;
}

inline Vector3 operator*(float a, const Vector3& b)
{
    return b * a;
}

inline Vector3 operator/(float a, const Vector3& b)
{
    return Vector3(a) / b;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Vector3& a, const Vector3& b)
    {
        return Vector3::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Vector3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Vector3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);
