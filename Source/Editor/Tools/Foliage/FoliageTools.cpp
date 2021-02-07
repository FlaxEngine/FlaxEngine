// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "FoliageTools.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Terrain/Terrain.h"
#include "Engine/Terrain/TerrainPatch.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Foliage/Foliage.h"
#include "Engine/UI/TextRender.h"
#include "Engine/Core/Random.h"

struct GeometryTriangle
{
    Vector3 Vertex;
    Vector3 Vector1;
    Vector3 Vector2;
    Vector3 Normal;
    float Area;

    GeometryTriangle(const bool isDeterminantPositive, const Vector3& v0, const Vector3& v1, const Vector3& v2)
    {
        Vertex = v0;
        Vector1 = v1 - Vertex;
        Vector2 = v2 - Vertex;

        Normal = isDeterminantPositive ? Vector1 ^ Vector2 : Vector2 ^ Vector1;
        const float normalLength = Normal.Length();
        Area = normalLength * 0.5f;
        if (normalLength > ZeroTolerance)
        {
            Normal /= normalLength;
        }
    }

    void GetRandomPoint(Vector3& result) const
    {
        float x = Random::Rand();
        float y = Random::Rand();
        if (x + y > 1.0f)
        {
            x = 1.0f - x;
            y = 1.0f - y;
        }
        result = Vertex + x * Vector1 + y * Vector2;
    }
};

template<>
struct TIsPODType<GeometryTriangle>
{
    enum { Value = true };
};

struct GeometryLookup
{
    BoundingSphere Brush;
    Array<GeometryTriangle> Triangles;
    Array<Vector3> TerrainCache;

    GeometryLookup(const Vector3& brushPosition, float brushRadius)
        : Brush(brushPosition, brushRadius)
    {
    }

    static bool Search(Actor* actor, GeometryLookup& lookup)
    {
        // Early out if object is not intersecting with the foliage brush bounds
        if (!actor->GetIsActive() || !actor->GetBox().Intersects(lookup.Brush))
            return true;

        const auto brush = lookup.Brush;
        if (const auto* staticModel = dynamic_cast<StaticModel*>(actor))
        {
            // Skip if model is not loaded
            if (staticModel->Model == nullptr || staticModel->Model->WaitForLoaded() || staticModel->Model->GetLoadedLODs() == 0)
                return true;

            PROFILE_CPU_NAMED("StaticModel");

            // Check model meshes
            Matrix world;
            staticModel->GetWorld(&world);
            const bool isDeterminantPositive = staticModel->GetTransform().GetDeterminant() >= 0.0f;
            auto& lod = staticModel->Model->LODs[0];
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                auto& mesh = lod.Meshes[meshIndex];
                auto& proxy = mesh.GetCollisionProxy();

                // Check every triangle
                for (int32 triangleIndex = 0; triangleIndex < proxy.Triangles.Count(); triangleIndex++)
                {
                    auto t = proxy.Triangles[triangleIndex];

                    // Transform triangle vertices from mesh space to world space
                    Vector3 t0, t1, t2;
                    Vector3::Transform(t.V0, world, t0);
                    Vector3::Transform(t.V1, world, t1);
                    Vector3::Transform(t.V2, world, t2);

                    // Check if triangles intersects with the brush
                    if (CollisionsHelper::SphereIntersectsTriangle(brush, t0, t1, t2))
                    {
                        lookup.Triangles.Add(GeometryTriangle(isDeterminantPositive, t0, t1, t2));
                    }
                }
            }
        }
        else if (const auto* terrain = dynamic_cast<Terrain*>(actor))
        {
            const bool isDeterminantPositive = terrain->GetTransform().GetDeterminant() >= 0.0f;

            PROFILE_CPU_NAMED("Terrain");

            // Check every patch
            for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
            {
                auto patch = terrain->GetPatch(patchIndex);

                auto& triangles = lookup.TerrainCache;
                patch->GetCollisionTriangles(brush, triangles);

                for (int32 vertexIndex = 0; vertexIndex < triangles.Count();)
                {
                    const Vector3 t0 = triangles[vertexIndex++];
                    const Vector3 t1 = triangles[vertexIndex++];
                    const Vector3 t2 = triangles[vertexIndex++];

                    lookup.Triangles.Add(GeometryTriangle(isDeterminantPositive, t0, t1, t2));
                }
            }
        }
        else if (const auto* textRender = dynamic_cast<TextRender*>(actor))
        {
            PROFILE_CPU_NAMED("TextRender");

            // Skip if text is not ready
            if (textRender->GetCollisionProxy().Triangles.IsEmpty())
                return true;
            auto& proxy = textRender->GetCollisionProxy();

            // Check model meshes
            Matrix world;
            textRender->GetLocalToWorldMatrix(world);
            const bool isDeterminantPositive = textRender->GetTransform().GetDeterminant() >= 0.0f;

            // Check every triangle
            for (int32 triangleIndex = 0; triangleIndex < proxy.Triangles.Count(); triangleIndex++)
            {
                auto t = proxy.Triangles[triangleIndex];

                // Transform triangle vertices from mesh space to world space
                Vector3 t0, t1, t2;
                Vector3::Transform(t.V0, world, t0);
                Vector3::Transform(t.V1, world, t1);
                Vector3::Transform(t.V2, world, t2);

                // Check if triangles intersects with the brush
                if (CollisionsHelper::SphereIntersectsTriangle(brush, t0, t1, t2))
                {
                    lookup.Triangles.Add(GeometryTriangle(isDeterminantPositive, t0, t1, t2));
                }
            }
        }

