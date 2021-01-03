// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_NAV_MESH_BUILDER

#include "NavMeshBuilder.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/VectorInt.h"
#include "Engine/Physics/Colliders/BoxCollider.h"
#include "Engine/Physics/Colliders/MeshCollider.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Terrain/TerrainPatch.h"
#include "Engine/Terrain/Terrain.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Core/Log.h"
#include "NavigationScene.h"
#include "NavigationSettings.h"
#include "NavMeshBoundsVolume.h"
#include "NavMesh.h"
#include "NavLink.h"
#include "Navigation.h"
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

struct OffMeshLink
{
    Vector3 Start;
    Vector3 End;
    float Radius;
    bool BiDir;
    int32 Id;
};

struct NavigationSceneRasterization
{
    BoundingBox TileBounds;
    rcContext* Context;
    rcConfig* Config;
    rcHeightfield* Heightfield;
    float WalkableThreshold;
    Array<Vector3> VertexBuffer;
    Array<int32> IndexBuffer;
    Array<OffMeshLink>* OffMeshLinks;

    NavigationSceneRasterization(const BoundingBox& tileBounds, rcContext* context, rcConfig* config, rcHeightfield* heightfield, Array<OffMeshLink>* offMeshLinks)
        : TileBounds(tileBounds)
    {
        Context = context;
        Config = config;
        Heightfield = heightfield;
        WalkableThreshold = Math::Cos(config->walkableSlopeAngle * DegreesToRadians);
        OffMeshLinks = offMeshLinks;
    }

    void RasterizeTriangles()
    {
        auto& vb = VertexBuffer;
        auto& ib = IndexBuffer;
        if (vb.IsEmpty() || ib.IsEmpty())
            return;

        // Rasterize triangles
        for (int32 i0 = 0; i0 < ib.Count();)
        {
            auto v0 = vb[ib[i0++]];
            auto v1 = vb[ib[i0++]];
            auto v2 = vb[ib[i0++]];

            auto n = Vector3::Cross(v0 - v1, v0 - v2);
            n.Normalize();
            const char area = n.Y > WalkableThreshold ? RC_WALKABLE_AREA : 0;
            rcRasterizeTriangle(Context, &v0.X, &v1.X, &v2.X, area, *Heightfield);
        }
    }

    static bool Walk(Actor* actor, NavigationSceneRasterization& e)
    {
        // Early out if object is not intersecting with the tile bounds or is not using navigation
        if (!actor->GetIsActive() || !(actor->GetStaticFlags() & StaticFlags::Navigation) || !actor->GetBox().Intersects(e.TileBounds))
            return true;

        // Prepare buffers (for triangles)
        auto& vb = e.VertexBuffer;
        auto& ib = e.IndexBuffer;
        vb.Clear();
        ib.Clear();

        // Extract data from the actor
        if (const auto* boxCollider = dynamic_cast<BoxCollider*>(actor))
        {
            PROFILE_CPU_NAMED("BoxCollider");

            OrientedBoundingBox box = boxCollider->GetOrientedBox();

            vb.Resize(8);
            box.GetCorners(vb.Get());

            ib.Add(BoxTrianglesIndicesCache, 36);

            e.RasterizeTriangles();
        }
        else if (const auto* meshCollider = dynamic_cast<MeshCollider*>(actor))
        {
            PROFILE_CPU_NAMED("MeshCollider");

            auto collisionData = meshCollider->CollisionData.Get();
            if (!collisionData || collisionData->WaitForLoaded(1000.0f))
                return true;

            collisionData->ExtractGeometry(vb, ib);

            e.RasterizeTriangles();
        }
        else if (const auto* terrain = dynamic_cast<Terrain*>(actor))
        {
            PROFILE_CPU_NAMED("Terrain");

            for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
            {
                const auto patch = terrain->GetPatch(patchIndex);
                if (!patch->GetBounds().Intersects(e.TileBounds))
                    continue;

                patch->ExtractCollisionGeometry(vb, ib);

                e.RasterizeTriangles();
            }
        }
        else if (const auto* navLink = dynamic_cast<NavLink*>(actor))
        {
            PROFILE_CPU_NAMED("NavLink");

            OffMeshLink link;
            link.Start = navLink->GetTransform().LocalToWorld(navLink->Start);
            link.End = navLink->GetTransform().LocalToWorld(navLink->End);
            link.Radius = navLink->Radius;
            link.BiDir = navLink->BiDirectional;
            link.Id = GetHash(navLink->GetID());

            e.OffMeshLinks->Add(link);
        }

        // TODO: nav mesh for capsule collider
        // TODO: nav mesh for sphere collider

        return true;
    }
};

