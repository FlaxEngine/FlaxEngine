// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Vector4.h"

/// <summary>
/// Represents a 4x4 mathematical matrix using double-precision floating-point values.
/// </summary>
struct FLAXENGINE_API Double4x4
{
public:
    union
    {
        struct
        {
            /// <summary>Value at row 1 column 1 of the matrix.</summary>
            double M11;

            /// <summary>Value at row 1 column 2 of the matrix.</summary>
            double M12;

            /// <summary>Value at row 1 column 3 of the matrix.</summary>
            double M13;

            /// <summary>Value at row 1 column 4 of the matrix.</summary>
            double M14;

            /// <summary>Value at row 2 column 1 of the matrix.</summary>
            double M21;

            /// <summary>Value at row 2 column 2 of the matrix.</summary>
            double M22;

            /// <summary>Value at row 2 column 3 of the matrix.</summary>
            double M23;

            /// <summary>Value at row 2 column 4 of the matrix.</summary>
            double M24;

            /// <summary>Value at row 3 column 1 of the matrix.</summary>
            double M31;

            /// <summary>Value at row 3 column 2 of the matrix.</summary>
            double M32;

            /// <summary>Value at row 3 column 3 of the matrix.</summary>
            double M33;

            /// <summary>Value at row 3 column 4 of the matrix.</summary>
            double M34;

            /// <summary>Value at row 4 column 1 of the matrix.</summary>
            double M41;

            /// <summary>Value at row 4 column 2 of the matrix.</summary>
            double M42;

            /// <summary>Value at row 4 column 3 of the matrix.</summary>
            double M43;

            /// <summary>Value at row 4 column 4 of the matrix.</summary>
            double M44;
        };

        double Values[4][4];
        double Raw[16];
    };
public:
    /// <summary>A matrix with all of its components set to zero.</summary>
    static const Double4x4 Zero;

    /// <summary>The identity matrix.</summary>
    static const Double4x4 Identity;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Double4x4() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix"/> struct.
    /// </summary>
    Double4x4(double m11, double m12, double m13, double m14,
        double m21, double m22, double m23, double m24,
        double m31, double m32, double m33, double m34,
        double m41, double m42, double m43, double m44)
        : M11(m11)
        , M12(m12)
        , M13(m13)
        , M14(m14)
        , M21(m21)
        , M22(m22)
        , M23(m23)
        , M24(m24)
        , M31(m31)
        , M32(m32)
        , M33(m33)
        , M34(m34)
        , M41(m41)
        , M42(m42)
        , M43(m43)
        , M44(m44)
    {
    }

    Double4x4(const Matrix& matrix);

public:
    // Inverts the matrix.
    void Invert()
    {
        Invert(*this, *this);
    }

    // Calculates the inverse of the specified matrix.
    static Double4x4 Invert(const Double4x4& value)
    {
        Double4x4 result;
        Invert(value, result);
        return result;
    }

    // Calculates the inverse of the specified matrix.
    static void Invert(const Double4x4& value, Double4x4& result);

    // Creates a left-handed, look-at matrix.
    static void LookAt(const Double3& eye, const Double3& target, const Double3& up, Double4x4& result);

    // Calculates the product of two matrices.
    static void Multiply(const Double4x4& left, const Double4x4& right, Double4x4& result);

    // Creates a matrix that contains both the X, Y and Z rotation, as well as scaling and translation.
    static void Transformation(const Float3& scaling, const Quaternion& rotation, const Vector3& translation, Double4x4& result);

public:
    Double4x4 operator*(const Double4x4& other) const
    {
        Double4x4 result;
        Multiply(*this, other, result);
        return result;
    }
};

template<>
struct TIsPODType<Double4x4>
{
    enum { Value = true };
};
