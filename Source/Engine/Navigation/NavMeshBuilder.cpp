// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_NAV_MESH_BUILDER

#include "NavMeshBuilder.h"
#include "NavMesh.h"
#include "NavigationSettings.h"
#include "NavMeshBoundsVolume.h"
#include "NavLink.h"
#include "NavModifierVolume.h"
#include "NavMeshRuntime.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Physics/Colliders/BoxCollider.h"
#include "Engine/Physics/Colliders/SphereCollider.h"
#include "Engine/Physics/Colliders/CapsuleCollider.h"
#include "Engine/Physics/Colliders/MeshCollider.h"
#include "Engine/Physics/Colliders/SplineCollider.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Terrain/TerrainPatch.h"
#include "Engine/Terrain/Terrain.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Level.h"
#include <ThirdParty/recastnavigation/Recast.h>
#include <ThirdParty/recastnavigation/DetourNavMeshBuilder.h>
#include <ThirdParty/recastnavigation/DetourNavMesh.h>

int32 BoxTrianglesIndicesCache[] =
{
	// @formatter:off
	3, 1, 2,
	3, 0, 1,
	7, 0, 3,
	7, 4, 0,
	7, 6, 5,
	7, 5, 4,
	6, 2, 1,
	6, 1, 5,
	1, 0, 4,
	1, 4, 5,
	7, 2, 6,
	7, 3, 2,
    // @formatter:on
};

#define NAV_MESH_TILE_MAX_EXTENT 100000000
#define NAV_MESH_BUILD_DEBUG_DRAW_GEOMETRY 0

#if NAV_MESH_BUILD_DEBUG_DRAW_GEOMETRY
#include "Engine/Debug/DebugDraw.h"
#endif

struct OffMeshLink
{
    Float3 Start;
    Float3 End;
    float Radius;
    bool BiDir;
    int32 Id;
};

struct Modifier
{
    BoundingBox Bounds;
    NavAreaProperties* NavArea;
};

struct NavSceneRasterizer
{
    NavMesh* NavMesh;
    BoundingBox TileBoundsNavMesh;
    Matrix WorldToNavMesh;
    rcContext* Context;
    rcConfig* Config;
    rcHeightfield* Heightfield;
    float WalkableThreshold;
    Array<Float3> VertexBuffer;
    Array<int32> IndexBuffer;
    Array<OffMeshLink>* OffMeshLinks;
    Array<Modifier>* Modifiers;
    const bool IsWorldToNavMeshIdentity;

    NavSceneRasterizer(::NavMesh* navMesh, const BoundingBox& tileBoundsNavMesh, const Matrix& worldToNavMesh, rcContext* context, rcConfig* config, rcHeightfield* heightfield, Array<OffMeshLink>* offMeshLinks, Array<Modifier>* modifiers)
        : TileBoundsNavMesh(tileBoundsNavMesh)
        , WorldToNavMesh(worldToNavMesh)
        , IsWorldToNavMeshIdentity(worldToNavMesh.IsIdentity())
    {
        NavMesh = navMesh;
        Context = context;
        Config = config;
        Heightfield = heightfield;
        WalkableThreshold = Math::Cos(config->walkableSlopeAngle * DegreesToRadians);
        OffMeshLinks = offMeshLinks;
        Modifiers = modifiers;
    }

    void RasterizeTriangles()
    {
        auto& vb = VertexBuffer;
        auto& ib = IndexBuffer;
        if (vb.IsEmpty() || ib.IsEmpty())
            return;
        PROFILE_CPU();

        // Rasterize triangles
        const Float3* vbData = vb.Get();
        const int32* ibData = ib.Get();
        Float3 v0, v1, v2;
        if (IsWorldToNavMeshIdentity)
        {
            // Faster path
            for (int32 i0 = 0; i0 < ib.Count();)
            {
                v0 = vbData[ibData[i0++]];
                v1 = vbData[ibData[i0++]];
                v2 = vbData[ibData[i0++]];
#if NAV_MESH_BUILD_DEBUG_DRAW_GEOMETRY
                DEBUG_DRAW_TRIANGLE(v0, v1, v2, Color::Orange.AlphaMultiplied(0.3f), 1.0f, true);
#endif

                auto n = Float3::Cross(v0 - v1, v0 - v2);
                n.Normalize();
                const char area = n.Y > WalkableThreshold ? RC_WALKABLE_AREA : RC_NULL_AREA;
                rcRasterizeTriangle(Context, &v0.X, &v1.X, &v2.X, area, *Heightfield);
            }
        }
        else
        {
            // Transform vertices from world space into the navmesh space
            const Matrix worldToNavMesh = WorldToNavMesh;
            for (int32 i0 = 0; i0 < ib.Count();)
            {
                Float3::Transform(vbData[ibData[i0++]], worldToNavMesh, v0);
                Float3::Transform(vbData[ibData[i0++]], worldToNavMesh, v1);
                Float3::Transform(vbData[ibData[i0++]], worldToNavMesh, v2);
#if NAV_MESH_BUILD_DEBUG_DRAW_GEOMETRY
                DEBUG_DRAW_TRIANGLE(v0, v1, v2, Color::Orange.AlphaMultiplied(0.3f), 1.0f, true);
#endif

                auto n = Float3::Cross(v0 - v1, v0 - v2);
                n.Normalize();
                const char area = n.Y > WalkableThreshold ? RC_WALKABLE_AREA : RC_NULL_AREA;
                rcRasterizeTriangle(Context, &v0.X, &v1.X, &v2.X, area, *Heightfield);
            }
        }

        // Clear after use
        vb.Clear();
        ib.Clear();
    }

