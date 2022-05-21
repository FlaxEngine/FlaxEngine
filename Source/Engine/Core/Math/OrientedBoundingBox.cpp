// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "OrientedBoundingBox.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"
#include "Ray.h"
#include "../Types/String.h"

OrientedBoundingBox::OrientedBoundingBox(const BoundingBox& bb)
{
    const Vector3 center = bb.Minimum + (bb.Maximum - bb.Minimum) * 0.5f;
    Extents = bb.Maximum - center;
    Matrix::Translation(center, Transformation);
}

OrientedBoundingBox::OrientedBoundingBox(const Vector3& extents, const Matrix3x3& rotationScale, const Vector3& translation)
    : Extents(extents)
    , Transformation(rotationScale)
{
    Transformation.SetTranslation(translation);
}

OrientedBoundingBox::OrientedBoundingBox(Vector3 points[], int32 pointCount)
{
    ASSERT(points && pointCount > 0);

    Vector3 minimum = points[0];
    Vector3 maximum = points[0];

    for (int32 i = 1; i < pointCount; i++)
    {
        Vector3::Min(minimum, points[i], minimum);
        Vector3::Max(maximum, points[i], maximum);
    }

    const Vector3 center = minimum + (maximum - minimum) / 2.0f;
    Extents = maximum - center;
    Matrix::Translation(center, Transformation);
}

String OrientedBoundingBox::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

void OrientedBoundingBox::GetCorners(Vector3 corners[8]) const
{
    Vector3 xv = Vector3(Extents.X, 0, 0);
    Vector3 yv = Vector3(0, Extents.Y, 0);
    Vector3 zv = Vector3(0, 0, Extents.Z);

    Vector3::TransformNormal(xv, Transformation, xv);
    Vector3::TransformNormal(yv, Transformation, yv);
    Vector3::TransformNormal(zv, Transformation, zv);

    const Vector3 center = Transformation.GetTranslation();

    corners[0] = center + xv + yv + zv;
    corners[1] = center + xv + yv - zv;
    corners[2] = center - xv + yv - zv;
    corners[3] = center - xv + yv + zv;
    corners[4] = center + xv - yv + zv;
    corners[5] = center + xv - yv - zv;
    corners[6] = center - xv - yv - zv;
    corners[7] = center - xv - yv + zv;
}

Vector3 OrientedBoundingBox::GetSize() const
{
    Vector3 xv = Vector3(Extents.X * 2, 0, 0);
    Vector3 yv = Vector3(0, Extents.Y * 2, 0);
    Vector3 zv = Vector3(0, 0, Extents.Z * 2);

    Vector3::TransformNormal(xv, Transformation, xv);
    Vector3::TransformNormal(yv, Transformation, yv);
    Vector3::TransformNormal(zv, Transformation, zv);

    return Vector3(xv.Length(), yv.Length(), zv.Length());
}

Vector3 OrientedBoundingBox::GetSizeSquared() const
{
    Vector3 xv = Vector3(Extents.X * 2, 0, 0);
    Vector3 yv = Vector3(0, Extents.Y * 2, 0);
    Vector3 zv = Vector3(0, 0, Extents.Z * 2);

    Vector3::TransformNormal(xv, Transformation, xv);
    Vector3::TransformNormal(yv, Transformation, yv);
    Vector3::TransformNormal(zv, Transformation, zv);

    return Vector3(xv.LengthSquared(), yv.LengthSquared(), zv.LengthSquared());
}

BoundingBox OrientedBoundingBox::GetBoundingBox() const
{
    BoundingBox result;

    Vector3 corners[8];
    GetCorners(corners);

    BoundingBox::FromPoints(corners, 8, result);

    return result;
}

void OrientedBoundingBox::GetBoundingBox(BoundingBox& result) const
{
    Vector3 corners[8];
    GetCorners(corners);

    BoundingBox::FromPoints(corners, 8, result);
}

void OrientedBoundingBox::Transform(const Matrix& matrix)
{
    const Matrix tmp = Transformation;
    Matrix::Multiply(tmp, matrix, Transformation);
}

