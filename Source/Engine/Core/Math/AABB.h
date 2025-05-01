// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Vector3.h"

/// <summary>
/// Integer axis aligned bounding box
/// </summary>
struct FLAXENGINE_API AABB
{
public:
    int32 MinX;
    int32 MaxX;

    int32 MinY;
    int32 MaxY;

    int32 MinZ;
    int32 MaxZ;

public:
    AABB()
    {
        MinX = MAX_int32;
        MaxX = MIN_int32;
        MinY = MAX_int32;
        MaxY = MIN_int32;
        MinZ = MAX_int32;
        MaxZ = MIN_int32;
    }

    AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    {
        MinX = (int32)minX;
        MaxX = (int32)maxX;
        MinY = (int32)minY;
        MaxY = (int32)maxY;
        MinZ = (int32)minZ;
        MaxZ = (int32)maxZ;
    }

    AABB(const AABB& other)
    {
        Set(other);
    }

public:
    int32 Width() const
    {
        return MaxX - MinX;
    }

    int32 Height() const
    {
        return MaxY - MinY;
    }

    int32 Depth() const
    {
        return MaxZ - MinZ;
    }

    int32 X() const
    {
        return (MaxX + MinX) / 2;
    }

    int32 Y() const
    {
        return (MaxY + MinY) / 2;
    }

    int32 Z() const
    {
        return (MaxZ + MinZ) / 2;
    }

    bool IsEmpty() const
    {
        return MinX >= MaxX || MinY >= MaxY || MinZ >= MaxZ;
    }

public:
    void Clear()
    {
        MinX = MAX_int32;
        MaxX = MIN_int32;
        MinY = MAX_int32;
        MaxY = MIN_int32;
        MinZ = MAX_int32;
        MaxZ = MIN_int32;
    }

    void Add(const Vector3& inCoordinate)
    {
        Add(inCoordinate.X, inCoordinate.Y, inCoordinate.Z);
    }

    void Add(Real inX, Real inY, Real inZ)
    {
        ASSERT(!isinf(inX) && !isnan(inX));
        ASSERT(!isinf(inY) && !isnan(inY));
        ASSERT(!isinf(inZ) && !isnan(inZ));

        MinX = Math::Min(MinX, (int32)Math::Floor(inX));
        MinY = Math::Min(MinY, (int32)Math::Floor(inY));
        MinZ = Math::Min(MinZ, (int32)Math::Floor(inZ));

        MaxX = Math::Max(MaxX, (int32)Math::Ceil(inX));
        MaxY = Math::Max(MaxY, (int32)Math::Ceil(inY));
        MaxZ = Math::Max(MaxZ, (int32)Math::Ceil(inZ));
    }

    void Add(const AABB& bounds)
    {
        MinX = Math::Min(MinX, bounds.MinX);
        MinY = Math::Min(MinY, bounds.MinY);
        MinZ = Math::Min(MinZ, bounds.MinZ);

        MaxX = Math::Max(MaxX, bounds.MaxX);
        MaxY = Math::Max(MaxY, bounds.MaxY);
        MaxZ = Math::Max(MaxZ, bounds.MaxZ);
    }

    void Set(const AABB& bounds)
    {
        MinX = bounds.MinX;
        MinY = bounds.MinY;
        MinZ = bounds.MinZ;

        MaxX = bounds.MaxX;
        MaxY = bounds.MaxY;
        MaxZ = bounds.MaxZ;
    }

    void Translate(int32 X, int32 Y, int32 Z)
    {
        MinX += X;
        MinY += Y;
        MinZ += Z;

        MaxX += X;
        MaxY += Y;
        MaxZ += Z;
    }

    void Translate(const Vector3& translation)
    {
        MinX = (int32)Math::Floor(MinX + translation.X);
        MinY = (int32)Math::Floor(MinY + translation.Y);
        MinZ = (int32)Math::Floor(MinZ + translation.Z);

        MaxX = (int32)Math::Ceil(MaxX + translation.X);
        MaxY = (int32)Math::Ceil(MaxY + translation.Y);
        MaxZ = (int32)Math::Ceil(MaxZ + translation.Z);
    }

    AABB Translated(const Vector3& translation) const
    {
        AABB result;
        result.Translate(translation);
        return result;
    }

    void Set(const AABB& other, const Vector3& translation)
    {
        MinX = (int32)Math::Floor(other.MinX + translation.X);
        MinY = (int32)Math::Floor(other.MinY + translation.Y);
        MinZ = (int32)Math::Floor(other.MinZ + translation.Z);

        MaxX = (int32)Math::Ceil(other.MaxX + translation.X);
        MaxY = (int32)Math::Ceil(other.MaxY + translation.Y);
        MaxZ = (int32)Math::Ceil(other.MaxZ + translation.Z);
    }

public:
    bool IsOutside(const AABB& other) const
    {
        return MaxX - other.MinX < 0 || MinX - other.MaxX > 0 ||
                MaxY - other.MinY < 0 || MinY - other.MaxY > 0 ||
                MaxZ - other.MinZ < 0 || MinZ - other.MaxZ > 0;
    }

    static bool IsOutside(const AABB& left, const AABB& right)
    {
        return left.MaxX - right.MinX < 0 || left.MinX - right.MaxX > 0 ||
                left.MaxY - right.MinY < 0 || left.MinY - right.MaxY > 0 ||
                left.MaxZ - right.MinZ < 0 || left.MinZ - right.MaxZ > 0;
    }
};
