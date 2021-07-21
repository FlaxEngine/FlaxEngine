// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Foliage.h"
#include "FoliageType.h"
#include "FoliageCluster.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Random.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/RenderTask.h"
#if !FOLIAGE_USE_SINGLE_QUAD_TREE && FOLIAGE_USE_DRAW_CALLS_BATCHING
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Renderer/RenderList.h"
#endif
#include "Engine/Level/SceneQuery.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Utilities/Encryption.h"

Foliage::Foliage(const SpawnParams& params)
    : Actor(params)
{
    _disableFoliageTypeEvents = false;
}

void Foliage::AddToCluster(ChunkedArray<FoliageCluster, FOLIAGE_CLUSTER_CHUNKS_SIZE>& clusters, FoliageCluster* cluster, FoliageInstance& instance)
{
    ASSERT(instance.Bounds.Radius > ZeroTolerance);
    ASSERT(cluster->Bounds.Intersects(instance.Bounds));

    // Find target cluster
    while (cluster->Children[0])
    {
#define CHECK_CHILD(idx) \
			if (cluster->Children[idx]->Bounds.Intersects(instance.Bounds)) \
			{ \
				cluster = cluster->Children[idx]; \
				continue; \
			}
        CHECK_CHILD(0);
        CHECK_CHILD(1);
        CHECK_CHILD(2);
        CHECK_CHILD(3);
#undef CHECK_CHILD
    }

    // Check if it's not full
    if (cluster->Instances.Count() != FOLIAGE_CLUSTER_CAPACITY)
    {
        // Insert into cluster
        cluster->Instances.Add(&instance);
    }
    else
    {
        // Subdivide cluster
        const int32 count = clusters.Count();
        clusters.Resize(count + 4);
        cluster->Children[0] = &clusters[count + 0];
        cluster->Children[1] = &clusters[count + 1];
        cluster->Children[2] = &clusters[count + 2];
        cluster->Children[3] = &clusters[count + 3];

        // Setup children
        const Vector3 min = cluster->Bounds.Minimum;
        const Vector3 max = cluster->Bounds.Maximum;
        const Vector3 size = cluster->Bounds.GetSize();
        cluster->Children[0]->Init(BoundingBox(min, min + size * Vector3(0.5f, 1.0f, 0.5f)));
        cluster->Children[1]->Init(BoundingBox(min + size * Vector3(0.5f, 0.0f, 0.5f), max));
        cluster->Children[2]->Init(BoundingBox(min + size * Vector3(0.5f, 0.0f, 0.0f), min + size * Vector3(1.0f, 1.0f, 0.5f)));
        cluster->Children[3]->Init(BoundingBox(min + size * Vector3(0.0f, 0.0f, 0.5f), min + size * Vector3(0.5f, 1.0f, 1.0f)));

        // Move instances to a proper cells
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            AddToCluster(clusters, cluster, *cluster->Instances[i]);
        }
        cluster->Instances.Clear();
        AddToCluster(clusters, cluster, instance);
    }
}

#if !FOLIAGE_USE_SINGLE_QUAD_TREE && FOLIAGE_USE_DRAW_CALLS_BATCHING

void Foliage::DrawInstance(RenderContext& renderContext, FoliageInstance& instance, FoliageType& type, Model* model, int32 lod, float lodDitherFactor, DrawCallsList* drawCallsLists, BatchedDrawCalls& result) const
{
    const auto& meshes = model->LODs[lod].Meshes;
    for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
    {
        auto& drawCall = drawCallsLists[lod][meshIndex];
        if (!drawCall.DrawCall.Material)
            continue;

        DrawKey key;
        key.Mat = drawCall.DrawCall.Material;
        key.Geo = &meshes[meshIndex];
        key.Lightmap = instance.Lightmap.TextureIndex;
        auto* e = result.TryGet(key);
        if (!e)
        {
            e = &result[key];
            e->DrawCall.Material = key.Mat;
            e->DrawCall.Surface.Lightmap = _staticFlags & StaticFlags::Lightmap ? _scene->LightmapsData.GetReadyLightmap(key.Lightmap) : nullptr;
        }

        // Add instance to the draw batch
        auto& instanceData = e->Instances.AddOne();
        instanceData.InstanceOrigin = Vector3(instance.World.M41, instance.World.M42, instance.World.M43);
        instanceData.PerInstanceRandom = instance.Random;
        instanceData.InstanceTransform1 = Vector3(instance.World.M11, instance.World.M12, instance.World.M13);
        instanceData.LODDitherFactor = lodDitherFactor;
        instanceData.InstanceTransform2 = Vector3(instance.World.M21, instance.World.M22, instance.World.M23);
        instanceData.InstanceTransform3 = Vector3(instance.World.M31, instance.World.M32, instance.World.M33);
        instanceData.InstanceLightmapArea = Half4(instance.Lightmap.UVsArea);
    }
}