    static void TriangulateBox(Array<Float3>& vb, Array<int32>& ib, const OrientedBoundingBox& box)
    {
        vb.Resize(8);
        box.GetCorners(vb.Get());
        ib.Add(BoxTrianglesIndicesCache, 36);
    }

    static void TriangulateBox(Array<Float3>& vb, Array<int32>& ib, const BoundingBox& box)
    {
        vb.Resize(8);
        box.GetCorners(vb.Get());
        ib.Add(BoxTrianglesIndicesCache, 36);
    }

    static void TriangulateSphere(Array<Float3>& vb, Array<int32>& ib, const BoundingSphere& sphere)
    {
        const int32 sphereResolution = 12;
        const int32 verticalSegments = sphereResolution;
        const int32 horizontalSegments = sphereResolution * 2;

        // Generate vertices for unit sphere
        Float3 vertices[(verticalSegments + 1) * (horizontalSegments + 1)];
        int32 vertexCount = 0;
        for (int32 j = 0; j <= horizontalSegments; j++)
            vertices[vertexCount++] = Float3(0, -1, 0);
        for (int32 i = 1; i < verticalSegments; i++)
        {
            const float latitude = (float)i * PI / verticalSegments - PI / 2.0f;
            const float dy = Math::Sin(latitude);
            const float dxz = Math::Cos(latitude);
            auto& firstHorizontalVertex = vertices[vertexCount++];
            firstHorizontalVertex = Float3(0, dy, dxz);
            for (int32 j = 1; j < horizontalSegments; j++)
            {
                const float longitude = (float)j * 2.0f * PI / horizontalSegments;
                const float dx = Math::Sin(longitude) * dxz;
                const float dz = Math::Cos(longitude) * dxz;
                vertices[vertexCount++] = Float3(dx, dy, dz);
            }
            vertices[vertexCount++] = firstHorizontalVertex;
        }
        for (int32 j = 0; j <= horizontalSegments; j++)
            vertices[vertexCount++] = Float3(0, 1, 0);

        // Transform vertices into world space vertex buffer
        vb.Resize(vertexCount);
        for (int32 i = 0; i < vertexCount; i++)
            vb[i] = sphere.Center + vertices[i] * sphere.Radius;

        // Generate index buffer
        const int32 stride = horizontalSegments + 1;
        int32 indexCount = 0;
        ib.Resize(verticalSegments * (horizontalSegments + 1) * 6);
        for (int32 i = 0; i < verticalSegments; i++)
        {
            const int32 nextI = i + 1;
            for (int32 j = 0; j <= horizontalSegments; j++)
            {
                const int32 nextJ = (j + 1) % stride;

                ib[indexCount++] = i * stride + j;
                ib[indexCount++] = nextI * stride + j;
                ib[indexCount++] = i * stride + nextJ;

                ib[indexCount++] = i * stride + nextJ;
                ib[indexCount++] = nextI * stride + j;
                ib[indexCount++] = nextI * stride + nextJ;
            }
        }
    }