void RasterizeGeometry(const BoundingBox& tileBounds, rcContext* context, rcConfig* config, rcHeightfield* heightfield, Array<OffMeshLink>* offMeshLinks)
{
    PROFILE_CPU_NAMED("RasterizeGeometry");

    NavigationSceneRasterization rasterization(tileBounds, context, config, heightfield, offMeshLinks);
    Function<bool(Actor*, NavigationSceneRasterization&)> treeWalkFunction(NavigationSceneRasterization::Walk);
    SceneQuery::TreeExecute<NavigationSceneRasterization&>(treeWalkFunction, rasterization);
}

// Builds navmesh tile bounds and check if there are any valid navmesh volumes at that tile location
// Returns true if tile is intersecting with any navmesh bounds volume actor - which means tile is in use
bool GetNavMeshTileBounds(NavigationScene* scene, int32 x, int32 y, float tileSize, BoundingBox& tileBounds)
{
    // Build initial tile bounds (with infinite extent)
    tileBounds.Minimum.X = x * tileSize;
    tileBounds.Minimum.Y = -NAV_MESH_TILE_MAX_EXTENT;
    tileBounds.Minimum.Z = y * tileSize;
    tileBounds.Maximum.X = tileBounds.Minimum.X + tileSize;
    tileBounds.Maximum.Y = NAV_MESH_TILE_MAX_EXTENT;
    tileBounds.Maximum.Z = tileBounds.Minimum.Z + tileSize;

    // Check if any navmesh volume intersects with the tile
    bool foundAnyVolume = false;
    Vector2 rangeY;
    for (int32 i = 0; i < scene->Volumes.Count(); i++)
    {
        const auto volume = scene->Volumes[i];
        const auto& volumeBounds = volume->GetBox();
        if (volumeBounds.Intersects(tileBounds))
        {
            if (foundAnyVolume)
            {
                rangeY.X = Math::Min(rangeY.X, volumeBounds.Minimum.Y);
                rangeY.Y = Math::Max(rangeY.Y, volumeBounds.Maximum.Y);
            }
            else
            {
                rangeY.X = volumeBounds.Minimum.Y;
                rangeY.Y = volumeBounds.Maximum.Y;
            }
            foundAnyVolume = true;
        }
    }

    if (foundAnyVolume)
    {
        // Build proper tile bounds
        tileBounds.Minimum.Y = rangeY.X;
        tileBounds.Maximum.Y = rangeY.Y;
    }

    return foundAnyVolume;
}

void RemoveTile(NavMesh* navMesh, NavigationScene* scene, int32 x, int32 y, int32 layer)
{
    ScopeLock lock(navMesh->Locker);

    // Find tile data and remove it
    for (int32 i = 0; i < scene->Data.Tiles.Count(); i++)
    {
        auto& tile = scene->Data.Tiles[i];
        if (tile.PosX == x && tile.PosY == y && tile.Layer == layer)
        {
            scene->Data.Tiles.RemoveAt(i);
            scene->IsDataDirty = true;
            break;
        }
    }

    // Remove tile from navmesh
    navMesh->RemoveTile(x, y, layer);
}