void Foliage::DrawCluster(RenderContext& renderContext, FoliageCluster* cluster, FoliageType& type, DrawCallsList* drawCallsLists, BatchedDrawCalls& result) const
{
    // Skip clusters that around too far from view
    if (Vector3::Distance(renderContext.View.Position, cluster->TotalBoundsSphere.Center) - cluster->TotalBoundsSphere.Radius > cluster->MaxCullDistance)
        return;

    //DebugDraw::DrawBox(cluster->Bounds, Color::Red);

    // Draw visible children
    if (cluster->Children[0])
    {
        // Don't store instances in non-leaf nodes
        ASSERT_LOW_LAYER(cluster->Instances.IsEmpty());

#define DRAW_CLUSTER(idx) \
		if (renderContext.View.CullingFrustum.Intersects(cluster->Children[idx]->TotalBounds)) \
			DrawCluster(renderContext, cluster->Children[idx], type, drawCallsLists, result)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    else
    {
        // Draw visible instances
        const auto frame = Engine::FrameCount;
        const auto model = type.Model.Get();
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances[i];
            if (Vector3::Distance(renderContext.View.Position, instance.Bounds.Center) - instance.Bounds.Radius < instance.CullDistance &&
                renderContext.View.CullingFrustum.Intersects(instance.Bounds))
            {
                const auto modelFrame = instance.DrawState.PrevFrame + 1;

                // Select a proper LOD index (model may be culled)
                int32 lodIndex = RenderTools::ComputeModelLOD(model, instance.Bounds.Center, instance.Bounds.Radius, renderContext);
                if (lodIndex == -1)
                {
                    // Handling model fade-out transition
                    if (modelFrame == frame && instance.DrawState.PrevLOD != -1)
                    {
                        // Check if start transition
                        if (instance.DrawState.LODTransition == 255)
                        {
                            instance.DrawState.LODTransition = 0;
                        }

                        RenderTools::UpdateModelLODTransition(instance.DrawState.LODTransition);

                        // Check if end transition
                        if (instance.DrawState.LODTransition == 255)
                        {
                            instance.DrawState.PrevLOD = lodIndex;
                        }
                        else
                        {
                            const auto prevLOD = model->ClampLODIndex(instance.DrawState.PrevLOD);
                            const float normalizedProgress = static_cast<float>(instance.DrawState.LODTransition) * (1.0f / 255.0f);
                            DrawInstance(renderContext, instance, type, model, prevLOD, normalizedProgress, drawCallsLists, result);
                        }
                    }
                    instance.DrawState.PrevFrame = frame;
                    continue;
                }
                lodIndex += renderContext.View.ModelLODBias;
                lodIndex = model->ClampLODIndex(lodIndex);

                // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
                if (modelFrame == frame)
                {
                    // Check if start transition
                    if (instance.DrawState.PrevLOD != lodIndex && instance.DrawState.LODTransition == 255)
                    {
                        instance.DrawState.LODTransition = 0;
                    }

                    RenderTools::UpdateModelLODTransition(instance.DrawState.LODTransition);

                    // Check if end transition
                    if (instance.DrawState.LODTransition == 255)
                    {
                        instance.DrawState.PrevLOD = lodIndex;
                    }
                }
                    // Check if there was a gap between frames in drawing this model instance
                else if (modelFrame < frame || instance.DrawState.PrevLOD == -1)
                {
                    // Reset state
                    instance.DrawState.PrevLOD = lodIndex;
                    instance.DrawState.LODTransition = 255;
                }

                // Draw
                if (instance.DrawState.PrevLOD == lodIndex)
                {
                    DrawInstance(renderContext, instance, type, model, lodIndex, 0.0f, drawCallsLists, result);
                }
                else if (instance.DrawState.PrevLOD == -1)
                {
                    const float normalizedProgress = static_cast<float>(instance.DrawState.LODTransition) * (1.0f / 255.0f);
                    DrawInstance(renderContext, instance, type, model, lodIndex, 1.0f - normalizedProgress, drawCallsLists, result);
                }
                else
                {
                    const auto prevLOD = model->ClampLODIndex(instance.DrawState.PrevLOD);
                    const float normalizedProgress = static_cast<float>(instance.DrawState.LODTransition) * (1.0f / 255.0f);
                    DrawInstance(renderContext, instance, type, model, prevLOD, normalizedProgress, drawCallsLists, result);
                    DrawInstance(renderContext, instance, type, model, lodIndex, normalizedProgress - 1.0f, drawCallsLists, result);
                }

                //DebugDraw::DrawSphere(instance.Bounds, Color::YellowGreen);

                instance.DrawState.PrevFrame = frame;
            }
        }
    }
}

#else