    void Rasterize(Actor* actor)
    {
        if (const auto* boxCollider = dynamic_cast<BoxCollider*>(actor))
        {
            if (boxCollider->GetIsTrigger())
                return;
            PROFILE_CPU_NAMED("BoxCollider");

            const OrientedBoundingBox box = boxCollider->GetOrientedBox();
            TriangulateBox(VertexBuffer, IndexBuffer, box);
            RasterizeTriangles();
        }
        else if (const auto* sphereCollider = dynamic_cast<SphereCollider*>(actor))
        {
            if (sphereCollider->GetIsTrigger())
                return;
            PROFILE_CPU_NAMED("SphereCollider");

            const BoundingSphere sphere = sphereCollider->GetSphere();
            TriangulateSphere(VertexBuffer, IndexBuffer, sphere);
            RasterizeTriangles();
        }
        else if (const auto* capsuleCollider = dynamic_cast<CapsuleCollider*>(actor))
        {
            if (capsuleCollider->GetIsTrigger())
                return;
            PROFILE_CPU_NAMED("CapsuleCollider");

            const BoundingBox box = capsuleCollider->GetBox();
            TriangulateBox(VertexBuffer, IndexBuffer, box);
            RasterizeTriangles();
        }
        else if (const auto* meshCollider = dynamic_cast<MeshCollider*>(actor))
        {
            if (meshCollider->GetIsTrigger())
                return;
            PROFILE_CPU_NAMED("MeshCollider");

            auto collisionData = meshCollider->CollisionData.Get();
            if (!collisionData || collisionData->WaitForLoaded())
                return;

            collisionData->ExtractGeometry(VertexBuffer, IndexBuffer);
            Matrix meshColliderToWorld;
            meshCollider->GetLocalToWorldMatrix(meshColliderToWorld);
            for (auto& v : VertexBuffer)
                Float3::Transform(v, meshColliderToWorld, v);
            RasterizeTriangles();
        }
        else if (const auto* splineCollider = dynamic_cast<SplineCollider*>(actor))
        {
            if (splineCollider->GetIsTrigger())
                return;
            PROFILE_CPU_NAMED("SplineCollider");

            auto collisionData = splineCollider->CollisionData.Get();
            if (!collisionData || collisionData->WaitForLoaded())
                return;

            splineCollider->ExtractGeometry(VertexBuffer, IndexBuffer);
            RasterizeTriangles();
        }
        else if (const auto* terrain = dynamic_cast<Terrain*>(actor))
        {
            PROFILE_CPU_NAMED("Terrain");

            for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
            {
                const auto patch = terrain->GetPatch(patchIndex);
                BoundingBox patchBoundsNavMesh;
                BoundingBox::Transform(patch->GetBounds(), WorldToNavMesh, patchBoundsNavMesh);
                if (!patchBoundsNavMesh.Intersects(TileBoundsNavMesh))
                    continue;

                // TODO: get collision only from tile area
                patch->ExtractCollisionGeometry(VertexBuffer, IndexBuffer);
                RasterizeTriangles();
            }
        }
        else if (const auto* navLink = dynamic_cast<NavLink*>(actor))
        {
            PROFILE_CPU_NAMED("NavLink");

            OffMeshLink link;
            link.Start = navLink->GetTransform().LocalToWorld(navLink->Start);
            Float3::Transform(link.Start, WorldToNavMesh, link.Start);
            link.End = navLink->GetTransform().LocalToWorld(navLink->End);
            Float3::Transform(link.End, WorldToNavMesh, link.End);
            link.Radius = navLink->Radius;
            link.BiDir = navLink->BiDirectional;
            link.Id = GetHash(navLink->GetID());

            OffMeshLinks->Add(link);
        }
        else if (const auto* navModifierVolume = dynamic_cast<NavModifierVolume*>(actor))
        {
            if (navModifierVolume->AgentsMask.IsNavMeshSupported(NavMesh->Properties))
            {
                PROFILE_CPU_NAMED("NavModifierVolume");

                Modifier modifier;
                OrientedBoundingBox bounds = navModifierVolume->GetOrientedBox();
                bounds.Transform(WorldToNavMesh);
                bounds.GetBoundingBox(modifier.Bounds);
                modifier.NavArea = navModifierVolume->GetNavArea();

                Modifiers->Add(modifier);
            }
        }
    }
};

// Builds navmesh tile bounds and check if there are any valid navmesh volumes at that tile location
// Returns true if tile is intersecting with any navmesh bounds volume actor - which means tile is in use
bool GetNavMeshTileBounds(Scene* scene, NavMesh* navMesh, int32 x, int32 y, float tileSize, BoundingBox& tileBoundsNavMesh, const Matrix& worldToNavMesh)
{
    // Build initial tile bounds (with infinite extent)
    tileBoundsNavMesh.Minimum.X = (float)x * tileSize;
    tileBoundsNavMesh.Minimum.Y = -NAV_MESH_TILE_MAX_EXTENT;
    tileBoundsNavMesh.Minimum.Z = (float)y * tileSize;
    tileBoundsNavMesh.Maximum.X = tileBoundsNavMesh.Minimum.X + tileSize;
    tileBoundsNavMesh.Maximum.Y = NAV_MESH_TILE_MAX_EXTENT;
    tileBoundsNavMesh.Maximum.Z = tileBoundsNavMesh.Minimum.Z + tileSize;

    // Check if any navmesh volume intersects with the tile
    bool foundAnyVolume = false;
    Vector2 rangeY;
    for (int32 i = 0; i < scene->Navigation.Volumes.Count(); i++)
    {
        const auto volume = scene->Navigation.Volumes[i];
        if (!volume->AgentsMask.IsNavMeshSupported(navMesh->Properties))
            continue;
        const auto& volumeBounds = volume->GetBox();
        BoundingBox volumeBoundsNavMesh;
        BoundingBox::Transform(volumeBounds, worldToNavMesh, volumeBoundsNavMesh);
        if (volumeBoundsNavMesh.Intersects(tileBoundsNavMesh))
        {
            if (foundAnyVolume)
            {
                rangeY.X = Math::Min(rangeY.X, volumeBoundsNavMesh.Minimum.Y);
                rangeY.Y = Math::Max(rangeY.Y, volumeBoundsNavMesh.Maximum.Y);
            }
            else
            {
                rangeY.X = volumeBoundsNavMesh.Minimum.Y;
                rangeY.Y = volumeBoundsNavMesh.Maximum.Y;
            }
            foundAnyVolume = true;
        }
    }

    if (foundAnyVolume)
    {
        // Build proper tile bounds
        tileBoundsNavMesh.Minimum.Y = rangeY.X;
        tileBoundsNavMesh.Maximum.Y = rangeY.Y;
    }

    return foundAnyVolume;
}

