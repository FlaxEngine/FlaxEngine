// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Triangle.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Helper container used for detailed triangle mesh intersections tests.
/// </summary>
class FLAXENGINE_API CollisionProxy
{
public:

    /// <summary>
    /// The triangles.
    /// </summary>
    Array<Triangle> Triangles;

public:

    FORCE_INLINE bool HasData() const
    {
        return Triangles.HasItems();
    }

    template<typename IndexType>
    void Init(uint32 vertices, uint32 triangles, Vector3* positions, IndexType* indices)
    {
        Triangles.Clear();
        Triangles.EnsureCapacity(triangles, false);

        IndexType* it = indices;
        for (uint32 i = 0; i < triangles; i++)
        {
            auto i0 = *(it++);
            auto i1 = *(it++);
            auto i2 = *(it++);

            if (i0 < vertices && i1 < vertices && i2 < vertices)
            {
                Triangles.Add({ positions[i0], positions[i1], positions[i2] });
            }
        }
    }

    void Clear()
    {
        Triangles.Clear();
    }

    bool Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal) const
    {
        // TODO: use SIMD
        for (int32 i = 0; i < Triangles.Count(); i++)
        {
            Triangle triangle = Triangles[i];

            Vector3::Transform(triangle.V0, world, triangle.V0);
            Vector3::Transform(triangle.V1, world, triangle.V1);
            Vector3::Transform(triangle.V2, world, triangle.V2);

            if (triangle.Intersects(ray, distance, normal))
            {
                return true;
            }
        }

        return false;
    }
};