void Foliage::DrawCluster(RenderContext& renderContext, FoliageCluster* cluster, Mesh::DrawInfo& draw)
{
    // Skip clusters that around too far from view
    if (Vector3::Distance(renderContext.View.Position, cluster->TotalBoundsSphere.Center) - cluster->TotalBoundsSphere.Radius > cluster->MaxCullDistance)
        return;

    //DebugDraw::DrawBox(cluster->Bounds, Color::Red);

    // Draw visible children
    if (cluster->Children[0])
    {
        // Don't store instances in non-leaf nodes
        ASSERT_LOW_LAYER(cluster->Instances.IsEmpty());

#define DRAW_CLUSTER(idx) \
		if (renderContext.View.CullingFrustum.Intersects(cluster->Children[idx]->TotalBounds)) \
			DrawCluster(renderContext, cluster->Children[idx], draw)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    else
    {
        // Draw visible instances
        const auto frame = Engine::FrameCount;
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances[i];
            auto& type = FoliageTypes[instance.Type];

            // Check if can draw this instance
            if (type._canDraw &&
                Vector3::Distance(renderContext.View.Position, instance.Bounds.Center) - instance.Bounds.Radius < instance.CullDistance &&
                renderContext.View.CullingFrustum.Intersects(instance.Bounds))
            {
                // Disable motion blur
                instance.DrawState.PrevWorld = instance.World;

                // Draw model
                draw.Lightmap = _scene->LightmapsData.GetReadyLightmap(instance.Lightmap.TextureIndex);
                draw.LightmapUVs = &instance.Lightmap.UVsArea;
                draw.Buffer = &type.Entries;
                draw.World = &instance.World;
                draw.DrawState = &instance.DrawState;
                draw.Bounds = instance.Bounds;
                draw.PerInstanceRandom = instance.Random;
                draw.DrawModes = type._drawModes;
                type.Model->Draw(renderContext, draw);

                //DebugDraw::DrawSphere(instance.Bounds, Color::YellowGreen);

                instance.DrawState.PrevFrame = frame;
            }
        }
    }
}

#endif

int32 Foliage::GetInstancesCount() const
{
    return Instances.Count();
}

FoliageInstance Foliage::GetInstance(int32 index) const
{
    return Instances[index];
}

int32 Foliage::GetFoliageTypesCount() const
{
    return FoliageTypes.Count();
}

FoliageType* Foliage::GetFoliageType(int32 index)
{
    CHECK_RETURN(index >= 0 && index < FoliageTypes.Count(), nullptr)
    return &FoliageTypes[index];
}

void Foliage::AddFoliageType(Model* model)
{
    PROFILE_CPU();

    // Ensure to have unique model
    CHECK(model);
    for (int32 i = 0; i < FoliageTypes.Count(); i++)
    {
        if (FoliageTypes[i].Model == model)
        {
            LOG(Error, "The given model is already used by other foliage type.");
            return;
        }
    }

    // Add
    _disableFoliageTypeEvents = true;
    auto& item = FoliageTypes.AddOne();
    _disableFoliageTypeEvents = false;

    // Setup
    item.Foliage = this;
    item.Index = FoliageTypes.Count() - 1;
    item.Model = model;
}

void Foliage::RemoveFoliageType(int32 index)
{
    PROFILE_CPU();

    // Remove instances using this foliage type
    if (FoliageTypes.Count() != 1)
    {
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Type == index)
            {
                Instances.Remove(i);
                --i;
            }
        }

        // Update all instances using foliage types with higher index to point into a valid type
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Type > index)
                i->Type--;
        }
    }
    else
    {
        Instances.Clear();
    }

    // Remove foliage instance type
    for (int32 i = index + 1; i < FoliageTypes.Count(); i++)
    {
        FoliageTypes[i].Index--;
    }
    auto& item = FoliageTypes[index];
    item.Model = nullptr;
    item.Entries.Release();
    FoliageTypes.RemoveAtKeepOrder(index);

    RebuildClusters();
}

int32 Foliage::GetFoliageTypeInstancesCount(int32 index) const
{
    PROFILE_CPU();

    int32 result = 0;

    for (auto i = Instances.Begin(); i.IsNotEnd(); i++)
    {
        if (i->Type == index)
            result++;
    }

    return result;
}

void Foliage::AddInstance(const FoliageInstance& instance)
{
    ASSERT(instance.Type >= 0 && instance.Type < FoliageTypes.Count());
    auto type = &FoliageTypes[instance.Type];

    // Add instance
    auto data = Instances.Add(instance);
    data->Bounds = BoundingSphere::Empty;
    data->Random = Random::Rand();
    data->CullDistance = type->CullDistance + type->CullDistanceRandomRange * data->Random;

    // Calculate foliage instance geometry transformation matrix
    Matrix matrix, world;
    GetLocalToWorldMatrix(world);
    data->Transform.GetWorld(matrix);
    Matrix::Multiply(matrix, world, data->World);
    data->DrawState.PrevWorld = data->World;

    // Validate foliage type model
    if (!type->IsReady())
        return;

    // Update bounds
    Vector3 corners[8];
    auto& meshes = type->Model->LODs[0].Meshes;
    for (int32 j = 0; j < meshes.Count(); j++)
    {
        meshes[j].GetCorners(corners);

        for (int32 k = 0; k < 8; k++)
        {
            Vector3::Transform(corners[k], data->World, corners[k]);
        }
        BoundingSphere meshBounds;
        BoundingSphere::FromPoints(corners, 8, meshBounds);
        ASSERT(meshBounds.Radius > ZeroTolerance);

        BoundingSphere::Merge(data->Bounds, meshBounds, data->Bounds);
    }
    data->Bounds.Radius += ZeroTolerance;
}

void Foliage::RemoveInstance(ChunkedArray<FoliageInstance, FOLIAGE_INSTANCE_CHUNKS_SIZE>::Iterator i)
{
    Instances.Remove(i);
}