void RemoveTile(NavMesh* navMesh, NavMeshRuntime* runtime, int32 x, int32 y, int32 layer)
{
    ScopeLock lock(runtime->Locker);

    // Find tile data and remove it
    for (int32 i = 0; i < navMesh->Data.Tiles.Count(); i++)
    {
        auto& tile = navMesh->Data.Tiles[i];
        if (tile.PosX == x && tile.PosY == y && tile.Layer == layer)
        {
            navMesh->Data.Tiles.RemoveAt(i);
            navMesh->IsDataDirty = true;
            break;
        }
    }

    // Remove tile from navmesh
    runtime->RemoveTile(x, y, layer);
}

bool GenerateTile(NavMesh* navMesh, NavMeshRuntime* runtime, int32 x, int32 y, BoundingBox& tileBoundsNavMesh, const Matrix& worldToNavMesh, float tileSize, rcConfig& config)
{
    rcContext context;
    int32 layer = 0;

    // Expand tile bounds by a certain margin
    const float tileBorderSize = (1.0f + (float)config.borderSize) * config.cs;
    tileBoundsNavMesh.Minimum -= tileBorderSize;
    tileBoundsNavMesh.Maximum += tileBorderSize;

    *(Float3*)&config.bmin = tileBoundsNavMesh.Minimum;
    *(Float3*)&config.bmax = tileBoundsNavMesh.Maximum;

    rcHeightfield* heightfield = rcAllocHeightfield();
    if (!heightfield)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for heightfield.");
        return true;
    }
    if (!rcCreateHeightfield(&context, *heightfield, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
    {
        LOG(Warning, "Could not generate navmesh: Could not create solid heightfield.");
        return true;
    }

    Array<OffMeshLink> offMeshLinks;
    Array<Modifier> modifiers;
    {
        PROFILE_CPU_NAMED("RasterizeGeometry");
        NavSceneRasterizer rasterizer(navMesh, tileBoundsNavMesh, worldToNavMesh, &context, &config, heightfield, &offMeshLinks, &modifiers);

        // Collect actors to rasterize
        Array<Actor*> actors;
        {
            PROFILE_CPU_NAMED("CollectActors");
            ScopeLock lock(Level::ScenesLock);
            for (Scene* scene : Level::Scenes)
            {
                for (Actor* actor : scene->Navigation.Actors)
                {
                    BoundingBox actorBoxNavMesh;
                    BoundingBox::Transform(actor->GetBox(), rasterizer.WorldToNavMesh, actorBoxNavMesh);
                    if (actorBoxNavMesh.Intersects(rasterizer.TileBoundsNavMesh) &&
                        actor->IsActiveInHierarchy() &&
                        EnumHasAllFlags(actor->GetStaticFlags(), StaticFlags::Navigation))
                    {
                        actors.Add(actor);
                    }
                }
            }
        }

        // Rasterize actors
        for (Actor* actor : actors)
        {
            rasterizer.Rasterize(actor);
        }
    }

    {
        PROFILE_CPU_NAMED("FilterHeightfield");
        rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *heightfield);
        rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *heightfield);
        rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *heightfield);
    }

    rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
    if (!compactHeightfield)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory compact heightfield.");
        return true;
    }
    {
        PROFILE_CPU_NAMED("CompactHeightfield");
        if (!rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *heightfield, *compactHeightfield))
        {
            LOG(Warning, "Could not generate navmesh: Could not build compact data.");
            return true;
        }
    }
    rcFreeHeightField(heightfield);
    {
        PROFILE_CPU_NAMED("ErodeWalkableArea");
        if (!rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield))
        {
            LOG(Warning, "Could not generate navmesh: Could not erode.");
            return true;
        }
    }

    // Mark areas
    {
        PROFILE_CPU_NAMED("MarkModifiers");
        for (auto& modifier : modifiers)
        {
            const unsigned char areaId = modifier.NavArea ? modifier.NavArea->Id : RC_NULL_AREA;
            Float3 bMin = modifier.Bounds.Minimum;
            Float3 bMax = modifier.Bounds.Maximum;
            rcMarkBoxArea(&context, &bMin.X, &bMax.X, areaId, *compactHeightfield);
        }
    }

    {
        PROFILE_CPU_NAMED("BuildDistanceField");
        if (!rcBuildDistanceField(&context, *compactHeightfield))
        {
            LOG(Warning, "Could not generate navmesh: Could not build distance field.");
            return true;
        }
    }
    {
        PROFILE_CPU_NAMED("BuildRegions");
        if (!rcBuildRegions(&context, *compactHeightfield, config.borderSize, config.minRegionArea, config.mergeRegionArea))
        {
            LOG(Warning, "Could not generate navmesh: Could not build regions.");
            return true;
        }
    }

    rcContourSet* contourSet = rcAllocContourSet();
    if (!contourSet)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for contour set.");
        return true;
    }
    {
        PROFILE_CPU_NAMED("BuildContours");
        if (!rcBuildContours(&context, *compactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *contourSet))
        {
            LOG(Warning, "Could not generate navmesh: Could not create contours.");
            return true;
        }
    }

    rcPolyMesh* polyMesh = rcAllocPolyMesh();
    if (!polyMesh)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for poly mesh.");
        return true;
    }
    {
        PROFILE_CPU_NAMED("BuildPolyMesh");
        if (!rcBuildPolyMesh(&context, *contourSet, config.maxVertsPerPoly, *polyMesh))
        {
            LOG(Warning, "Could not generate navmesh: Could not triangulate contours.");
            return true;
        }
    }

    rcPolyMeshDetail* detailMesh = rcAllocPolyMeshDetail();
    if (!detailMesh)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for detail mesh.");
        return true;
    }
    {
        PROFILE_CPU_NAMED("BuildPolyMeshDetail");
        if (!rcBuildPolyMeshDetail(&context, *polyMesh, *compactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *detailMesh))
        {
            LOG(Warning, "Could not generate navmesh: Could not build detail mesh.");
            return true;
        }
    }

    rcFreeCompactHeightfield(compactHeightfield);
    rcFreeContourSet(contourSet);

    for (int i = 0; i < polyMesh->npolys; i++)
        polyMesh->flags[i] = polyMesh->areas[i] != RC_NULL_AREA ? 1 : 0;
    if (polyMesh->nverts == 0)
    {
        // Empty tile
        RemoveTile(navMesh, runtime, x, y, layer);
        return false;
    }

    dtNavMeshCreateParams params;
    Platform::MemoryClear(&params, sizeof(params));
    params.verts = polyMesh->verts;
    params.vertCount = polyMesh->nverts;
    params.polys = polyMesh->polys;
    params.polyAreas = polyMesh->areas;
    params.polyFlags = polyMesh->flags;
    params.polyCount = polyMesh->npolys;
    params.nvp = polyMesh->nvp;
    params.detailMeshes = detailMesh->meshes;
    params.detailVerts = detailMesh->verts;
    params.detailVertsCount = detailMesh->nverts;
    params.detailTris = detailMesh->tris;
    params.detailTriCount = detailMesh->ntris;
    params.walkableHeight = (float)config.walkableHeight * config.ch;
    params.walkableRadius = (float)config.walkableRadius * config.cs;
    params.walkableClimb = (float)config.walkableClimb * config.ch;
    params.tileX = x;
    params.tileY = y;
    params.tileLayer = layer;
    rcVcopy(params.bmin, polyMesh->bmin);
    rcVcopy(params.bmax, polyMesh->bmax);
    params.cs = config.cs;
    params.ch = config.ch;
    params.buildBvTree = false;

    // Prepare navmesh links
    Array<Float3> offMeshStartEnd;
    Array<float> offMeshRadius;
    Array<unsigned char> offMeshDir;
    Array<unsigned char> offMeshArea;
    Array<unsigned short> offMeshFlags;
    Array<unsigned int> offMeshId;
    if (offMeshLinks.HasItems())
    {
        int32 linksCount = offMeshLinks.Count();
        offMeshStartEnd.Resize(linksCount * 2);
        offMeshRadius.Resize(linksCount);
        offMeshDir.Resize(linksCount);
        offMeshArea.Resize(linksCount);
        offMeshFlags.Resize(linksCount);
        offMeshId.Resize(linksCount);

        for (int32 i = 0; i < linksCount; i++)
        {
            auto& link = offMeshLinks[i];

            offMeshStartEnd[i * 2] = link.Start;
            offMeshStartEnd[i * 2 + 1] = link.End;
            offMeshRadius[i] = link.Radius;
            offMeshDir[i] = link.BiDir ? DT_OFFMESH_CON_BIDIR : 0;
            offMeshId[i] = link.Id;
            offMeshArea[i] = RC_WALKABLE_AREA;
            offMeshFlags[i] = 1;

            // TODO: support navigation area type for off mesh links
        }

        params.offMeshConCount = linksCount;
        params.offMeshConVerts = (const float*)offMeshStartEnd.Get();
        params.offMeshConRad = offMeshRadius.Get();
        params.offMeshConDir = offMeshDir.Get();
        params.offMeshConAreas = offMeshArea.Get();
        params.offMeshConFlags = offMeshFlags.Get();
        params.offMeshConUserID = offMeshId.Get();
    }

    // Generate navmesh tile data
    unsigned char* navData = nullptr;
    int navDataSize = 0;
    {
        PROFILE_CPU_NAMED("CreateNavMeshData");
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            LOG(Warning, "Could not build Detour navmesh.");
            return true;
        }
    }
    ASSERT_LOW_LAYER(navDataSize > 4 && *(uint32*)navData == DT_NAVMESH_MAGIC); // Sanity check for Detour header

    {
        PROFILE_CPU_NAMED("CreateTiles");

        ScopeLock lock(runtime->Locker);

        navMesh->IsDataDirty = true;
        NavMeshTileData* tile = nullptr;
        for (int32 i = 0; i < navMesh->Data.Tiles.Count(); i++)
        {
            auto& e = navMesh->Data.Tiles[i];
            if (e.PosX == x && e.PosY == y && e.Layer == layer)
            {
                tile = &e;
                break;
            }
        }
        if (!tile)
        {
            // Add new tile
            tile = &navMesh->Data.Tiles.AddOne();
            tile->PosX = x;
            tile->PosY = y;
            tile->Layer = layer;
        }

        // Copy data to the tile
        tile->Data.Copy(navData, navDataSize);

        // Add tile to navmesh
        runtime->AddTile(navMesh, *tile);
    }

    dtFree(navData);

    return false;
}