ContainmentType OrientedBoundingBox::Contains(const Vector3& point, float* distance) const
{
    // Transform the point into the obb coordinates
    Matrix invTrans;
    Matrix::Invert(Transformation, invTrans);

    Vector3 locPoint;
    Vector3::TransformCoordinate(point, invTrans, locPoint);

    locPoint.X = Math::Abs(locPoint.X);
    locPoint.Y = Math::Abs(locPoint.Y);
    locPoint.Z = Math::Abs(locPoint.Z);

    if (distance)
    {
        // Get minimum distance to edge in local space
        Vector3 tmp;
        Vector3::Subtract(Extents, locPoint, tmp);
        tmp.Absolute();
        const float minDstToEdgeLocal = tmp.MinValue();

        // Transform distance to world space
        Vector3 dstVec = Vector3::UnitX * minDstToEdgeLocal;
        Vector3::TransformNormal(dstVec, Transformation, dstVec);
        *distance = dstVec.Length();
    }

    // Simple axes-aligned BB check
    if (locPoint.X < Extents.X && locPoint.Y < Extents.Y && locPoint.Z < Extents.Z)
        return ContainmentType::Contains;
    if (Vector3::NearEqual(locPoint, Extents))
        return ContainmentType::Intersects;
    return ContainmentType::Disjoint;
}

ContainmentType OrientedBoundingBox::Contains(int32 pointsCnt, Vector3 points[]) const
{
    ASSERT(pointsCnt > 0);

    Matrix invTrans;
    Matrix::Invert(Transformation, invTrans);

    bool containsAll = true;
    bool containsAny = false;

    for (int32 i = 0; i < pointsCnt; i++)
    {
        Vector3 locPoint;
        Vector3::TransformCoordinate(points[i], invTrans, locPoint);

        locPoint.X = Math::Abs(locPoint.X);
        locPoint.Y = Math::Abs(locPoint.Y);
        locPoint.Z = Math::Abs(locPoint.Z);

        // Simple axes-aligned BB check
        if (Vector3::NearEqual(locPoint, Extents))
            containsAny = true;
        if (locPoint.X < Extents.X && locPoint.Y < Extents.Y && locPoint.Z < Extents.Z)
            containsAny = true;
        else
            containsAll = false;
    }

    if (containsAll)
        return ContainmentType::Contains;
    if (containsAny)
        return ContainmentType::Intersects;
    return ContainmentType::Disjoint;
}

ContainmentType OrientedBoundingBox::Contains(const BoundingSphere& sphere, bool ignoreScale) const
{
    Matrix invTrans;
    Matrix::Invert(Transformation, invTrans);

    // Transform sphere center into the obb coordinates
    Vector3 locCenter;
    Vector3::TransformCoordinate(sphere.Center, invTrans, locCenter);

    float locRadius;
    if (ignoreScale)
    {
        locRadius = sphere.Radius;
    }
    else
    {
        // Transform sphere radius into the obb coordinates
        Vector3 vRadius = Vector3::UnitX * sphere.Radius;
        Vector3::TransformNormal(vRadius, invTrans, vRadius);
        locRadius = vRadius.Length();
    }

    // Perform regular BoundingBox to BoundingSphere containment check
    const Vector3 minusExtents = -Extents;
    Vector3 vector;
    Vector3::Clamp(locCenter, minusExtents, Extents, vector);
    const float distance = Vector3::DistanceSquared(locCenter, vector);

    if (distance > locRadius * locRadius)
        return ContainmentType::Disjoint;

    if (minusExtents.X + locRadius <= locCenter.X
        && locCenter.X <= Extents.X - locRadius
        && (Extents.X - minusExtents.X > locRadius
            && minusExtents.Y + locRadius <= locCenter.Y)
        && (locCenter.Y <= Extents.Y - locRadius
            && Extents.Y - minusExtents.Y > locRadius
            && (minusExtents.Z + locRadius <= locCenter.Z
                && locCenter.Z <= Extents.Z - locRadius
                && Extents.Z - minusExtents.Z > locRadius)))
    {
        return ContainmentType::Contains;
    }

    return ContainmentType::Intersects;
}