void Foliage::SetInstanceTransform(int32 index, const Transform& value)
{
    auto& instance = Instances[index];
    auto type = &FoliageTypes[instance.Type];

    // Change transform
    instance.Transform = value;

    // Update world matrix
    Matrix matrix, world;
    GetLocalToWorldMatrix(world);
    instance.Transform.GetWorld(matrix);
    Matrix::Multiply(matrix, world, instance.World);

    // Update bounds
    instance.Bounds = BoundingSphere::Empty;
    if (!type->IsReady())
        return;
    Vector3 corners[8];
    auto& meshes = type->Model->LODs[0].Meshes;
    for (int32 j = 0; j < meshes.Count(); j++)
    {
        meshes[j].GetCorners(corners);

        for (int32 k = 0; k < 8; k++)
        {
            Vector3::Transform(corners[k], instance.World, corners[k]);
        }
        BoundingSphere meshBounds;
        BoundingSphere::FromPoints(corners, 8, meshBounds);
        ASSERT(meshBounds.Radius > ZeroTolerance);

        BoundingSphere::Merge(instance.Bounds, meshBounds, instance.Bounds);
    }
    instance.Bounds.Radius += ZeroTolerance;
}

void Foliage::OnFoliageTypeModelLoaded(int32 index)
{
    if (_disableFoliageTypeEvents)
        return;
    PROFILE_CPU();
    auto& type = FoliageTypes[index];
    ASSERT(type.IsReady());

    // Update bounds for instances using this type
    bool hasAnyInstance = false;
#if !FOLIAGE_USE_SINGLE_QUAD_TREE
    BoundingBox totalBoundsType, box;
#endif
    {
        PROFILE_CPU_NAMED("Update Bounds");
        Vector3 corners[8];
        auto& meshes = type.Model->LODs[0].Meshes;
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            if (instance.Type != index)
                continue;
            instance.Bounds = BoundingSphere::Empty;

            // Include all meshes
            for (int32 j = 0; j < meshes.Count(); j++)
            {
                // TODO: cache bounds for all model meshes and reuse later
                meshes[j].GetCorners(corners);

                // TODO: use SIMD
                for (int32 k = 0; k < 8; k++)
                {
                    Vector3::Transform(corners[k], instance.World, corners[k]);
                }
                BoundingSphere meshBounds;
                BoundingSphere::FromPoints(corners, 8, meshBounds);
                ASSERT(meshBounds.Radius > ZeroTolerance);
                BoundingSphere::Merge(instance.Bounds, meshBounds, instance.Bounds);
            }

#if !FOLIAGE_USE_SINGLE_QUAD_TREE
            // TODO: use SIMD
            BoundingBox::FromSphere(instance.Bounds, box);
            if (hasAnyInstance)
                BoundingBox::Merge(totalBoundsType, box, totalBoundsType);
            else
                totalBoundsType = box;
#endif
            hasAnyInstance = true;
        }
    }
    if (!hasAnyInstance)
        return;

    // Refresh quad-tree
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    RebuildClusters();
#else
    {
        PROFILE_CPU_NAMED("Setup");

        // Setup first and topmost cluster
        type.Clusters.Resize(1);
        type.Root = &type.Clusters[0];
        type.Root->Init(totalBoundsType);

        // Update bounds of the foliage
        _box = totalBoundsType;
        for (auto& e : FoliageTypes)
        {
            if (e.Index != index && e.Root)
                BoundingBox::Merge(_box, e.Root->Bounds, _box);
        }
        BoundingSphere::FromBox(_box, _sphere);
        if (_sceneRenderingKey != -1)
            GetSceneRendering()->UpdateGeometry(this, _sceneRenderingKey);
    }
    {
        PROFILE_CPU_NAMED("Create Clusters");

        // Create clusters for foliage type quad tree
        const float globalDensityScale = GetGlobalDensityScale();
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            const float densityScale = type.UseDensityScaling ? globalDensityScale * type.DensityScalingScale : 1.0f;
            if (instance.Type == index && instance.Random < densityScale)
            {
                AddToCluster(type.Clusters, type.Root, instance);
            }
        }
    }
    {
        PROFILE_CPU_NAMED("Update Cache");
        type.Root->UpdateTotalBoundsAndCullDistance();
    }
#endif
}

