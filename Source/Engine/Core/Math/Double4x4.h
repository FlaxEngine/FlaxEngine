// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Double4x4() = default;

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