float GetTileSize()
{
    auto& settings = *NavigationSettings::Get();
    return settings.CellSize * settings.TileSize;
}

void InitConfig(rcConfig& config, NavMesh* navMesh)
{
    auto& settings = *NavigationSettings::Get();
    auto& navMeshProperties = navMesh->Properties;

    config.cs = settings.CellSize;
    config.ch = settings.CellHeight;
    config.walkableSlopeAngle = navMeshProperties.Agent.MaxSlopeAngle;
    config.walkableHeight = (int)(navMeshProperties.Agent.Height / config.ch + 0.99f);
    config.walkableClimb = (int)(navMeshProperties.Agent.StepHeight / config.ch);
    config.walkableRadius = (int)(navMeshProperties.Agent.Radius / config.cs + 0.99f);
    config.maxEdgeLen = (int)(settings.MaxEdgeLen / config.cs);
    config.maxSimplificationError = settings.MaxEdgeError;
    config.minRegionArea = rcSqr(settings.MinRegionArea);
    config.mergeRegionArea = rcSqr(settings.MergeRegionArea);
    config.maxVertsPerPoly = 6;
    config.detailSampleDist = config.cs * settings.DetailSamplingDist;
    config.detailSampleMaxError = config.ch * settings.MaxDetailSamplingError;
    config.borderSize = config.walkableRadius + 3;
    config.tileSize = settings.TileSize;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
}