void Foliage::RebuildClusters()
{
    PROFILE_CPU();

    // Faster path if foliage is empty or no types is ready
    bool anyTypeReady = false;
    for (auto& type : FoliageTypes)
        anyTypeReady |= type.IsReady();
    if (!anyTypeReady || Instances.IsEmpty())
    {
#if FOLIAGE_USE_SINGLE_QUAD_TREE
        Root = nullptr;
        Clusters.Clear();
#else
        for (auto& type : FoliageTypes)
        {
            type.Root = nullptr;
            type.Clusters.Clear();
        }
#endif
        _box = BoundingBox(_transform.Translation, _transform.Translation);
        _sphere = BoundingSphere(_transform.Translation, 0.0f);
        if (_sceneRenderingKey != -1)
            GetSceneRendering()->UpdateGeometry(this, _sceneRenderingKey);
        return;
    }

    // Clear clusters and initialize root
    {
        PROFILE_CPU_NAMED("Init Root");

        BoundingBox totalBounds, box;
#if FOLIAGE_USE_SINGLE_QUAD_TREE
        {
            // Calculate total bounds of all instances
            auto i = Instances.Begin();
            for (; i.IsNotEnd(); ++i)
            {
                if (!FoliageTypes[i->Type].IsReady())
                    continue;
                BoundingBox::FromSphere(i->Bounds, box);
                totalBounds = box;
                break;
            }
            ++i;
            // TODO: inline code and use SIMD
            for (; i.IsNotEnd(); ++i)
            {
                if (!FoliageTypes[i->Type].IsReady())
                    continue;
                BoundingBox::FromSphere(i->Bounds, box);
                BoundingBox::Merge(totalBounds, box, totalBounds);
            }
        }

        // Setup first and topmost cluster
        Clusters.Resize(1);
        Root = &Clusters[0];
        Root->Init(totalBounds);
#else
        bool hasTotalBounds = false;
        for (auto& type : FoliageTypes)
        {
            if (!type.IsReady())
            {
                type.Root = nullptr;
                type.Clusters.Clear();
                continue;
            }

            // Calculate total bounds of all instances of this type
            BoundingBox totalBoundsType;
            auto i = Instances.Begin();
            for (; i.IsNotEnd(); ++i)
            {
                if (i->Type == type.Index)
                {
                    BoundingBox::FromSphere(i->Bounds, box);
                    totalBoundsType = box;
                    break;
                }
            }
            ++i;
            // TODO: inline code and use SIMD
            for (; i.IsNotEnd(); ++i)
            {
                if (i->Type == type.Index)
                {
                    BoundingBox::FromSphere(i->Bounds, box);
                    BoundingBox::Merge(totalBoundsType, box, totalBoundsType);
                }
            }

            // Setup first and topmost cluster
            type.Clusters.Resize(1);
            type.Root = &type.Clusters[0];
            type.Root->Init(totalBoundsType);
            if (hasTotalBounds)
            {
                BoundingBox::Merge(totalBounds, totalBoundsType, totalBounds);
            }
            else
            {
                totalBounds = totalBoundsType;
                hasTotalBounds = true;
            }
        }
        ASSERT(hasTotalBounds);
#endif
        ASSERT(!totalBounds.Minimum.IsNanOrInfinity() && !totalBounds.Maximum.IsNanOrInfinity());
        _box = totalBounds;
        BoundingSphere::FromBox(_box, _sphere);
        if (_sceneRenderingKey != -1)
            GetSceneRendering()->UpdateGeometry(this, _sceneRenderingKey);
    }

    // Insert all instances to the clusters
    {
        PROFILE_CPU_NAMED("Create Clusters");
        const float globalDensityScale = GetGlobalDensityScale();
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            const float densityScale = type.UseDensityScaling ? globalDensityScale * type.DensityScalingScale : 1.0f;

            if (type.IsReady() && instance.Random < densityScale)
            {
#if FOLIAGE_USE_SINGLE_QUAD_TREE
                AddToCluster(Clusters, Root, instance);
#else
                AddToCluster(type.Clusters, type.Root, instance);
#endif
            }
        }
    }

#if FOLIAGE_USE_SINGLE_QUAD_TREE
    if (Root)
    {
        PROFILE_CPU_NAMED("Update Cache");
        Root->UpdateTotalBoundsAndCullDistance();
    }
#else
    for (auto& type : FoliageTypes)
    {
        if (type.Root)
        {
            PROFILE_CPU_NAMED("Update Cache");
            type.Root->UpdateTotalBoundsAndCullDistance();
        }
    }
#endif
}

void Foliage::UpdateCullDistance()
{
    PROFILE_CPU();

    {
        PROFILE_CPU_NAMED("Instances");
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            instance.CullDistance = type.CullDistance + type.CullDistanceRandomRange * instance.Random;
        }
    }

#if FOLIAGE_USE_SINGLE_QUAD_TREE
    if (Root)
    {
        PROFILE_CPU_NAMED("Clusters");
        Root->UpdateCullDistance();
    }
#else
    for (auto& type : FoliageTypes)
    {
        if (type.Root)
        {
            PROFILE_CPU_NAMED("Clusters");
            type.Root->UpdateCullDistance();
        }
    }
#endif
}

static float GlobalDensityScale = 1.0f;

float Foliage::GetGlobalDensityScale()
{
    return GlobalDensityScale;
}

bool UpdateFoliageDensityScaling(Actor* actor)
{
    if (auto* foliage = dynamic_cast<Foliage*>(actor))
    {
        foliage->RebuildClusters();
    }

    return true;
}

void Foliage::SetGlobalDensityScale(float value)
{
    value = Math::Saturate(value);
    if (Math::NearEqual(value, GlobalDensityScale))
        return;

    PROFILE_CPU();

    GlobalDensityScale = value;

    Function<bool(Actor*)> f(UpdateFoliageDensityScaling);
    SceneQuery::TreeExecute(f);
}