bool GenerateTile(NavigationScene* scene, int32 x, int32 y, BoundingBox& tileBounds, float tileSize, rcConfig& config)
{
    rcContext context;
    int32 layer = 0;

    // Expand tile bounds by a certain margin
    const float tileBorderSize = (1 + config.borderSize) * config.cs;
    tileBounds.Minimum -= tileBorderSize;
    tileBounds.Maximum += tileBorderSize;

    rcVcopy(config.bmin, &tileBounds.Minimum.X);
    rcVcopy(config.bmax, &tileBounds.Maximum.X);

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
    RasterizeGeometry(tileBounds, &context, &config, heightfield, &offMeshLinks);

    rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *heightfield);
    rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *heightfield);
    rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *heightfield);

    rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
    if (!compactHeightfield)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory compact heightfield.");
        return true;
    }
    if (!rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *heightfield, *compactHeightfield))
    {
        LOG(Warning, "Could not generate navmesh: Could not build compact data.");
        return true;
    }

    rcFreeHeightField(heightfield);

    if (!rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield))
    {
        LOG(Warning, "Could not generate navmesh: Could not erode.");
        return true;
    }

    if (!rcBuildDistanceField(&context, *compactHeightfield))
    {
        LOG(Warning, "Could not generate navmesh: Could not build distance field.");
        return true;
    }

    if (!rcBuildRegions(&context, *compactHeightfield, config.borderSize, config.minRegionArea, config.mergeRegionArea))
    {
        LOG(Warning, "Could not generate navmesh: Could not build regions.");
        return true;
    }

    rcContourSet* contourSet = rcAllocContourSet();
    if (!contourSet)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for contour set.");
        return true;
    }
    if (!rcBuildContours(&context, *compactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *contourSet))
    {
        LOG(Warning, "Could not generate navmesh: Could not create contours.");
        return true;
    }

    rcPolyMesh* polyMesh = rcAllocPolyMesh();
    if (!polyMesh)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for poly mesh.");
        return true;
    }
    if (!rcBuildPolyMesh(&context, *contourSet, config.maxVertsPerPoly, *polyMesh))
    {
        LOG(Warning, "Could not generate navmesh: Could not triangulate contours.");
        return true;
    }

    rcPolyMeshDetail* detailMesh = rcAllocPolyMeshDetail();
    if (!detailMesh)
    {
        LOG(Warning, "Could not generate navmesh: Out of memory for detail mesh.");
        return true;
    }
    if (!rcBuildPolyMeshDetail(&context, *polyMesh, *compactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *detailMesh))
    {
        LOG(Warning, "Could not generate navmesh: Could not build detail mesh.");
        return true;
    }

    rcFreeCompactHeightfield(compactHeightfield);
    rcFreeContourSet(contourSet);

    for (int i = 0; i < polyMesh->npolys; ++i)
    {
        polyMesh->flags[i] = polyMesh->areas[i] == RC_WALKABLE_AREA ? 1 : 0;
    }

    if (polyMesh->nverts == 0)
    {
        // Empty tile
        RemoveTile(Navigation::GetNavMesh(), scene, x, y, layer);
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
    params.walkableHeight = config.walkableHeight * config.ch;
    params.walkableRadius = config.walkableRadius * config.cs;
    params.walkableClimb = config.walkableClimb * config.ch;
    params.tileX = x;
    params.tileY = y;
    params.tileLayer = layer;
    rcVcopy(params.bmin, polyMesh->bmin);
    rcVcopy(params.bmax, polyMesh->bmax);
    params.cs = config.cs;
    params.ch = config.ch;
    params.buildBvTree = false;

    // Prepare navmesh links
    Array<Vector3> offMeshStartEnd;
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

            // TODO: support navigation areas, navigation area type for off mesh links
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
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        LOG(Warning, "Could not build Detour navmesh.");
        return true;
    }

    {
        PROFILE_CPU_NAMED("Navigation.CreateTile");

        ScopeLock lock(Navigation::GetNavMesh()->Locker);

        // Add tile data
        scene->IsDataDirty = true;
        auto& tile = scene->Data.Tiles.AddOne();
        tile.PosX = x;
        tile.PosY = y;
        tile.Layer = layer;

        // Copy data
        tile.Data.Copy(navData, navDataSize);

        // Add tile to navmesh
        Navigation::GetNavMesh()->AddTile(scene, tile);
    }

    dtFree(navData);

    return false;
}