struct BuildRequest
{
    ScriptingObjectReference<Scene> Scene;
    DateTime Time;
    BoundingBox DirtyBounds;
};

CriticalSection NavBuildQueueLocker;
Array<BuildRequest> NavBuildQueue;

CriticalSection NavBuildTasksLocker;
int32 NavBuildTasksMaxCount = 0;
Array<class NavMeshTileBuildTask*> NavBuildTasks;

class NavMeshTileBuildTask : public ThreadPoolTask
{
public:
    Scene* Scene;
    ScriptingObjectReference<NavMesh> NavMesh;
    NavMeshRuntime* Runtime;
    BoundingBox TileBoundsNavMesh;
    Matrix WorldToNavMesh;
    int32 X;
    int32 Y;
    float TileSize;
    rcConfig Config;

public:
    // [ThreadPoolTask]
    bool Run() override
    {
        PROFILE_CPU_NAMED("BuildNavMeshTile");
        const auto navMesh = NavMesh.Get();
        if (!navMesh)
            return false;
        if (GenerateTile(NavMesh, Runtime, X, Y, TileBoundsNavMesh, WorldToNavMesh, TileSize, Config))
        {
            LOG(Warning, "Failed to generate navmesh tile at {0}x{1}.", X, Y);
        }
        return false;
    }

    void OnEnd() override
    {
        // Remove from tasks list
        ScopeLock lock(NavBuildTasksLocker);
        NavBuildTasks.Remove(this);
        if (NavBuildTasks.IsEmpty())
            NavBuildTasksMaxCount = 0;
    }
};

void OnSceneUnloading(Scene* scene, const Guid& sceneId)
{
    // Cancel pending build requests
    NavBuildQueueLocker.Lock();
    for (int32 i = 0; i < NavBuildQueue.Count(); i++)
    {
        if (NavBuildQueue[i].Scene == scene)
        {
            NavBuildQueue.RemoveAtKeepOrder(i);
            break;
        }
    }
    NavBuildQueueLocker.Unlock();

    // Cancel active build tasks
    NavBuildTasksLocker.Lock();
    for (int32 i = 0; i < NavBuildTasks.Count(); i++)
    {
        auto task = NavBuildTasks[i];
        if (task->Scene == scene)
        {
            NavBuildTasksLocker.Unlock();

            // Cancel task but without locking queue from this thread to prevent deadlocks
            task->Cancel();

            NavBuildTasksLocker.Lock();
            i--;
            if (NavBuildTasks.IsEmpty())
                break;
        }
    }
    NavBuildTasksLocker.Unlock();
}

void NavMeshBuilder::Init()
{
    Level::SceneUnloading.Bind<OnSceneUnloading>();
}

bool NavMeshBuilder::IsBuildingNavMesh()
{
    NavBuildTasksLocker.Lock();
    const bool hasAnyTask = NavBuildTasks.HasItems();
    NavBuildTasksLocker.Unlock();

    return hasAnyTask;
}

float NavMeshBuilder::GetNavMeshBuildingProgress()
{
    NavBuildTasksLocker.Lock();
    float result = 1.0f;
    if (NavBuildTasksMaxCount != 0)
    {
        result = (float)(NavBuildTasksMaxCount - NavBuildTasks.Count()) / NavBuildTasksMaxCount;
    }
    NavBuildTasksLocker.Unlock();

    return result;
}