bool Foliage::Intersects(const Ray& ray, float& distance, Vector3& normal, int32& instanceIndex)
{
    PROFILE_CPU();

    instanceIndex = -1;
    normal = Vector3::Up;
    distance = MAX_float;

    FoliageInstance* instance = nullptr;
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    if (Root)
        Root->Intersects(this, ray, distance, normal, instance);
#else
    float tmpDistance;
    Vector3 tmpNormal;
    FoliageInstance* tmpInstance;
    for (auto& type : FoliageTypes)
    {
        if (type.Root && type.Root->Intersects(this, ray, tmpDistance, tmpNormal, tmpInstance) && tmpDistance < distance)
        {
            distance = tmpDistance;
            normal = tmpNormal;
            instance = tmpInstance;
        }
    }
#endif
    if (instance != nullptr)
    {
        int32 j = 0;
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (&*i == instance)
            {
                instanceIndex = j;
                return true;
            }
            j++;
        }
    }
    return false;
}

void Foliage::Draw(RenderContext& renderContext)
{
    if (Instances.IsEmpty())
        return;
    auto& view = renderContext.View;

    PROFILE_CPU();

    // Cache data per foliage instance type
    for (auto& type : FoliageTypes)
    {
        const auto drawModes = static_cast<DrawPass>(type.DrawModes & view.Pass & (int32)view.GetShadowsDrawPassMask(type.ShadowsMode));
        type._canDraw = type.IsReady() && drawModes != DrawPass::None;
        type._drawModes = drawModes;
        if (type._canDraw)
        {
            for (int32 j = 0; j < type.Entries.Count(); j++)
            {
                auto& e = type.Entries[j];
                e.ReceiveDecals = type.ReceiveDecals != 0;
                e.ShadowsMode = type.ShadowsMode;
            }
        }
    }

    // Draw visible clusters
#if FOLIAGE_USE_SINGLE_QUAD_TREE || !FOLIAGE_USE_DRAW_CALLS_BATCHING
    Mesh::DrawInfo draw;
    draw.Flags = GetStaticFlags();
    draw.DrawModes = (DrawPass)(DrawPass::Default & view.Pass);
    draw.LODBias = 0;
    draw.ForcedLOD = -1;
    draw.VertexColors = nullptr;
#else
    DrawCallsList drawCallsLists[MODEL_MAX_LODS];
#endif
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    if (Root)
        DrawCluster(renderContext, Root, draw);
#else
    for (auto& type : FoliageTypes)
    {
        if (type.Root && type._canDraw && type.Model->CanBeRendered())
        {
#if FOLIAGE_USE_DRAW_CALLS_BATCHING
            // Initialize draw calls for foliage type all LODs meshes
            for (int32 lod = 0; lod < type.Model->LODs.Count(); lod++)
            {
                auto& modelLod = type.Model->LODs[lod];
                DrawCallsList& drawCallsList = drawCallsLists[lod];
                const auto& meshes = modelLod.Meshes;
                drawCallsList.Resize(meshes.Count());
                for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
                {
                    const auto& mesh = meshes[meshIndex];
                    auto& drawCall = drawCallsList[meshIndex];
                    drawCall.DrawCall.Material = nullptr;

                    // Check entry visibility
                    const auto& entry = type.Entries[mesh.GetMaterialSlotIndex()];
                    if (!entry.Visible || !mesh.IsInitialized())
                        continue;
                    const MaterialSlot& slot = type.Model->MaterialSlots[mesh.GetMaterialSlotIndex()];

                    // Select material
                    MaterialBase* material;
                    if (entry.Material && entry.Material->IsLoaded())
                        material = entry.Material;
                    else if (slot.Material && slot.Material->IsLoaded())
                        material = slot.Material;
                    else
                        material = GPUDevice::Instance->GetDefaultMaterial();
                    if (!material || !material->IsSurface())
                        continue;

                    // Select draw modes
                    const auto shadowsMode = static_cast<ShadowsCastingMode>(entry.ShadowsMode & slot.ShadowsMode);
                    const auto drawModes = static_cast<DrawPass>(type._drawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode)) & material->GetDrawModes();
                    if (drawModes == 0)
                        continue;

                    drawCall.DrawCall.Material = material;
                }
            }

            // Draw instances of the foliage type
            BatchedDrawCalls result;
            DrawCluster(renderContext, type.Root, type, drawCallsLists, result);

            // Submit draw calls with valid instances added
            for (auto& e : result)
            {
                auto& batch = e.Value;
                if (batch.Instances.IsEmpty())
                    continue;
                const auto& mesh = *e.Key.Geo;
                const auto& entry = type.Entries[mesh.GetMaterialSlotIndex()];
                const MaterialSlot& slot = type.Model->MaterialSlots[mesh.GetMaterialSlotIndex()];
                const auto shadowsMode = static_cast<ShadowsCastingMode>(entry.ShadowsMode & slot.ShadowsMode);
                const auto drawModes = (DrawPass)(static_cast<DrawPass>(type._drawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode)) & batch.DrawCall.Material->GetDrawModes());

                // Setup draw call
                mesh.GetDrawCallGeometry(batch.DrawCall);
                batch.DrawCall.InstanceCount = 1;
                auto& firstInstance = batch.Instances[0];
                batch.DrawCall.ObjectPosition = firstInstance.InstanceOrigin;
                batch.DrawCall.PerInstanceRandom = firstInstance.PerInstanceRandom;
                auto lightmapArea = firstInstance.InstanceLightmapArea.ToVector4();
                batch.DrawCall.Surface.LightmapUVsArea = *(Rectangle*)&lightmapArea;
                batch.DrawCall.Surface.LODDitherFactor = firstInstance.LODDitherFactor;
                batch.DrawCall.World.SetRow1(Vector4(firstInstance.InstanceTransform1, 0.0f));
                batch.DrawCall.World.SetRow2(Vector4(firstInstance.InstanceTransform2, 0.0f));
                batch.DrawCall.World.SetRow3(Vector4(firstInstance.InstanceTransform3, 0.0f));
                batch.DrawCall.World.SetRow4(Vector4(firstInstance.InstanceOrigin, 1.0f));
                batch.DrawCall.Surface.PrevWorld = batch.DrawCall.World;
                batch.DrawCall.Surface.GeometrySize = mesh.GetBox().GetSize();
                batch.DrawCall.Surface.Skinning = nullptr;
                batch.DrawCall.WorldDeterminantSign = 1;

                const int32 batchIndex = renderContext.List->BatchedDrawCalls.Count();
                renderContext.List->BatchedDrawCalls.Add(MoveTemp(batch));

                // Add draw call to proper draw lists
                if (drawModes & DrawPass::Depth)
                {
                    renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Depth].PreBatchedDrawCalls.Add(batchIndex);
                }
                if (drawModes & DrawPass::GBuffer)
                {
                    if (entry.ReceiveDecals)
                        renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer].PreBatchedDrawCalls.Add(batchIndex);
                    else
                        renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].PreBatchedDrawCalls.Add(batchIndex);
                }
                if (drawModes & DrawPass::Distortion)
                {
                    renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Distortion].PreBatchedDrawCalls.Add(batchIndex);
                }
                if (drawModes & DrawPass::MotionVectors && (_staticFlags & StaticFlags::Transform) == 0)
                {
                    renderContext.List->DrawCallsLists[(int32)DrawCallsListType::MotionVectors].PreBatchedDrawCalls.Add(batchIndex);
                }
                if (drawModes & DrawPass::Forward)
                {
                    // Transparency requires sorting by depth so convert back the batched draw call into normal draw calls (RenderList impl will handle this)
                    batch = renderContext.List->BatchedDrawCalls[batchIndex];
                    DrawCall drawCall = batch.DrawCall;
                    for (int32 j = 0; j < batch.Instances.Count(); j++)
                    {
                        auto& instance = batch.Instances[j];
                        drawCall.ObjectPosition = instance.InstanceOrigin;
                        drawCall.PerInstanceRandom = instance.PerInstanceRandom;
                        lightmapArea = instance.InstanceLightmapArea.ToVector4();
                        drawCall.Surface.LightmapUVsArea = *(Rectangle*)&lightmapArea;
                        drawCall.Surface.LODDitherFactor = instance.LODDitherFactor;
                        drawCall.World.SetRow1(Vector4(instance.InstanceTransform1, 0.0f));
                        drawCall.World.SetRow2(Vector4(instance.InstanceTransform2, 0.0f));
                        drawCall.World.SetRow3(Vector4(instance.InstanceTransform3, 0.0f));
                        drawCall.World.SetRow4(Vector4(instance.InstanceOrigin, 1.0f));
                        const int32 drawCallIndex = renderContext.List->DrawCalls.Count();
                        renderContext.List->DrawCalls.Add(drawCall);
                        renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(drawCallIndex);
                    }
                }
            }