/*ContainmentType OrientedBoundingBox::Contains(const OrientedBoundingBox& obb)
{
	Vector3 obbCorners[8];
	obb.GetCorners(obbCorners);
	auto cornersCheck = Contains(8, obbCorners);
	if (cornersCheck != ContainmentType::Disjoint)
		return cornersCheck;

	// http://www.3dkingdoms.com/weekly/bbox.cpp
	Vector3 SizeA = Extents;
	Vector3 SizeB = obb.Extents;
	Vector3 RotA[3];
	Vector3 RotB[3];
	GetRows(Transformation, RotA);
	GetRows(obb.Transformation, RotB);

	Matrix R = Matrix::Identity;       // Rotation from B to A
	Matrix AR = Matrix::Identity;      // absolute values of R matrix, to use with box extents

	float ExtentA, ExtentB, Separation;
	int i, k;

	// Calculate B to A rotation matrix
	for (i = 0; i < 3; i++)
	{
		for (k = 0; k < 3; k++)
		{
			R[i, k] = Vector3::Dot(RotA[i], RotB[k]);
			AR[i, k] = Math::Abs(R[i, k]);
		}
	}

	// Vector separating the centers of Box B and of Box A	
	Vector3 vSepWS = obb.GetCenter() - GetCenter();

	// Rotated into Box A's coordinates
	Vector3 vSepA = Vector3(Vector3::Dot(vSepWS, RotA[0]), Vector3::Dot(vSepWS, RotA[1]), Vector3::Dot(vSepWS, RotA[2]));

	// Test if any of A's basis vectors separate the box
	for (i = 0; i < 3; i++)
	{
		ExtentA = SizeA[i];
		ExtentB = Vector3::Dot(SizeB, Vector3(AR[i, 0], AR[i, 1], AR[i, 2]));
		Separation = Math::Abs(vSepA[i]);

		if (Separation > ExtentA + ExtentB)
			return ContainmentType::Disjoint;
	}

	// Test if any of B's basis vectors separate the box
	for (k = 0; k < 3; k++)
	{
		ExtentA = Vector3::Dot(SizeA, Vector3(AR[0, k], AR[1, k], AR[2, k]));
		ExtentB = SizeB[k];
		Separation = Math::Abs(Vector3::Dot(vSepA, Vector3(R[0, k], R[1, k], R[2, k])));

		if (Separation > ExtentA + ExtentB)
			return ContainmentType::Disjoint;
	}

	// Now test Cross Products of each basis vector combination ( A[i], B[k] )
	for (i = 0; i < 3; i++)
	{
		for (k = 0; k < 3; k++)
		{
			int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
			int k1 = (k + 1) % 3, k2 = (k + 2) % 3;
			ExtentA = SizeA[i1] * AR[i2, k] + SizeA[i2] * AR[i1, k];
			ExtentB = SizeB[k1] * AR[i, k2] + SizeB[k2] * AR[i, k1];
			Separation = Math::Abs(vSepA[i2] * R[i1, k] - vSepA[i1] * R[i2, k]);
			if (Separation > ExtentA + ExtentB)
				return ContainmentType::Disjoint;
		}
	}

	// No separating axis found, the boxes overlap	
	return ContainmentType::Intersects;
}*/

bool OrientedBoundingBox::Intersects(const Ray& ray, Vector3& point) const
{
    // Put ray in box space
    Matrix invTrans;
    Matrix::Invert(Transformation, invTrans);

    Ray bRay;
    Vector3::TransformNormal(ray.Direction, invTrans, bRay.Direction);
    Vector3::TransformCoordinate(ray.Position, invTrans, bRay.Position);

    // Perform a regular ray to BoundingBox check
    const BoundingBox bb = BoundingBox(-Extents, Extents);
    const bool intersects = CollisionsHelper::RayIntersectsBox(bRay, bb, point);

    // Put the result intersection back to world
    if (intersects)
        Vector3::TransformCoordinate(point, Transformation, point);

    return intersects;
}

bool OrientedBoundingBox::Intersects(const Ray& ray, float& distance) const
{
    Vector3 point;
    const bool result = Intersects(ray, point);
    distance = Vector3::Distance(ray.Position, point);
    return result;
}

bool OrientedBoundingBox::Intersects(const Ray& ray, float& distance, Vector3& normal) const
{
    // Put ray in box space
    Matrix invTrans;
    Matrix::Invert(Transformation, invTrans);

    Ray bRay;
    Vector3::TransformNormal(ray.Direction, invTrans, bRay.Direction);
    Vector3::TransformCoordinate(ray.Position, invTrans, bRay.Position);

    // Perform a regular ray to BoundingBox check
    const BoundingBox bb(-Extents, Extents);
    if (CollisionsHelper::RayIntersectsBox(bRay, bb, distance, normal))
    {
        // Put the result intersection back to world
        Vector3::TransformNormal(normal, Transformation, normal);
        normal.Normalize();
        return true;
    }

    return false;
}