void BuildTileAsync(NavMesh* navMesh, const int32 x, const int32 y, const rcConfig& config, const BoundingBox& tileBoundsNavMesh, const Matrix& worldToNavMesh, float tileSize)
{
    NavMeshRuntime* runtime = navMesh->GetRuntime();
    NavBuildTasksLocker.Lock();

    // Skip if this tile is already during cooking
    for (int32 i = 0; i < NavBuildTasks.Count(); i++)
    {
        const auto task = NavBuildTasks[i];
        if (task->X == x && task->Y == y && task->Runtime == runtime)
        {
            NavBuildTasksLocker.Unlock();
            return;
        }
    }

    // Create task
    auto task = New<NavMeshTileBuildTask>();
    task->Scene = navMesh->GetScene();
    task->NavMesh = navMesh;
    task->Runtime = runtime;
    task->X = x;
    task->Y = y;
    task->TileBoundsNavMesh = tileBoundsNavMesh;
    task->WorldToNavMesh = worldToNavMesh;
    task->TileSize = tileSize;
    task->Config = config;
    NavBuildTasks.Add(task);
    NavBuildTasksMaxCount++;

    NavBuildTasksLocker.Unlock();

    // Invoke job
    task->Start();
}

void BuildDirtyBounds(Scene* scene, NavMesh* navMesh, const BoundingBox& dirtyBounds, bool rebuild)
{
    const float tileSize = GetTileSize();
    NavMeshRuntime* runtime = navMesh->GetRuntime();
    Matrix worldToNavMesh;
    Matrix::RotationQuaternion(runtime->Properties.Rotation, worldToNavMesh);

    // Align dirty bounds to tile size
    BoundingBox dirtyBoundsNavMesh;
    BoundingBox::Transform(dirtyBounds, worldToNavMesh, dirtyBoundsNavMesh);
    BoundingBox dirtyBoundsAligned;
    dirtyBoundsAligned.Minimum = Float3::Floor(dirtyBoundsNavMesh.Minimum / tileSize) * tileSize;
    dirtyBoundsAligned.Maximum = Float3::Ceil(dirtyBoundsNavMesh.Maximum / tileSize) * tileSize;

    // Calculate tiles range for the given navigation dirty bounds (aligned to tiles size)
    const Int3 tilesMin(dirtyBoundsAligned.Minimum / tileSize);
    const Int3 tilesMax(dirtyBoundsAligned.Maximum / tileSize);
    const int32 tilesX = tilesMax.X - tilesMin.X;
    const int32 tilesY = tilesMax.Z - tilesMin.Z;

    {
        PROFILE_CPU_NAMED("Prepare");

        // Prepare scene data and navmesh
        rebuild |= Math::NotNearEqual(navMesh->Data.TileSize, tileSize);
        if (rebuild)
        {
            // Remove all tiles from navmesh runtime
            runtime->RemoveTiles(navMesh);
            runtime->SetTileSize(tileSize);
            runtime->EnsureCapacity(tilesX * tilesY);

            // Remove all tiles from navmesh data
            navMesh->Data.TileSize = tileSize;
            navMesh->Data.Tiles.Clear();
            navMesh->Data.Tiles.EnsureCapacity(tilesX * tilesX);
            navMesh->IsDataDirty = true;
        }
        else
        {
            // Ensure to have enough memory for tiles
            runtime->SetTileSize(tileSize);
            runtime->EnsureCapacity(tilesX * tilesY);
        }
    }

    // Initialize nav mesh configuration
    rcConfig config;
    InitConfig(config, navMesh);

    // Generate all tiles that intersect with the navigation volume bounds
    {
        PROFILE_CPU_NAMED("StartBuildingTiles");

        for (int32 y = tilesMin.Z; y < tilesMax.Z; y++)
        {
            for (int32 x = tilesMin.X; x < tilesMax.X; x++)
            {
                BoundingBox tileBoundsNavMesh;
                if (GetNavMeshTileBounds(scene, navMesh, x, y, tileSize, tileBoundsNavMesh, worldToNavMesh))
                {
                    BuildTileAsync(navMesh, x, y, config, tileBoundsNavMesh, worldToNavMesh, tileSize);
                }
                else
                {
                    RemoveTile(navMesh, runtime, x, y, 0);
                }
            }
        }
    }
}