#else
            DrawCluster(renderContext, type.Root, draw);
#endif
        }
    }
#endif
}

void Foliage::DrawGeneric(RenderContext& renderContext)
{
    Draw(renderContext);
}

bool Foliage::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    int32 instanceIndex;
    return Intersects(ray, distance, normal, instanceIndex);
}

// Layout for encoded instance data (serialized as Base64 string)

static constexpr int32 GetInstanceBase64Size(int32 size)
{
    // 4 * (n / 3) -> align up to 4
    return (size * 4 / 3 + 3) & ~3;
}

// [Deprecated on 30.11.2019, expires on 30.11.2021]
struct InstanceEncoded1
{
    int32 Type;
    float Random;
    Transform Transform;

    static constexpr int32 Size = 48;
    static constexpr int32 Base64Size = GetInstanceBase64Size(Size);
};

struct InstanceEncoded2
{
    int32 Type;
    float Random;
    Transform Transform;
    LightmapEntry Lightmap;

    static const int32 Size = 68;
    static const int32 Base64Size = GetInstanceBase64Size(Size);
};

typedef InstanceEncoded2 InstanceEncoded;
static_assert(InstanceEncoded::Size == sizeof(InstanceEncoded), "Please update base64 buffer size to match the encoded instance buffer.");
static_assert(InstanceEncoded::Base64Size == GetInstanceBase64Size(sizeof(InstanceEncoded)), "Please update base64 buffer size to match the encoded instance buffer.");