        return true;
    }
};

struct FoliagePlacement
{
    int32 FoliageTypeIndex;
    Vector3 Location; // In world space
    Vector3 Normal; // In world space
};

void FoliageTools::Paint(Foliage* foliage, Span<int32> foliageTypesIndices, const Vector3& brushPosition, float brushRadius, bool additive, float densityScale)
{
    if (additive)
        Paint(foliage, foliageTypesIndices, brushPosition, brushRadius, densityScale);
    else
        Remove(foliage, foliageTypesIndices, brushPosition, brushRadius);
}

void FoliageTools::Paint(Foliage* foliage, Span<int32> foliageTypesIndices, const Vector3& brushPosition, float brushRadius, float densityScale)
{
    if (foliageTypesIndices.Length() <= 0)
        return;

    PROFILE_CPU();

    // Prepare
    GeometryLookup geometry(brushPosition, brushRadius);

    // Find geometry actors to place foliage on top of them
    {
        PROFILE_CPU_NAMED("Search Geometry");

        Function<bool(Actor*, GeometryLookup&)> treeWalkFunction(GeometryLookup::Search);
        SceneQuery::TreeExecute<GeometryLookup&>(treeWalkFunction, geometry);
    }

    // For each selected foliage instance type try to place something
    Array<FoliagePlacement> placements;
    {
        PROFILE_CPU_NAMED("Find Placements");

        for (int32 i1 = 0; i1 < foliageTypesIndices.Length(); i1++)
        {
            const int32 foliageTypeIndex = foliageTypesIndices[i1];
            ASSERT(foliageTypeIndex >= 0 && foliageTypeIndex < foliage->FoliageTypes.Count());
            const FoliageType& foliageType = foliage->FoliageTypes[foliageTypeIndex];

            // Prepare
            const float minNormalAngle = Math::Cos(foliageType.PaintGroundSlopeAngleMin * DegreesToRadians);
            const float maxNormalAngle = Math::Cos(foliageType.PaintGroundSlopeAngleMax * DegreesToRadians);
            const bool usePaintRadius = foliageType.PaintRadius > 0.0f;
            const float paintRadiusSqr = foliageType.PaintRadius * foliageType.PaintRadius;

            // Check every area
            for (int32 triangleIndex = 0; triangleIndex < geometry.Triangles.Count(); triangleIndex++)
            {
                const auto& triangle = geometry.Triangles[triangleIndex];

                // Check if can reject triangle based on its normal
                if ((maxNormalAngle > (triangle.Normal.Y + ZeroTolerance) || minNormalAngle < (triangle.Normal.Y - ZeroTolerance)))
                {
                    continue;
                }

                // Calculate amount of foliage instances to place
                const float targetInstanceCountEst = triangle.Area * foliageType.PaintDensity * densityScale / (1000.0f * 1000.0f);
                const int32 targetInstanceCount = targetInstanceCountEst > 1.0f ? Math::RoundToInt(targetInstanceCountEst) : Random::Rand() < targetInstanceCountEst ? 1 : 0;

                // Try to add new instances
                FoliagePlacement placement;
                for (int32 j = 0; j < targetInstanceCount; j++)
                {
                    triangle.GetRandomPoint(placement.Location);

                    // Reject locations outside the brush
                    if (CollisionsHelper::SphereContainsPoint(geometry.Brush, placement.Location) == ContainmentType::Disjoint)
                        continue;

                    // Check if it's too close to any other instances
                    if (usePaintRadius)
                    {
                        // Skip if any places instance is close that placement location
                        bool isInvalid = false;
                        // TODO: use quad tree to boost this logic
                        for (auto i = foliage->Instances.Begin(); i.IsNotEnd(); ++i)
                        {
                            const auto& instance = *i;
                            if (Vector3::DistanceSquared(instance.World.GetTranslation(), placement.Location) <= paintRadiusSqr)
                            {
                                isInvalid = true;
                                break;
                            }
                        }
                        if (isInvalid)
                            continue;

                        // Skip if any places instance is close that placement location
                        isInvalid = false;
                        for (int32 i = 0; i < placements.Count(); i++)
                        {
                            if (Vector3::DistanceSquared(placements[i].Location, placement.Location) <= paintRadiusSqr)
                            {
                                isInvalid = true;
                                break;
                            }
                        }
                        if (isInvalid)
                            continue;
                    }

                    placement.FoliageTypeIndex = foliageTypeIndex;
                    placement.Normal = triangle.Normal;
                    placements.Add(placement);
                }
            }
        }
    }

    // Place foliage instances
    if (placements.HasItems())
    {
        PROFILE_CPU_NAMED("Place Instances");

        Matrix matrix;
        FoliageInstance instance;
        Quaternion tmp;
        Matrix world;
        foliage->GetLocalToWorldMatrix(world);

        for (int32 i = 0; i < placements.Count(); i++)
        {
            const auto& placement = placements[i];
            const FoliageType& foliageType = foliage->FoliageTypes[placement.FoliageTypeIndex];
            const Vector3 normal = foliageType.PlacementAlignToNormal ? placement.Normal : Vector3::Up;

            if (normal == Vector3::Down)
                instance.Transform.Orientation = Quaternion(0.0f, 0.0f, Math::Sin(PI_OVER_2), Math::Cos(PI_OVER_2));
            else
                instance.Transform.Orientation = Quaternion::LookRotation(Vector3::Cross(Vector3::Cross(normal, Vector3::Forward), normal), normal);

            if (foliageType.PlacementRandomYaw)
            {
                Quaternion::RotationAxis(Vector3::UnitY, Random::Rand() * TWO_PI, tmp);
                instance.Transform.Orientation *= tmp;
            }

            if (!Math::IsZero(foliageType.PlacementRandomRollAngle))
            {
                Quaternion::RotationAxis(Vector3::UnitZ, Random::Rand() * DegreesToRadians * foliageType.PlacementRandomRollAngle, tmp);
                instance.Transform.Orientation *= tmp;
            }

            if (!Math::IsZero(foliageType.PlacementRandomPitchAngle))
            {
                Quaternion::RotationAxis(Vector3::UnitX, Random::Rand() * DegreesToRadians * foliageType.PlacementRandomPitchAngle, tmp);
                instance.Transform.Orientation *= tmp;
            }

            instance.Type = placement.FoliageTypeIndex;
            instance.Random = Random::Rand();
            instance.Transform.Translation = placement.Location;
            if (!foliageType.PlacementOffsetY.IsZero())
            {
                float offsetY = Math::Lerp(foliageType.PlacementOffsetY.X, foliageType.PlacementOffsetY.Y, Random::Rand());
                instance.Transform.Translation += (instance.Transform.Orientation * Vector3::Up) * offsetY;
            }
            instance.Transform.Scale = foliageType.GetRandomScale();
            instance.Transform.Orientation.Normalize();

            // Convert instance transformation into the local-space of the foliage actor
            foliage->GetTransform().WorldToLocal(instance.Transform, instance.Transform);

            // Calculate foliage instance geometry transformation matrix
            instance.Transform.GetWorld(matrix);
            Matrix::Multiply(matrix, world, instance.World);
            instance.DrawState.PrevWorld = instance.World;

            // Add foliage instance
            foliage->AddInstance(instance);
        }

        foliage->RebuildClusters();
    }
}

void FoliageTools::Remove(Foliage* foliage, Span<int32> foliageTypesIndices, const Vector3& brushPosition, float brushRadius)
{
    if (foliageTypesIndices.Length() <= 0)
        return;

    PROFILE_CPU();

    // For each selected foliage instance type try to remove something
    const BoundingSphere brush(brushPosition, brushRadius);
    for (auto i = foliage->Instances.Begin(); i.IsNotEnd(); ++i)
    {
        auto& instance = *i;

        // Skip instances outside the brush
        if (CollisionsHelper::SphereContainsPoint(brush, instance.World.GetTranslation()) == ContainmentType::Disjoint)
            continue;

        // Skip instances not existing in a filter
        bool skip = true;
        for (int32 i1 = 0; i1 < foliageTypesIndices.Length(); i1++)
        {
            if (foliageTypesIndices[i1] == instance.Type)
            {
                skip = false;
                break;
            }
        }
        if (skip)
            continue;

        // Remove instance
        foliage->RemoveInstance(i);
        --i;
    }

    foliage->RebuildClusters();
}
