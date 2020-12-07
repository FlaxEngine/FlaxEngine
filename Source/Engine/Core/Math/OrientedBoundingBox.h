// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Vector3.h"
#include "Matrix.h"
#include "CollisionsHelper.h"

// Oriented Bounding Box (OBB) is a rectangular block, much like an AABB (Bounding Box) but with an arbitrary orientation in 3D space.
API_STRUCT(InBuild) struct OrientedBoundingBox
{
public:

    // Half lengths of the box along each axis.
    Vector3 Extents;

    // The matrix which aligns and scales the box, and its translation vector represents the center of the box.
    Matrix Transformation;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    OrientedBoundingBox()
    {
    }

    // Init
    // @param bb The BoundingBox to create from
    OrientedBoundingBox(const BoundingBox& bb);

    // Init
    // @param extents The half lengths of the box along each axis.
    // @param transformation The matrix which aligns and scales the box, and its translation vector represents the center of the box.
    OrientedBoundingBox(const Vector3& extents, const Matrix& transformation)
    {
        Extents = extents;
        Transformation = transformation;
    }

    // Init
    // @param minimum The minimum vertex of the bounding box.
    // @param maximum The maximum vertex of the bounding box.
    OrientedBoundingBox(const Vector3& minimum, const Vector3& maximum)
    {
        const Vector3 center = minimum + (maximum - minimum) / 2.0f;
        Extents = maximum - center;
        Matrix::Translation(center, Transformation);
    }

    // Init
    // @param points The points that will be contained by the box.
    // @param pointCount Amount of the points in th array.
    OrientedBoundingBox(Vector3 points[], int32 pointCount);

public:

    String ToString() const;

public:

    // Gets the eight corners of the bounding box.
    void GetCorners(Vector3 corners[8]) const;

    // The size of the OBB if no scaling is applied to the transformation matrix.
    Vector3 GetSizeUnscaled() const
    {
        return Extents * 2.0f;
    }

    // Returns the size of the OBB taking into consideration the scaling applied to the transformation matrix.
    Vector3 GetSize() const;

    // Returns the square size of the OBB taking into consideration the scaling applied to the transformation matrix.
    // @returns The size of the consideration.
    Vector3 GetSizeSquared() const;

    // Gets the center of the OBB.
    Vector3 GetCenter() const
    {
        return Transformation.GetTranslation();
    }

    /// <summary>
    /// Gets the AABB which contains all OBB corners.
    /// </summary>
    /// <returns>The result</returns>
    BoundingBox GetBoundingBox() const;

    /// <summary>
    /// Gets the AABB which contains all OBB corners.
    /// </summary>
    /// <param name="result">The result.</param>
    void GetBoundingBox(BoundingBox& result) const;

public:

    // Transforms this box using a transformation matrix.
    // @param mat The transformation matrix.
    void Transform(const Matrix& mat)
    {
        Transformation *= mat;
    }

    // Scales the OBB by scaling its Extents without affecting the Transformation matrix.
    // By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
    // @param scaling Scale to apply to the box.
    void Scale(const Vector3& scaling)
    {
        Extents *= scaling;
    }

    // Scales the OBB by scaling its Extents without affecting the Transformation matrix.
    // By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
    // @param scaling Scale to apply to the box.
    void Scale(float scaling)
    {
        Extents *= scaling;
    }

    // Translates the OBB to a new position using a translation vector.
    // @param translation the translation vector.
    void Translate(const Vector3& translation)
    {
        Transformation.SetTranslation(Transformation.GetTranslation() + translation);
    }

public:

    FORCE_INLINE bool operator==(const OrientedBoundingBox& other) const
    {
        return Extents == other.Extents && Transformation == other.Transformation;
    }

    FORCE_INLINE bool operator!=(const OrientedBoundingBox& other) const
    {
        return Extents != other.Extents || Transformation != other.Transformation;
    }

    FORCE_INLINE OrientedBoundingBox operator*(const Matrix& matrix) const
    {
        OrientedBoundingBox result = *this;
        result.Transform(matrix);
        return result;
    }

private:

    static void GetRows(const Matrix& mat, Vector3 rows[3])
    {
        rows[0] = Vector3(mat.M11, mat.M12, mat.M13);
        rows[1] = Vector3(mat.M21, mat.M22, mat.M23);
        rows[2] = Vector3(mat.M31, mat.M32, mat.M33);
    }

public:

    /// <summary>
    /// Creates the centered box (axis aligned).
    /// </summary>
    /// <param name="center">The center.</param>
    /// <param name="size">The size.</param>
    /// <param name="result">The result.</param>
    static void CreateCentered(const Vector3& center, const Vector3& size, OrientedBoundingBox& result)
    {
        result.Extents = size / 2.0f;
        Matrix::Translation(center, result.Transformation);
    }

    /// <summary>
    /// Creates the centered box (axis-aligned).
    /// </summary>
    /// <param name="center">The center.</param>
    /// <param name="size">The size.</param>
    /// <returns>The result.</returns>
    static OrientedBoundingBox CreateCentered(const Vector3& center, const Vector3& size)
    {
        OrientedBoundingBox result;
        result.Extents = size / 2.0f;
        Matrix::Translation(center, result.Transformation);
        return result;
    }

public:

    // Determines whether a OBB contains a point.
    // @param point The point to test.
    // @returns The type of containment the two objects have.
    ContainmentType Contains(const Vector3& point, float* distance = nullptr) const;

    // Determines whether a OBB contains an array of points.
    // @param pointsCnt Amount of points to test.
    // @param points The points array to test.
    // @returns The type of containment.
    ContainmentType Contains(int32 pointsCnt, Vector3 points[]) const;

    // Determines whether a OBB contains a BoundingSphere.
    // @param sphere The sphere to test.
    // @param ignoreScale Optimize the check operation by assuming that OBB has no scaling applied.
    // @returns The type of containment the two objects have.
    ContainmentType Contains(const BoundingSphere& sphere, bool ignoreScale = false) const;

    // Determines whether there is an intersection between a Ray and a OBB.
    // @param ray The ray to test.
    // @param point When the method completes, contains the point of intersection, or Vector3.Zero if there was no intersection.
    // @returns Whether the two objects intersected.
    bool Intersects(const Ray& ray, Vector3& point) const;

    // Determines if there is an intersection between the current object and a Ray.
    // @param ray The ray to test.
    // @param distance When the method completes, contains the distance of the intersection, or 0 if there was no intersection.
    // @returns Whether the two objects intersected.
    bool Intersects(const Ray& ray, float& distance) const;

    // Determines if there is an intersection between the current object and a Ray.
    // @param ray The ray to test.
    // @param distance When the method completes, contains the distance of the intersection, or 0 if there was no intersection.
    // @param normal When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection.
    // @returns Whether the two objects intersected.
    bool Intersects(const Ray& ray, float& distance, Vector3& normal) const;

    // Determines whether there is an intersection between a Ray and a OBB.
    // @param ray The ray to test.
    // @returns Whether the two objects intersected.
    bool Intersects(const Ray& ray) const
    {
        Vector3 point;
        return Intersects(ray, point);
    }
};

template<>
struct TIsPODType<OrientedBoundingBox>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(OrientedBoundingBox, "Center: {0}, Size: {1}", v.GetCenter(), v.GetSize());