void Foliage::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Foliage);

    if (FoliageTypes.IsEmpty())
        return;

    PROFILE_CPU();

    stream.JKEY("Foliage");
    stream.StartArray();
    for (auto& type : FoliageTypes)
    {
        stream.StartObject();
        type.Serialize(stream, nullptr);
        stream.EndObject();
    }
    stream.EndArray();

    stream.JKEY("Instances");
    stream.StartArray();
    InstanceEncoded enc;
    char base64[InstanceEncoded::Base64Size + 2];
    base64[0] = '\"';
    base64[InstanceEncoded::Base64Size + 1] = '\"';
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        auto& instance = *i;

        enc.Type = instance.Type;
        enc.Random = instance.Random;
        enc.Transform = instance.Transform;
        enc.Lightmap = instance.Lightmap;

        Encryption::Base64Encode((const byte*)&enc, sizeof(enc), base64 + 1);

        stream.RawValue(base64, InstanceEncoded::Base64Size + 2);
    }
    stream.EndArray();
}

void Foliage::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    PROFILE_CPU();

    // Clear
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    Root = nullptr;
    Clusters.Release();
#endif
    Instances.Release();
    FoliageTypes.Resize(0, false);

    // Deserialize foliage types
    int32 foliageTypesCount = 0;
    const auto& foliageTypesMember = stream.FindMember("Foliage");
    if (foliageTypesMember != stream.MemberEnd() && foliageTypesMember->value.IsArray())
    {
        foliageTypesCount = foliageTypesMember->value.Size();
    }
    if (foliageTypesCount)
    {
        const DeserializeStream& items = foliageTypesMember->value;;
        FoliageTypes.Resize(foliageTypesCount, false);
        for (int32 i = 0; i < foliageTypesCount; i++)
        {
            FoliageTypes[i].Foliage = this;
            FoliageTypes[i].Index = i;
            FoliageTypes[i].Deserialize((DeserializeStream&)items[i], modifier);
        }
    }

    // Skip if no foliage
    if (FoliageTypes.IsEmpty())
        return;

    // Deserialize foliage instances
    int32 foliageInstancesCount = 0;
    const auto& foliageInstancesMember = stream.FindMember("Instances");
    if (foliageInstancesMember != stream.MemberEnd() && foliageInstancesMember->value.IsArray())
    {
        foliageInstancesCount = foliageInstancesMember->value.Size();
    }
    if (foliageInstancesCount)
    {
        const DeserializeStream& items = foliageInstancesMember->value;
        Instances.Resize(foliageInstancesCount);

        if (modifier->EngineBuild <= 6189)
        {
            // [Deprecated on 30.11.2019, expires on 30.11.2021]
            InstanceEncoded1 enc;
            for (int32 i = 0; i < foliageInstancesCount; i++)
            {
                auto& instance = Instances[i];
                auto& item = items[i];

                const int32 length = item.GetStringLength();
                if (length != InstanceEncoded1::Base64Size)
                {
                    LOG(Warning, "Invalid foliage instance data size.");
                    continue;
                }
                Encryption::Base64Decode(item.GetString(), length, (byte*)&enc);

                instance.Type = enc.Type;
                instance.Random = enc.Random;
                instance.Transform = enc.Transform;
                instance.Lightmap = LightmapEntry();
            }
        }
        else
        {
            InstanceEncoded enc;
            for (int32 i = 0; i < foliageInstancesCount; i++)
            {
                auto& instance = Instances[i];
                auto& item = items[i];

                const int32 length = item.GetStringLength();
                if (length != InstanceEncoded::Base64Size)
                {
                    LOG(Warning, "Invalid foliage instance data size.");
                    continue;
                }
                Encryption::Base64Decode(item.GetString(), length, (byte*)&enc);

                instance.Type = enc.Type;
                instance.Random = enc.Random;
                instance.Transform = enc.Transform;
                instance.Lightmap = enc.Lightmap;
            }
        }

#if BUILD_DEBUG
        // Remove invalid instances
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Type < 0 || i->Type >= FoliageTypes.Count())
            {
                LOG(Warning, "Removing invalid foliage instance.");
                Instances.Remove(i);
                --i;
            }
        }
#endif

        // Update cull distance
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            instance.CullDistance = type.CullDistance + type.CullDistanceRandomRange * instance.Random;
        }
    }
}

void Foliage::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateGeometry(this, _sceneRenderingKey);
}

void Foliage::OnEnable()
{
    _sceneRenderingKey = GetSceneRendering()->AddGeometry(this);

    // Base
    Actor::OnEnable();
}

void Foliage::OnDisable()
{
    GetSceneRendering()->RemoveGeometry(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void Foliage::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    PROFILE_CPU();

    // Update instances matrices and cached world bounds
    Vector3 corners[8];
    Matrix world, matrix;
    GetLocalToWorldMatrix(world);
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        auto& instance = *i;
        auto type = &FoliageTypes[instance.Type];

        // Update world matrix
        instance.Transform.GetWorld(matrix);
        Matrix::Multiply(matrix, world, instance.World);

        // Update bounds
        instance.Bounds = BoundingSphere::Empty;
        if (!type->IsReady())
            continue;
        auto& meshes = type->Model->LODs[0].Meshes;
        for (int32 j = 0; j < meshes.Count(); j++)
        {
            meshes[j].GetCorners(corners);

            for (int32 k = 0; k < 8; k++)
            {
                Vector3::Transform(corners[k], instance.World, corners[k]);
            }
            BoundingSphere meshBounds;
            BoundingSphere::FromPoints(corners, 8, meshBounds);

            BoundingSphere::Merge(instance.Bounds, meshBounds, instance.Bounds);
        }
    }

    RebuildClusters();
}
