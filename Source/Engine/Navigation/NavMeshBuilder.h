// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_NAV_MESH_BUILDER

class Scene;

/// <summary>
/// The navigation mesh building utility.
/// </summary>
class FLAXENGINE_API NavMeshBuilder
{
public:

    static void Init();
    static bool IsBuildingNavMesh();
    static float GetNavMeshBuildingProgress();
    static void Update();
    static void Build(Scene* scene, float timeoutMs);
    static void Build(Scene* scene, const struct BoundingBox& dirtyBounds, float timeoutMs);
};

#endif