void BuildDirtyBounds(Scene* scene, const BoundingBox& dirtyBounds, bool rebuild)
{
    auto settings = NavigationSettings::Get();

    // Validate nav areas ids to be unique and in valid range
    for (int32 i = 0; i < settings->NavAreas.Count(); i++)
    {
        auto& a = settings->NavAreas[i];
        if (a.Id > RC_WALKABLE_AREA)
        {
            LOG(Error, "Nav Area {0} uses invalid Id. Valid values are in range 0-63 only.", a.Name);
            return;
        }

        for (int32 j = i + 1; j < settings->NavAreas.Count(); j++)
        {
            auto& b = settings->NavAreas[j];
            if (a.Id == b.Id)
            {
                LOG(Error, "Nav Area {0} uses the same Id={1} as Nav Area {2}. Each area hast to have unique Id.", a.Name, a.Id, b.Name);
                return;
            }
        }
    }

    // Sync navmeshes
    for (auto& navMeshProperties : settings->NavMeshes)
    {
        NavMesh* navMesh = nullptr;
        for (auto e : scene->Navigation.Meshes)
        {
            if (e->Properties.Name == navMeshProperties.Name)
            {
                navMesh = e;
                break;
            }
        }
        if (navMesh)
        {
            // Sync settings
            auto runtime = navMesh->GetRuntime(false);
            navMesh->Properties = navMeshProperties;
            if (runtime)
                runtime->Properties = navMeshProperties;
        }
        else if (settings->AutoAddMissingNavMeshes)
        {
            // Spawn missing navmesh
            navMesh = New<NavMesh>();
            navMesh->SetStaticFlags(StaticFlags::FullyStatic);
            navMesh->SetName(TEXT("NavMesh.") + navMeshProperties.Name);
            navMesh->Properties = navMeshProperties;
            navMesh->SetParent(scene, false);
        }
    }

    // Build all navmeshes on the scene
    for (NavMesh* navMesh : scene->Navigation.Meshes)
    {
        BuildDirtyBounds(scene, navMesh, dirtyBounds, rebuild);
    }

    // Remove unused navmeshes
    if (settings->AutoRemoveMissingNavMeshes)
    {
        for (NavMesh* navMesh : scene->Navigation.Meshes)
        {
            // Skip used navmeshes
            if (navMesh->Data.Tiles.HasItems())
                continue;

            // Skip navmeshes during async building
            int32 usageCount = 0;
            NavBuildTasksLocker.Lock();
            for (int32 i = 0; i < NavBuildTasks.Count(); i++)
            {
                if (NavBuildTasks[i]->NavMesh == navMesh)
                    usageCount++;
            }
            NavBuildTasksLocker.Unlock();
            if (usageCount != 0)
                continue;

            navMesh->DeleteObject();
        }
    }
}

void BuildWholeScene(Scene* scene)
{
    // Compute total navigation area bounds
    const BoundingBox worldBounds = scene->Navigation.GetNavigationBounds();

    BuildDirtyBounds(scene, worldBounds, true);
}

void ClearNavigation(Scene* scene)
{
    const bool autoRemoveMissingNavMeshes = NavigationSettings::Get()->AutoRemoveMissingNavMeshes;
    for (NavMesh* navMesh : scene->Navigation.Meshes)
    {
        navMesh->ClearData();
        if (autoRemoveMissingNavMeshes)
            navMesh->DeleteObject();
    }
}

void NavMeshBuilder::Update()
{
    ScopeLock lock(NavBuildQueueLocker);

    // Process nav mesh building requests and kick the tasks
    const auto now = DateTime::NowUTC();
    for (int32 i = 0; NavBuildQueue.HasItems() && i < NavBuildQueue.Count(); i++)
    {
        auto req = NavBuildQueue[i];
        if (now - req.Time >= 0)
        {
            NavBuildQueue.RemoveAt(i--);
            const auto scene = req.Scene.Get();
            if (!scene)
                continue;

            // Early out if scene has no bounds volumes to define nav mesh area
            if (scene->Navigation.Volumes.IsEmpty())
            {
                ClearNavigation(scene);
                continue;
            }

            // Check if build a custom dirty bounds or whole scene
            if (req.DirtyBounds == BoundingBox::Empty)
            {
                BuildWholeScene(scene);
            }
            else
            {
                BuildDirtyBounds(scene, req.DirtyBounds, false);
            }
        }
    }
}

void NavMeshBuilder::Build(Scene* scene, float timeoutMs)
{
    if (!scene)
    {
        LOG(Warning, "Could not generate navmesh without scene.");
        return;
    }

    // Early out if scene is not using navigation
    if (scene->Navigation.Volumes.IsEmpty())
    {
        ClearNavigation(scene);
        return;
    }

    PROFILE_CPU_NAMED("NavMeshBuilder");

    ScopeLock lock(NavBuildQueueLocker);

    BuildRequest req;
    req.Scene = scene;
    req.Time = DateTime::NowUTC() + TimeSpan::FromMilliseconds(timeoutMs);
    req.DirtyBounds = BoundingBox::Empty;

    for (int32 i = 0; i < NavBuildQueue.Count(); i++)
    {
        auto& e = NavBuildQueue[i];
        if (e.Scene == scene && e.DirtyBounds == req.DirtyBounds)
        {
            e = req;
            return;
        }
    }

    NavBuildQueue.Add(req);
}

void NavMeshBuilder::Build(Scene* scene, const BoundingBox& dirtyBounds, float timeoutMs)
{
    if (!scene)
    {
        LOG(Warning, "Could not generate navmesh without scene.");
        return;
    }

    // Early out if scene is not using navigation
    if (scene->Navigation.Volumes.IsEmpty())
    {
        ClearNavigation(scene);
        return;
    }

    PROFILE_CPU_NAMED("NavMeshBuilder");

    ScopeLock lock(NavBuildQueueLocker);

    BuildRequest req;
    req.Scene = scene;
    req.Time = DateTime::NowUTC() + TimeSpan::FromMilliseconds(timeoutMs);
    req.DirtyBounds = dirtyBounds;

    NavBuildQueue.Add(req);
}

#endif
