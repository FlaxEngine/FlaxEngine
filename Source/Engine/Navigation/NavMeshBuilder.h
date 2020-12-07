// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_NAV_MESH_BUILDER

class Scene;

class NavMeshBuilder
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