float GetTileSize()
{
    auto& settings = *NavigationSettings::Instance();
    return settings.CellSize * settings.TileSize;
}

void InitConfig(rcConfig& config)
{
    auto& settings = *NavigationSettings::Instance();

    config.cs = settings.CellSize;
    config.ch = settings.CellHeight;
    config.walkableSlopeAngle = settings.WalkableMaxSlopeAngle;
    config.walkableHeight = (int)(settings.WalkableHeight / config.ch + 0.99f);
    config.walkableClimb = (int)(settings.WalkableMaxClimb / config.ch);
    config.walkableRadius = (int)(settings.WalkableRadius / config.cs + 0.99f);
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

    NavigationScene* Scene;
    BoundingBox TileBounds;
    int32 X;
    int32 Y;
    float TileSize;
    rcConfig Config;

public:

    // [ThreadPoolTask]
    bool Run() override
    {
        PROFILE_CPU_NAMED("BuildNavMeshTile");

        if (GenerateTile(Scene, X, Y, TileBounds, TileSize, Config))
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
        if (task->Scene == scene->Navigation)
        {
            NavBuildTasksLocker.Unlock();

            // Cancel task but without locking queue from this thread to prevent dead-locks
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

void BuildTileAsync(NavigationScene* scene, int32 x, int32 y, rcConfig& config, const BoundingBox& tileBounds, float tileSize)
{
    NavBuildTasksLocker.Lock();

    // Skip if this tile is already during cooking
    for (int32 i = 0; i < NavBuildTasks.Count(); i++)
    {
        const auto task = NavBuildTasks[i];
        if (task->X == x && task->Y == y)
        {
            NavBuildTasksLocker.Unlock();
            return;
        }
    }

    // Create task
    auto task = New<NavMeshTileBuildTask>();
    task->Scene = scene;
    task->X = x;
    task->Y = y;
    task->TileBounds = tileBounds;
    task->TileSize = tileSize;
    task->Config = config;
    NavBuildTasks.Add(task);
    NavBuildTasksMaxCount++;

    NavBuildTasksLocker.Unlock();

    // Invoke job
    task->Start();
}

void BuildWholeScene(NavigationScene* scene)
{
    const float tileSize = GetTileSize();
    const auto navMesh = Navigation::GetNavMesh();

    // Compute total navigation area bounds
    const BoundingBox worldBounds = scene->GetNavigationBounds();

    // Align total bounds to tile size
    BoundingBox worldBoundsAligned;
    worldBoundsAligned.Minimum = Vector3::Floor(worldBounds.Minimum / tileSize) * tileSize;
    worldBoundsAligned.Maximum = Vector3::Ceil(worldBounds.Maximum / tileSize) * tileSize;

    // Calculate tiles range for the given navigation world bounds (aligned to tiles size)
    const Int3 tilesMin = Int3(worldBoundsAligned.Minimum / tileSize);
    const Int3 tilesMax = Int3(worldBoundsAligned.Maximum / tileSize);
    const int32 tilesX = tilesMax.X - tilesMin.X;
    const int32 tilesY = tilesMax.Z - tilesMin.Z;

    {
        PROFILE_CPU_NAMED("Prepare");

        // Prepare navmesh
        navMesh->RemoveTiles(scene);
        navMesh->SetTileSize(tileSize);
        navMesh->EnsureCapacity(tilesX * tilesY);

        // Prepare scene data
        scene->Data.TileSize = tileSize;
        scene->Data.Tiles.Clear();
        scene->Data.Tiles.EnsureCapacity(tilesX * tilesX);
        scene->IsDataDirty = true;
    }

    // Initialize nav mesh configuration
    rcConfig config;
    InitConfig(config);

    // Generate all tiles that intersect with the navigation volume bounds
    {
        PROFILE_CPU_NAMED("StartBuildingTiles");

        for (int32 y = tilesMin.Z; y < tilesMax.Z; y ++)
        {
            for (int32 x = tilesMin.X; x < tilesMax.X; x ++)
            {
                BoundingBox tileBounds;
                if (GetNavMeshTileBounds(scene, x, y, tileSize, tileBounds))
                {
                    BuildTileAsync(scene, x, y, config, tileBounds, tileSize);
                }
                else
                {
                    RemoveTile(navMesh, scene, x, y, 0);
                }
            }
        }
    }
}

void BuildDirtyBounds(NavigationScene* scene, const BoundingBox& dirtyBounds)
{
    const float tileSize = GetTileSize();
    const auto navMesh = Navigation::GetNavMesh();

    // Align dirty bounds to tile size
    BoundingBox dirtyBoundsAligned;
    dirtyBoundsAligned.Minimum = Vector3::Floor(dirtyBounds.Minimum / tileSize) * tileSize;
    dirtyBoundsAligned.Maximum = Vector3::Ceil(dirtyBounds.Maximum / tileSize) * tileSize;

    // Calculate tiles range for the given navigation dirty bounds (aligned to tiles size)
    const Int3 tilesMin = Int3(dirtyBoundsAligned.Minimum / tileSize);
    const Int3 tilesMax = Int3(dirtyBoundsAligned.Maximum / tileSize);
    const int32 tilesX = tilesMax.X - tilesMin.X;
    const int32 tilesY = tilesMax.Z - tilesMin.Z;

    {
        PROFILE_CPU_NAMED("Prepare");

        // Prepare scene data and navmesh
        if (Math::NotNearEqual(scene->Data.TileSize, tileSize))
        {
            navMesh->RemoveTiles(scene);
            navMesh->SetTileSize(tileSize);
            navMesh->EnsureCapacity(tilesX * tilesY);

            scene->Data.TileSize = tileSize;
            scene->Data.Tiles.Clear();
            scene->Data.Tiles.EnsureCapacity(tilesX * tilesX);
            scene->IsDataDirty = true;
        }
        else
        {
            // Prepare navmesh
            navMesh->SetTileSize(tileSize);
            navMesh->EnsureCapacity(tilesX * tilesY);
        }
    }

    // Initialize nav mesh configuration
    rcConfig config;
    InitConfig(config);

    // Generate all tiles that intersect with the navigation volume bounds
    {
        PROFILE_CPU_NAMED("StartBuildingTiles");

        for (int32 y = tilesMin.Z; y < tilesMax.Z; y ++)
        {
            for (int32 x = tilesMin.X; x < tilesMax.X; x ++)
            {
                BoundingBox tileBounds;
                if (GetNavMeshTileBounds(scene, x, y, tileSize, tileBounds))
                {
                    BuildTileAsync(scene, x, y, config, tileBounds, tileSize);
                }
                else
                {
                    RemoveTile(navMesh, scene, x, y, 0);
                }
            }
        }
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
            auto scene = req.Scene->Navigation;

            // Early out if scene has no bounds volumes to define nav mesh area
            if (scene->Volumes.IsEmpty())
            {
                // Cleanup if no navigation to use
                scene->Data.TileSize = 0;
                scene->Data.Tiles.Resize(0);
                scene->IsDataDirty = true;
                continue;
            }

            // Check if build a custom dirty bounds or whole scene
            if (req.DirtyBounds == BoundingBox::Empty)
            {
                BuildWholeScene(scene);
            }
            else
            {
                BuildDirtyBounds(scene, req.DirtyBounds);
            }
        }
    }
}

void NavMeshBuilder::Build(Scene* scene, float timeoutMs)
{
    // Early out if scene is not using navigation
    if (scene->Navigation->Volumes.IsEmpty())
    {
        scene->Navigation->ClearData();
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
    // Early out if scene is not using navigation
    if (scene->Navigation->Volumes.IsEmpty())
    {
        scene->Navigation->ClearData();
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
