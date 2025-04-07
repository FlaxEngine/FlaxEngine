// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Plane.h"
#include "Vector3.h"
#include "Matrix.h"
#include "CollisionsHelper.h"

/// <summary>
/// Defines a frustum which can be used in frustum culling, zoom to Extents (zoom to fit) operations, (matrix, frustum, camera) interchange, and many kind of intersection testing.
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API BoundingFrustum
{
    friend CollisionsHelper;
private:
    Matrix _matrix;

    union
    {
        struct
        {
            Plane _pNear;
            Plane _pFar;
            Plane _pLeft;
            Plane _pRight;
            Plane _pTop;
            Plane _pBottom;
        };
        Plane _planes[6];
    };

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    BoundingFrustum() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="BoundingFrustum"/> struct.
    /// </summary>
    /// <param name="matrix">The combined matrix that usually takes View * Projection matrix.</param>
    BoundingFrustum(const Matrix& matrix)
    {
        SetMatrix(matrix);
    }

public:
    String ToString() const;

public:
    /// <summary>
    /// Gets the matrix that describes this bounding frustum.
    /// </summary>
    FORCE_INLINE const Matrix& GetMatrix() const
    {
        return _matrix;
    }

    /// <summary>
    /// Gets the matrix that describes this bounding frustum.
    /// </summary>
    FORCE_INLINE Matrix GetMatrix()
    {
        return _matrix;
    }

    /// <summary>
    /// Gets the inverted matrix to that describes this bounding frustum.
    /// </summary>
    /// <param name="result">The result matrix.</param>
    FORCE_INLINE void GetInvMatrix(Matrix& result) const
    {
        Matrix::Invert(_matrix, result);
    }

    /// <summary>
    /// Sets the matrix (made from view and projection matrices) that describes this bounding frustum.
    /// </summary>
    /// <param name="view">The view matrix.</param>
    /// <param name="projection">The projection matrix.</param>
    void SetMatrix(const Matrix& view, const Matrix& projection)
    {
        Matrix viewProjection;
        Matrix::Multiply(view, projection, viewProjection);
        SetMatrix(viewProjection);
    }

    /// <summary>
    /// Sets the matrix that describes this bounding frustum.
    /// </summary>
    /// <param name="matrix">The matrix (View*Projection).</param>
    void SetMatrix(const Matrix& matrix);

    /// <summary>
    /// Gets the near.
    /// </summary>
    FORCE_INLINE Plane GetNear() const
    {
        return _pNear;
    }

    /// <summary>
    /// Gets the far plane of the frustum.
    /// </summary>
    FORCE_INLINE Plane GetFar() const
    {
        return _pFar;
    }

    /// <summary>
    /// Gets the left plane of the frustum.
    /// </summary>
    FORCE_INLINE Plane GetLeft() const
    {
        return _pLeft;
    }

    /// <summary>
    /// Gets the right plane of the frustum.
    /// </summary>
    FORCE_INLINE Plane GetRight() const
    {
        return _pRight;
    }

    /// <summary>
    /// Gets the top plane of the frustum.
    /// </summary>
    FORCE_INLINE Plane GetTop() const
    {
        return _pTop;
    }

    /// <summary>
    /// Gets the bottom plane of the frustum.
    /// </summary>
    FORCE_INLINE Plane GetBottom() const
    {
        return _pBottom;
    }

    /// <summary>
    /// Gets the one of the 6 planes related to this frustum.
    /// </summary>
    /// <param name="index">The index where 0 for Left, 1 for Right, 2 for Top, 3 for Bottom, 4 for Near, 5 for Far.</param>
    /// <returns>The plane.</returns>
    Plane GetPlane(int32 index) const;

    /// <summary>
    /// Gets the the 8 corners of the frustum: Near1 (near right down corner), Near2 (near right top corner), Near3 (near Left top corner), Near4 (near Left down corner), Far1 (far right down corner), Far2 (far right top corner), Far3 (far left top corner), Far4 (far left down corner).
    /// </summary>
    /// <param name="corners">The corners.</param>
    void GetCorners(Float3 corners[8]) const;

    /// <summary>
    /// Gets the the 8 corners of the frustum: Near1 (near right down corner), Near2 (near right top corner), Near3 (near Left top corner), Near4 (near Left down corner), Far1 (far right down corner), Far2 (far right top corner), Far3 (far left top corner), Far4 (far left down corner).
    /// </summary>
    /// <param name="corners">The corners.</param>
    void GetCorners(Double3 corners[8]) const;

    /// <summary>
    /// Gets bounding box that contains whole frustum.
    /// </summary>
    /// <param name="result">The result box.</param>
    void GetBox(BoundingBox& result) const;

    /// <summary>
    /// Gets bounding sphere that contains whole frustum.
    /// </summary>
    /// <param name="result">The result sphere.</param>
    void GetSphere(BoundingSphere& result) const;

    /// <summary>
    /// Determines whether this frustum is orthographic.
    /// </summary>
    FORCE_INLINE bool IsOrthographic() const
    {
        return _pLeft.Normal == -_pRight.Normal && _pTop.Normal == -_pBottom.Normal;
    }

    /// <summary>
    /// Gets the width of the frustum at specified depth.
    /// </summary>
    /// <param name="depth">The depth at which to calculate frustum width.</param>
    /// <returns>The width of the frustum at the specified depth.</returns>
    float GetWidthAtDepth(float depth) const;

    /// <summary>
    /// Gets the height of the frustum at specified depth.
    /// </summary>
    /// <param name="depth">The depth at which to calculate frustum height.</param>
    /// <returns>The height of the frustum at the specified depth.</returns>
    float GetHeightAtDepth(float depth) const;

public:
    FORCE_INLINE bool operator==(const BoundingFrustum& other) const
    {
        return _matrix == other._matrix;
    }

    FORCE_INLINE bool operator!=(const BoundingFrustum& other) const
    {
        return _matrix != other._matrix;
    }

public:
    /// <summary>
    /// Checks whether a point lays inside, intersects or lays outside the frustum.
    /// </summary>
    /// <param name="point">The point.</param>
    /// <returns>The type of the containment.</returns>
    ContainmentType Contains(const Vector3& point) const;

    /// <summary>
    /// Determines the intersection relationship between the frustum and a bounding box.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <returns>The type of the containment.</returns>
    FORCE_INLINE ContainmentType Contains(const BoundingBox& box) const
    {
        return CollisionsHelper::FrustumContainsBox(*this, box);
    }

    /// <summary>
    /// Determines the intersection relationship between the frustum and a bounding sphere.
    /// </summary>
    /// <param name="sphere">The sphere.</param>
    /// <returns>The type of the containment.</returns>
    ContainmentType Contains(const BoundingSphere& sphere) const;

    /// <summary>
    /// Checks whether the current frustum intersects a sphere.
    /// </summary>
    /// <param name="sphere">The sphere.</param>
    /// <returns>True if the current frustum intersects a sphere, otherwise false.</returns>
    bool Intersects(const BoundingSphere& sphere) const;

    /// <summary>
    /// Checks whether the current frustum intersects a box.
    /// </summary>
    /// <param name="box">The box</param>
    /// <returns>True if the current frustum intersects a box, otherwise false.</returns>
    FORCE_INLINE bool Intersects(const BoundingBox& box) const
    {
        return CollisionsHelper::FrustumContainsBox(*this, box) != ContainmentType::Disjoint;
    }
};

template<>
struct TIsPODType<BoundingFrustum>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(BoundingFrustum, "{}", v.GetMatrix());
