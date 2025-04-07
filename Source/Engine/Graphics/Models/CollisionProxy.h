// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/CollisionsHelper.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Helper container used for detailed triangle mesh intersections tests.
/// </summary>
class FLAXENGINE_API CollisionProxy
{
public:
    struct CollisionTriangle
    {
        Float3 V0, V1, V2;
    };

    /// <summary>
    /// The triangles.
    /// </summary>
    Array<CollisionTriangle> Triangles;

public:
    FORCE_INLINE bool HasData() const
    {
        return Triangles.HasItems();
    }

    template<typename IndexType>
    void Init(uint32 vertices, uint32 triangles, const Float3* positions, const IndexType* indices, uint32 positionsStride = sizeof(Float3))
    {
        Triangles.Clear();
        Triangles.EnsureCapacity(triangles, false);
        const IndexType* it = indices;
        for (uint32 i = 0; i < triangles; i++)
        {
            const IndexType i0 = *(it++);
            const IndexType i1 = *(it++);
            const IndexType i2 = *(it++);
            if (i0 < vertices && i1 < vertices && i2 < vertices)
            {
#define GET_POS(idx) *(const Float3*)((const byte*)positions + positionsStride * idx)
                Triangles.Add({ GET_POS(i0), GET_POS(i1), GET_POS(i2) });
#undef GET_POS
            }
        }
    }

    void Clear()
    {
        Triangles.Clear();
    }

    bool Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal) const
    {
        distance = MAX_Real;
        for (int32 i = 0; i < Triangles.Count(); i++)
        {
            CollisionTriangle triangle = Triangles[i];
            Float3::Transform(triangle.V0, world, triangle.V0);
            Float3::Transform(triangle.V1, world, triangle.V1);
            Float3::Transform(triangle.V2, world, triangle.V2);

            // TODO: use 32-bit precision for intersection
            Real d;
            if (CollisionsHelper::RayIntersectsTriangle(ray, triangle.V0, triangle.V1, triangle.V2, d) && d < distance)
            {
                normal = Vector3::Normalize((triangle.V1 - triangle.V0) ^ (triangle.V2 - triangle.V0));
                distance = d;
            }
        }
        return distance < MAX_Real;
    }

    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const
    {
        distance = MAX_Real;
        for (int32 i = 0; i < Triangles.Count(); i++)
        {
            CollisionTriangle triangle = Triangles[i];
            Vector3 v0, v1, v2;
            transform.LocalToWorld(triangle.V0, v0);
            transform.LocalToWorld(triangle.V1, v1);
            transform.LocalToWorld(triangle.V2, v2);

            // TODO: use 32-bit precision for intersection
            Real d;
            if (CollisionsHelper::RayIntersectsTriangle(ray, v0, v1, v2, d) && d < distance)
            {
                normal = Vector3::Normalize((v1 - v0) ^ (v2 - v0));
                distance = d;
            }
        }
        return distance < MAX_Real;
    }
};
