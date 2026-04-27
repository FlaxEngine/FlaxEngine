// Copyright (c) Wojciech Figat. All rights reserved.

#include "Foliage.h"
#include "FoliageType.h"
#include "FoliageCluster.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Random.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Content/Deprecated.h"
#if !FOLIAGE_USE_SINGLE_QUAD_TREE
#include "Engine/Threading/JobSystem.h"
#endif
#include "Engine/Level/SceneQuery.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/GlobalSignDistanceFieldPass.h"
#include "Engine/Renderer/GI/GlobalSurfaceAtlasPass.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Utilities/Encryption.h"
#include <ThirdParty/LZ4/lz4.h>

#define FOLIAGE_GET_DRAW_MODES(renderContext, type) (type._drawModes & renderContext.View.Pass & renderContext.View.GetShadowsDrawPassMask(type.ShadowsMode))
#define FOLIAGE_CAN_DRAW(renderContext, type) (type.IsReady() && FOLIAGE_GET_DRAW_MODES(renderContext, type) != DrawPass::None && type.Model->CanBeRendered())

Foliage::Foliage(const SpawnParams& params)
    : Actor(params)
{
    _disableFoliageTypeEvents = false;

    // When using separate quad-tree for each foliage type we can run async job to draw them in separate, otherwise just draw whole foliage in async as one
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    _drawCategory = SceneRendering::SceneDrawAsync;
#else
    _drawCategory = SceneRendering::SceneDraw;
#endif
}

void Foliage::AddToCluster(ChunkedArray<FoliageCluster, FOLIAGE_CLUSTER_CHUNKS_SIZE>& clusters, FoliageCluster* cluster, FoliageInstance& instance)
{
    ASSERT_LOW_LAYER(instance.Bounds.Radius > ZeroTolerance);

    // Minor clusters don't use bounds intersection but try to find the first free cluster instead
    if (cluster->IsMinor)
    {
        // Insert into the first non-full child cluster or subdivide 1st child
#define CHECK_CHILD(idx) \
        if (cluster->Children[idx]->Instances.Count() < FOLIAGE_CLUSTER_CAPACITY) \
        { \
           cluster->Children[idx]->Instances.Add(&instance); \
           return; \
        }
        CHECK_CHILD(3);
        CHECK_CHILD(2);
        CHECK_CHILD(1);
        cluster = cluster->Children[0];
#undef CHECK_CHILD
    }
    else
    {
        // Find target cluster
        ASSERT(cluster->Bounds.Intersects(instance.Bounds));
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
        const Vector3 size = max - min;
        cluster->Children[0]->Init(BoundingBox(min, min + size * Vector3(0.5f, 1.0f, 0.5f)));
        cluster->Children[1]->Init(BoundingBox(min + size * Vector3(0.5f, 0.0f, 0.5f), max));
        cluster->Children[2]->Init(BoundingBox(min + size * Vector3(0.5f, 0.0f, 0.0f), min + size * Vector3(1.0f, 1.0f, 0.5f)));
        cluster->Children[3]->Init(BoundingBox(min + size * Vector3(0.0f, 0.0f, 0.5f), min + size * Vector3(0.5f, 1.0f, 1.0f)));
        if (cluster->IsMinor || size.MinValue() < 1.0f)
        {
            // Mark children as minor to avoid infinite subdivision
            cluster->IsMinor = true;
            cluster->Children[0]->IsMinor = true;
            cluster->Children[1]->IsMinor = true;
            cluster->Children[2]->IsMinor = true;
            cluster->Children[3]->IsMinor = true;
        }

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

void Foliage::DrawInstance(DrawContext& context, FoliageInstance& instance, Model* model, int32 lod, float lodDitherFactor, DrawCallsList* drawCallsLists, BatchedDrawCalls& result) const
{
    const auto& meshes = model->LODs.Get()[lod].Meshes;
    for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
    {
        auto& drawCall = drawCallsLists[lod][meshIndex];
        if (!drawCall.Material)
            continue;

        DrawKey key;
        key.Mat = drawCall.Material;
        key.Geo = &meshes.Get()[meshIndex];
        key.Lightmap = instance.LightmapTextureIndex;
        auto* e = result.TryGet(key);
        if (!e)
        {
            e = &result.Add(key, BatchedDrawCall(context.RenderContext.List))->Value;
            e->DrawCall.Material = key.Mat;
            e->DrawCall.Surface.Lightmap = EnumHasAnyFlags(_staticFlags, StaticFlags::Lightmap) && _scene ? _scene->LightmapsData.GetReadyLightmap(key.Lightmap) : nullptr;
            e->DrawCall.Surface.GeometrySize = key.Geo->GetBox().GetSize();
        }

        // Add instance to the draw batch
        auto& instanceData = e->Instances.AddOne();
        Matrix world;
        Transform transform;
        _transform.LocalToWorld(instance.Transform, transform);
        const Float3 translation = transform.Translation - context.ViewOrigin;
        Matrix::Transformation(transform.Scale, transform.Orientation, translation, world);
        constexpr float worldDeterminantSign = 1.0f;
        instanceData.Store(world, world, instance.LightmapUVsArea, drawCall.Surface.GeometrySize, instance.Random, worldDeterminantSign, lodDitherFactor);
    }
}

void Foliage::DrawCluster(DrawContext& context, FoliageCluster* cluster, DrawCallsList* drawCallsLists, BatchedDrawCalls& result) const
{
    // Skip clusters that around too far from view
    if (Float3::Distance(context.LodView.Position, cluster->TotalBoundsSphere.Center - context.LodView.Origin) - (float)cluster->TotalBoundsSphere.Radius > cluster->MaxCullDistance)
        return;
    //DebugDraw::DrawBox(cluster->Bounds, Color::Red);

    // Draw visible children
    if (cluster->Children[0])
    {
        BoundingBox box;
#define DRAW_CLUSTER(idx) \
        box = cluster->Children[idx]->TotalBounds; \
        box.Minimum -= context.ViewOrigin; \
        box.Maximum -= context.ViewOrigin; \
		if (context.RenderContext.View.CullingFrustum.Intersects(box)) \
			DrawCluster(context, cluster->Children[idx], drawCallsLists, result)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    //else // Minor clusters can be subdivided and contain instances
    {
        // Draw visible instances
        const auto frame = Engine::FrameCount;
        const auto model = context.FoliageType.Model.Get();
        const auto transitionLOD = context.RenderContext.View.Pass != DrawPass::Depth; // Let the main view pass update LOD transitions
        // TODO: move DrawState to be stored per-view (so shadows can fade objects on their own)
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances.Get()[i];
            BoundingSphere sphere = instance.Bounds;
            sphere.Center -= context.ViewOrigin;
            if (Float3::Distance(context.LodView.Position, sphere.Center) - (float)sphere.Radius < instance.CullDistance &&
                context.RenderContext.View.CullingFrustum.Intersects(sphere) &&
                RenderTools::ComputeBoundsScreenRadiusSquared(sphere.Center, (float)sphere.Radius, context.RenderContext.View) * context.ViewScreenSizeSq >= context.MinObjectPixelSizeSq)
            {
                const auto modelFrame = instance.DrawStatePrevFrame + 1;

                // Select a proper LOD index (model may be culled)
                int32 lodIndex = RenderTools::ComputeModelLOD(model, sphere.Center, (float)sphere.Radius, context.RenderContext);
                if (lodIndex == -1)
                {
                    // Handling model fade-out transition
                    if (modelFrame == frame && instance.DrawStatePrevLOD != -1)
                    {
                        if (transitionLOD)
                        {
                            // Check if start transition
                            if (instance.DrawStateLODTransition == 255)
                            {
                                instance.DrawStateLODTransition = 0;
                            }

                            RenderTools::UpdateModelLODTransition(instance.DrawStateLODTransition);

                            // Check if end transition
                            if (instance.DrawStateLODTransition == 255)
                            {
                                instance.DrawStatePrevLOD = lodIndex;
                            }
                            else
                            {
                                const auto prevLOD = model->ClampLODIndex(instance.DrawStatePrevLOD);
                                const float normalizedProgress = static_cast<float>(instance.DrawStateLODTransition) * (1.0f / 255.0f);
                                DrawInstance(context, instance, model, prevLOD, normalizedProgress, drawCallsLists, result);
                            }
                        }
                        else if (instance.DrawStateLODTransition < 255)
                        {
                            const auto prevLOD = model->ClampLODIndex(instance.DrawStatePrevLOD);
                            const float normalizedProgress = static_cast<float>(instance.DrawStateLODTransition) * (1.0f / 255.0f);
                            DrawInstance(context, instance, model, prevLOD, normalizedProgress, drawCallsLists, result);
                        }
                    }
                    instance.DrawStatePrevFrame = frame;
                    continue;
                }
                lodIndex += context.RenderContext.View.ModelLODBias;
                lodIndex = model->ClampLODIndex(lodIndex);

                if (transitionLOD)
                {
                    // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
                    if (modelFrame == frame)
                    {
                        // Check if start transition
                        if (instance.DrawStatePrevLOD != lodIndex && instance.DrawStateLODTransition == 255)
                        {
                            instance.DrawStateLODTransition = 0;
                        }

                        RenderTools::UpdateModelLODTransition(instance.DrawStateLODTransition);

                        // Check if end transition
                        if (instance.DrawStateLODTransition == 255)
                        {
                            instance.DrawStatePrevLOD = lodIndex;
                        }
                    }
                    // Check if there was a gap between frames in drawing this model instance
                    else if (modelFrame < frame || instance.DrawStatePrevLOD == -1)
                    {
                        // Reset state
                        instance.DrawStatePrevLOD = lodIndex;
                        instance.DrawStateLODTransition = 0;
                    }
                }

                // Draw
                if (instance.DrawStatePrevLOD == lodIndex)
                {
                    DrawInstance(context, instance, model, lodIndex, 0.0f, drawCallsLists, result);
                }
                else if (instance.DrawStatePrevLOD == -1)
                {
                    const float normalizedProgress = static_cast<float>(instance.DrawStateLODTransition) * (1.0f / 255.0f);
                    DrawInstance(context, instance, model, lodIndex, 1.0f - normalizedProgress, drawCallsLists, result);
                }
                else
                {
                    const auto prevLOD = model->ClampLODIndex(instance.DrawStatePrevLOD);
                    const float normalizedProgress = static_cast<float>(instance.DrawStateLODTransition) * (1.0f / 255.0f);
                    DrawInstance(context, instance, model, prevLOD, normalizedProgress, drawCallsLists, result);
                    DrawInstance(context, instance, model, lodIndex, normalizedProgress - 1.0f, drawCallsLists, result);
                }

                //DebugDraw::DrawSphere(instance.Bounds, Color::YellowGreen);

                if (transitionLOD)
                    instance.DrawStatePrevFrame = frame;
            }
        }
    }
}

#else

void Foliage::DrawCluster(DrawContext& context, FoliageCluster* cluster, Mesh::DrawInfo& draw)
{
    // Skip clusters that around too far from view
    if (Float3::Distance(context.LodView.Position, cluster->TotalBoundsSphere.Center - context.LodView.Origin) - (float)cluster->TotalBoundsSphere.Radius > cluster->MaxCullDistance)
        return;
    //DebugDraw::DrawBox(cluster->Bounds, Color::Red);

    // Draw visible children
    if (cluster->Children[0])
    {
        BoundingBox box;
#define DRAW_CLUSTER(idx) \
        box = cluster->Children[idx]->TotalBounds; \
        box.Minimum -= context.ViewOrigin; \
        box.Maximum -= context.ViewOrigin; \
		if (context.RenderContext.View.CullingFrustum.Intersects(box)) \
			DrawCluster(context, cluster->Children[idx], draw)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    //else // Minor clusters can be subdivided and contain instances
    {
        // Draw visible instances
        const auto frame = Engine::FrameCount;
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances[i];
            auto& type = FoliageTypes[instance.Type];
            BoundingSphere sphere = instance.Bounds;
            sphere.Center -= context.ViewOrigin;

            // Check if can draw this instance
            if (type._canDraw &&
                Float3::Distance(context.LodView.Position, sphere.Center) - (float)sphere.Radius < instance.CullDistance &&
                context.RenderContext.View.CullingFrustum.Intersects(sphere) &&
                RenderTools::ComputeBoundsScreenRadiusSquared(sphere.Center, sphere.Radius, context.RenderContext.View) * context.ViewScreenSizeSq >= context.MinObjectPixelSizeSq)
            {
                Matrix world;
                const Transform transform = _transform.LocalToWorld(instance.Transform);
                const Float3 translation = transform.Translation - context.ViewOrigin;
                Matrix::Transformation(transform.Scale, transform.Orientation, translation, world);

                // Disable motion blur
                GeometryDrawStateData drawState;
                drawState.PrevWorld = world;
                instance.DrawState.PrevWorld = world;

                // Draw model
                draw.Lightmap = _scene->LightmapsData.GetReadyLightmap(instance.LightmapTextureIndex);
                draw.LightmapUVs = &instance.Lightmap.UVsArea;
                draw.Buffer = &type.Entries;
                draw.World = &world;
                draw.DrawState = &drawState;
                draw.Bounds = sphere;
                draw.PerInstanceRandom = instance.Random;
                draw.DrawModes = type._drawModes;
                draw.SetStencilValue(_layer);
                type.Model->Draw(context.RenderContext, draw);

                //DebugDraw::DrawSphere(instance.Bounds, Color::YellowGreen);

                instance.DrawStatePrevFrame = frame;
            }
        }
    }
}

#endif

#if !FOLIAGE_USE_SINGLE_QUAD_TREE

void Foliage::DrawClusterGlobalSDF(class GlobalSignDistanceFieldPass* globalSDF, const BoundingBox& globalSDFBounds, FoliageCluster* cluster, const FoliageType& type)
{
    if (cluster->Children[0])
    {
        // Draw children recursive
#define DRAW_CLUSTER(idx) \
		if (globalSDFBounds.Intersects(cluster->Children[idx]->TotalBounds)) \
			DrawClusterGlobalSDF(globalSDF, globalSDFBounds, cluster->Children[idx], type)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    //else // Minor clusters can be subdivided and contain instances
    {
        // Draw visible instances
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances[i];
            if (CollisionsHelper::BoxIntersectsSphere(globalSDFBounds, instance.Bounds))
            {
                const Transform transform = _transform.LocalToWorld(instance.Transform);
                BoundingBox bounds;
                BoundingBox::FromSphere(instance.Bounds, bounds);
                globalSDF->RasterizeModelSDF(this, type.Model->SDF, transform, bounds);
            }
        }
    }
}

void Foliage::DrawClusterGlobalSA(GlobalSurfaceAtlasPass* globalSA, const Vector4& cullingPosDistance, FoliageCluster* cluster, const FoliageType& type, const BoundingBox& localBounds)
{
    if (cluster->Children[0])
    {
        // Draw children recursive
#define DRAW_CLUSTER(idx) \
        if (CollisionsHelper::DistanceBoxPoint(cluster->Children[idx]->TotalBounds, Vector3(cullingPosDistance)) < cullingPosDistance.W) \
            DrawClusterGlobalSA(globalSA, cullingPosDistance, cluster->Children[idx], type, localBounds)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    //else // Minor clusters can be subdivided and contain instances
    {
        // Draw visible instances
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances[i];
            if (CollisionsHelper::DistanceSpherePoint(instance.Bounds, Vector3(cullingPosDistance)) < cullingPosDistance.W)
            {
                const Transform transform = _transform.LocalToWorld(instance.Transform);
                globalSA->RasterizeActor(this, &instance, instance.Bounds, transform, localBounds, MAX_uint32, true, 0.5f);
            }
        }
    }
}

void Foliage::DrawFoliageJob(int32 i)
{
    PROFILE_CPU();
    PROFILE_MEM(Graphics);
    const int32 foliageIndex = i / _renderContextBatch->Contexts.Count();
    const int32 contextIndex = i % _renderContextBatch->Contexts.Count();
    const FoliageType& type = FoliageTypes[foliageIndex];
    if (!type._canDraw)
        return;
    DrawCallsList drawCallsLists[MODEL_MAX_LODS];
    RenderContext& renderContext = _renderContextBatch->Contexts[contextIndex];
#if !FOLIAGE_USE_SINGLE_QUAD_TREE && FOLIAGE_USE_DRAW_CALLS_BATCHING
    DrawType(renderContext, type, drawCallsLists);
#else
    Mesh::DrawInfo draw;
    draw.Flags = GetStaticFlags();
    draw.DrawModes = (DrawPass)(DrawPass::Default & renderContext.View.Pass);
    draw.LODBias = 0;
    draw.ForcedLOD = -1;
    draw.VertexColors = nullptr;
    draw.Deformation = nullptr;
    DrawType(renderContext, type, draw);
#endif
}

#endif

#if !FOLIAGE_USE_SINGLE_QUAD_TREE && FOLIAGE_USE_DRAW_CALLS_BATCHING
void Foliage::DrawType(RenderContext& renderContext, const FoliageType& type, DrawCallsList* drawCallsLists)
#else
void Foliage::DrawType(RenderContext& renderContext, const FoliageType& type, Mesh::DrawInfo& draw)
#endif
{
    if (!type.Root || !FOLIAGE_CAN_DRAW(renderContext, type))
        return;
    const DrawPass typeDrawModes = FOLIAGE_GET_DRAW_MODES(renderContext, type);
    PROFILE_CPU_ASSET(type.Model);
    DrawContext context
    {
        renderContext,
        renderContext.LodProxyView ? *renderContext.LodProxyView : renderContext.View,
        type,
        renderContext.View.Origin,
        Math::Square(Graphics::Shadows::MinObjectPixelSize),
        renderContext.View.ScreenSize.X * renderContext.View.ScreenSize.Y,
    };
    if (context.RenderContext.View.Pass != DrawPass::Depth)
        context.MinObjectPixelSizeSq = 0.0f; // Don't use it in main view
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
            const auto& mesh = meshes.Get()[meshIndex];
            auto& drawCall = drawCallsList.Get()[meshIndex];
            drawCall.Material = nullptr; // DrawInstance skips draw calls from meshes with unset material

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
            const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
            const auto drawModes = typeDrawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode) & material->GetDrawModes();
            if (drawModes == DrawPass::None)
                continue;

            drawCall.Material = material;
            drawCall.Surface.GeometrySize = mesh.GetBox().GetSize();
        }
    }

    // Draw instances of the foliage type
    BatchedDrawCalls result(&renderContext.List->Memory);
    DrawCluster(context, type.Root, drawCallsLists, result);

    // Submit draw calls with valid instances added
    for (auto& e : result)
    {
        auto& batch = e.Value;
        if (batch.Instances.IsEmpty())
            continue;
        const auto& mesh = *e.Key.Geo;
        const auto& entry = type.Entries[mesh.GetMaterialSlotIndex()];
        const MaterialSlot& slot = type.Model->MaterialSlots[mesh.GetMaterialSlotIndex()];
        const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
        const auto drawModes = typeDrawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode) & batch.DrawCall.Material->GetDrawModes();

        // Setup draw call
        mesh.GetDrawCallGeometry(batch.DrawCall);
        batch.DrawCall.InstanceCount = 1;
        auto& firstInstance = batch.Instances[0];
        firstInstance.Load(batch.DrawCall);
#if USE_EDITOR
        if (renderContext.View.Mode == ViewMode::LightmapUVsDensity)
            batch.DrawCall.Surface.LODDitherFactor = type.ScaleInLightmap; // See LightmapUVsDensityMaterialShader
#endif

        if (EnumHasAnyFlags(drawModes, DrawPass::Forward))
        {
            // Transparency requires sorting by depth so convert back the batched draw call into normal draw calls (RenderList impl will handle this)
            DrawCall drawCall = batch.DrawCall;
            for (int32 j = 0; j < batch.Instances.Count(); j++)
            {
                auto& instance = batch.Instances[j];
                instance.Load(drawCall);
                const int32 drawCallIndex = renderContext.List->DrawCalls.Add(drawCall);
                renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(drawCallIndex);
            }
        }

        // Add draw call batch
        const int32 batchIndex = renderContext.List->BatchedDrawCalls.Add(MoveTemp(batch));

        // Add draw call to proper draw lists
        if (EnumHasAnyFlags(drawModes, DrawPass::Depth))
        {
            renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Depth].PreBatchedDrawCalls.Add(batchIndex);
        }
        if (EnumHasAnyFlags(drawModes, DrawPass::GBuffer))
        {
            if (entry.ReceiveDecals)
                renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer].PreBatchedDrawCalls.Add(batchIndex);
            else
                renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].PreBatchedDrawCalls.Add(batchIndex);
        }
        if (EnumHasAnyFlags(drawModes, DrawPass::Distortion))
        {
            renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Distortion].PreBatchedDrawCalls.Add(batchIndex);
        }
        if (EnumHasAnyFlags(drawModes, DrawPass::MotionVectors) && (_staticFlags & StaticFlags::Transform) == StaticFlags::None)
        {
            renderContext.List->DrawCallsLists[(int32)DrawCallsListType::MotionVectors].PreBatchedDrawCalls.Add(batchIndex);
        }
    }
#else
    DrawCluster(context, type.Root, draw);
#endif
}

void Foliage::InitType(const RenderView& view, FoliageType& type)
{
    const DrawPass drawModes = type._drawModes & view.Pass & view.GetShadowsDrawPassMask(type.ShadowsMode);
    type._canDraw = type.IsReady() && drawModes != DrawPass::None && type.Model && type.Model->CanBeRendered();
    bool drawModesDirty = false;
    for (int32 j = 0; j < type.Entries.Count(); j++)
    {
        auto& e = type.Entries[j];
        e.ReceiveDecals = type.ReceiveDecals != 0;
        e.ShadowsMode = type.ShadowsMode;
        if (type._drawModesDirty)
        {
            type._drawModesDirty = 0;
            drawModesDirty = true;
        }
    }
    if (drawModesDirty)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::DrawModes);
}

void Foliage::UpdateBounds()
{
    PROFILE_CPU();
    PROFILE_MEM(LevelFoliage);

    // Cache bounds for each foliage type
    BitArray<> typeReady;
    Array<BoundingSphere> typeBounds;
    typeReady.Resize(FoliageTypes.Count());
    typeBounds.Resize(FoliageTypes.Count());
    for (int32 i = 0; i < typeBounds.Count(); i++)
    {
        auto& type = FoliageTypes[i];
        bool ready = type.IsReady();
        typeReady.Set(i, ready);
        if (ready)
            BoundingSphere::FromBox(type.Model->GetBox(), typeBounds[i]);
    }

    // Update bounds for all instances
    Matrix foliageWorld, instanceLocal, instanceWorld;
    GetLocalToWorldMatrix(foliageWorld);
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        auto& instance = *i;
        if (typeReady.Get(instance.Type))
        {
            instance.Transform.GetWorld(instanceLocal);
            Matrix::Multiply(foliageWorld, instanceLocal, instanceWorld);
            BoundingSphere::Transform(typeBounds[instance.Type], instanceWorld, instance.Bounds);
        }
        else
        {
            instance.Bounds = BoundingSphere::Empty;
        }
    }
}

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
    PROFILE_MEM(LevelFoliage);

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
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Type == index)
            result++;
    }
    return result;
}

void Foliage::AddInstance(const FoliageInstance& instance)
{
    PROFILE_MEM(LevelFoliage);
    ASSERT(instance.Type >= 0 && instance.Type < FoliageTypes.Count());
    auto type = &FoliageTypes[instance.Type];

    // Add instance
    auto data = Instances.Add(instance);
    data->Bounds = BoundingSphere::Empty;
    data->Random = Random::Rand();
    data->CullDistance = type->CullDistance + type->CullDistanceRandomRange * data->Random;

    // Validate foliage type model
    if (!type->IsReady())
        return;

    // Update bounds
    Vector3 corners[8];
    auto& meshes = type->Model->LODs[0].Meshes;
    const Transform transform = _transform.LocalToWorld(data->Transform);
    for (int32 j = 0; j < meshes.Count(); j++)
    {
        meshes[j].GetBox().GetCorners(corners);

        for (int32 k = 0; k < 8; k++)
        {
            Vector3::Transform(corners[k], transform, corners[k]);
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
    const auto& type = FoliageTypes[instance.Type];

    // Change transform
    instance.Transform = value;

    // Update bounds
    if (type.IsReady())
    {
        BoundingSphere typeBounds;
        BoundingSphere::FromBox(type.Model->GetBox(), typeBounds);
        Matrix foliageWorld, instanceLocal, instanceWorld;
        GetLocalToWorldMatrix(foliageWorld);
        instance.Transform.GetWorld(instanceLocal);
        Matrix::Multiply(foliageWorld, instanceLocal, instanceWorld);
        BoundingSphere::Transform(typeBounds, instanceWorld, instance.Bounds);
    }
    else
    {
        instance.Bounds = BoundingSphere::Empty;
    }
}

void Foliage::OnFoliageTypeModelLoaded(int32 index)
{
    if (_disableFoliageTypeEvents)
        return;
    PROFILE_CPU();
    PROFILE_MEM(LevelFoliage);
    auto& type = FoliageTypes[index];
    ASSERT(type.IsReady());

    // Update bounds for instances using this type
    bool hasAnyInstance = false;
#if !FOLIAGE_USE_SINGLE_QUAD_TREE
    BoundingBox totalBoundsType, box;
#endif
    {
        PROFILE_CPU_NAMED("Update Bounds");

        BoundingSphere typeBounds;
        BoundingSphere::FromBox(type.Model->GetBox(), typeBounds);
        Matrix foliageWorld, instanceLocal, instanceWorld;
        GetLocalToWorldMatrix(foliageWorld);

        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            if (instance.Type != index)
                continue;

            instance.Transform.GetWorld(instanceLocal);
            Matrix::Multiply(foliageWorld, instanceLocal, instanceWorld);
            BoundingSphere::Transform(typeBounds, instanceWorld, instance.Bounds);

#if !FOLIAGE_USE_SINGLE_QUAD_TREE
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
            GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
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
    PROFILE_MEM(LevelFoliage);

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
            GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
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
            GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
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

void Foliage::RemoveAllInstances()
{
    Instances.Clear();
    RebuildClusters();
}

void Foliage::RemoveLightmap()
{
    for (auto& e : Instances)
        e.RemoveLightmap();
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

bool Foliage::Intersects(const Ray& ray, Real& distance, Vector3& normal, int32& instanceIndex)
{
    PROFILE_CPU();

    instanceIndex = -1;
    normal = Vector3::Up;
    distance = MAX_Real;

    FoliageInstance* instance = nullptr;
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    if (Root)
        Root->Intersects(this, ray, distance, normal, instance);
#else
    Real tmpDistance;
    Vector3 tmpNormal;
    FoliageInstance* tmpInstance;
    for (const auto& type : FoliageTypes)
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
    PROFILE_CPU();
    const RenderView& view = renderContext.View;

    // Cache data per foliage instance type
    for (auto& type : FoliageTypes)
    {
        InitType(renderContext.View, type);
    }

    if (renderContext.View.Pass == DrawPass::GlobalSDF)
    {
        auto globalSDF = GlobalSignDistanceFieldPass::Instance();
        BoundingBox globalSDFBounds;
        globalSDF->GetCullingData(globalSDFBounds);
#if FOLIAGE_USE_SINGLE_QUAD_TREE
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            if (type._canDraw && CollisionsHelper::BoxIntersectsSphere(globalSDFBounds, instance.Bounds))
            {
                const Transform transform = _transform.LocalToWorld(instance.Transform);
                BoundingBox bounds;
                BoundingBox::FromSphere(instance.Bounds, bounds);
                globalSDF->RasterizeModelSDF(this, type.Model->SDF, transform, bounds);
            }
        }
#else
        for (auto& type : FoliageTypes)
        {
            if (type.Root && FOLIAGE_CAN_DRAW(renderContext, type))
                DrawClusterGlobalSDF(globalSDF, globalSDFBounds, type.Root, type);
        }
#endif
        return;
    }

    if (renderContext.View.Pass == DrawPass::GlobalSurfaceAtlas)
    {
        // Draw foliage instances into Global Surface Atlas
        auto globalSA = GlobalSurfaceAtlasPass::Instance();
        Vector4 cullingPosDistance;
        globalSA->GetCullingData(cullingPosDistance);
#if FOLIAGE_USE_SINGLE_QUAD_TREE
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            if (type._canDraw && CollisionsHelper::DistanceSpherePoint(instance.Bounds, Vector3(cullingPosDistance)) < cullingPosDistance.W)
            {
                const Transform transform = _transform.LocalToWorld(instance.Transform);
                BoundingBox localBounds = type.Model->LODs.Last().GetBox();
                globalSA->RasterizeActor(this, &instance, instance.Bounds, transform, localBounds, MAX_uint32, true, 0.5f);
            }
        }
#else
        for (auto& type : FoliageTypes)
        {
            if (type.Root && FOLIAGE_CAN_DRAW(renderContext, type))
            {
                BoundingBox localBounds = type.Model->LODs.Last().GetBox();
                DrawClusterGlobalSA(globalSA, cullingPosDistance, type.Root, type, localBounds);
            }
        }
#endif
        return;
    }
    if (EnumHasAnyFlags(renderContext.View.Pass, DrawPass::GlobalSurfaceAtlas))
    {
        // Draw single foliage instance projection into Global Surface Atlas
        auto& instance = *(FoliageInstance*)GlobalSurfaceAtlasPass::Instance()->GetCurrentActorObject();
        auto& type = FoliageTypes[instance.Type];
        InitType(renderContext.View, type);
        Matrix world;
        const Transform transform = _transform.LocalToWorld(instance.Transform);
        renderContext.View.GetWorldMatrix(transform, world);
        GeometryDrawStateData drawState;
        drawState.PrevWorld = world;
        Mesh::DrawInfo draw;
        draw.Flags = GetStaticFlags();
        draw.LODBias = 0;
        draw.ForcedLOD = -1;
        draw.SortOrder = 0;
        draw.VertexColors = nullptr;
        draw.Lightmap = _scene ? _scene->LightmapsData.GetReadyLightmap(instance.LightmapTextureIndex) : nullptr;
        draw.LightmapUVs = &instance.LightmapUVsArea;
        draw.Buffer = &type.Entries;
        draw.World = &world;
        draw.DrawState = &drawState;
        draw.Deformation = nullptr;
        draw.Bounds = instance.Bounds;
        draw.PerInstanceRandom = instance.Random;
        draw.DrawModes = type._drawModes & view.Pass & view.GetShadowsDrawPassMask(type.ShadowsMode);
        draw.SetStencilValue(_layer);
        type.Model->Draw(renderContext, draw);
        return;
    }

    // Draw visible clusters
#if FOLIAGE_USE_SINGLE_QUAD_TREE || !FOLIAGE_USE_DRAW_CALLS_BATCHING
    Mesh::DrawInfo draw;
    draw.Flags = GetStaticFlags();
    draw.DrawModes = (DrawPass)(DrawPass::Default & view.Pass);
    draw.LODBias = 0;
    draw.ForcedLOD = -1;
    draw.VertexColors = nullptr;
    draw.Deformation = nullptr;
#else
    DrawCallsList draw[MODEL_MAX_LODS];
#endif
#if FOLIAGE_USE_SINGLE_QUAD_TREE
    if (Root)
        DrawCluster(renderContext, Root, draw);
#else
    for (auto& type : FoliageTypes)
    {
        DrawType(renderContext, type, draw);
    }
#endif
}

void Foliage::Draw(RenderContextBatch& renderContextBatch)
{
    if (Instances.IsEmpty())
        return;

#if !FOLIAGE_USE_SINGLE_QUAD_TREE
    // Run async job for each foliage type
    const RenderView& view = renderContextBatch.GetMainContext().View;
    if (EnumHasAnyFlags(view.Pass, DrawPass::GBuffer) && !(view.Pass & (DrawPass::GlobalSDF | DrawPass::GlobalSurfaceAtlas)) && renderContextBatch.EnableAsync)
    {
        // Cache data per foliage instance type
        for (FoliageType& type : FoliageTypes)
            InitType(view, type);

        // Run async job for each foliage type
        _renderContextBatch = &renderContextBatch;
        Function<void(int32)> func;
        func.Bind<Foliage, &Foliage::DrawFoliageJob>(this);
        const int64 waitLabel = JobSystem::Dispatch(func, FoliageTypes.Count() * renderContextBatch.Contexts.Count());
        renderContextBatch.WaitLabels.Add(waitLabel);
        return;
    }
#endif

    // Fallback to default rendering
    Actor::Draw(renderContextBatch);
}

bool Foliage::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
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
    Float3 Translation;
    Quaternion Orientation;
    Float3 Scale;

    static constexpr int32 Size = 48;
    static constexpr int32 Base64Size = GetInstanceBase64Size(Size);
};

// [Deprecated in v1.13]
struct InstanceEncoded2
{
    int32 Type;
    float Random;
    Float3 Translation;
    Quaternion Orientation;
    Float3 Scale;
    int32 LightmapTextureIndex;
    Rectangle LightmapUVsArea;

    static const int32 Size = 68;
    static const int32 Base64Size = GetInstanceBase64Size(Size);
};

// [Deprecated in v1.13]
typedef InstanceEncoded2 InstanceEncoded;
static_assert(InstanceEncoded::Size == sizeof(InstanceEncoded), "Please update base64 buffer size to match the encoded instance buffer.");
static_assert(InstanceEncoded::Base64Size == GetInstanceBase64Size(sizeof(InstanceEncoded)), "Please update base64 buffer size to match the encoded instance buffer.");

struct FoliageChunkMeta
{
    uint16 InstanceCounter;
};

struct FoliageInstanceData
{
    union
    {
        struct
        {
            uint16 Type : 14; // Max 16,384 foliage types, which is more than enough for any use case
            uint16 OrientationNegativeW : 1;
            uint16 HasLightmap : 1;
        };
        uint16 Packed;
    };
    int16 OrientationX, OrientationY, OrientationZ;
    float Random;
    Float3 Position, Scale;
};

struct FoliageWriter
{
    static constexpr int32 InstanceSizeApprox = sizeof(FoliageInstanceData);
    static constexpr int32 InstancesPerChunk = 64;
    typedef uint16 CompressedSize;
    int32 InstanceCounter = 0;
    Foliage::SerializeStream& Stream;
    MemoryWriteStream Memory;
    Array<byte> Compressed;
    Array<char> Base64;

    FoliageWriter(Foliage::SerializeStream& stream)
        : Stream(stream)
        , Memory(Math::RoundUpToPowerOf2(InstancesPerChunk* InstanceSizeApprox))
    {
        Memory.Move<FoliageChunkMeta>();
    }

    void Write(const FoliageInstance& instance)
    {
        // Fixed-size data
        FoliageInstanceData data;
        data.Type = (uint16)instance.Type;
        data.HasLightmap = instance.HasLightmap();
        data.Random = instance.Random;
        data.Position = (Float3)instance.Transform.Translation;
        data.Scale = instance.Transform.Scale;
        data.OrientationX = (int16)(instance.Transform.Orientation.X * MAX_int16);
        data.OrientationY = (int16)(instance.Transform.Orientation.Y * MAX_int16);
        data.OrientationZ = (int16)(instance.Transform.Orientation.Z * MAX_int16);
        data.OrientationNegativeW = instance.Transform.Orientation.W < 0;
        Memory.Write(data);

        // Lightmap data (if used)
        if (data.HasLightmap)
        {
            Memory.Write(instance.LightmapTextureIndex);
            Memory.Write(instance.LightmapUVsArea);
        }

        // Move to the next instance
        InstanceCounter++;
        if (InstanceCounter >= InstancesPerChunk)
            Flush();
    }

    void Flush()
    {
        if (InstanceCounter == 0)
            return;

        // Store chunk size metadata in the beginning
        static_assert(InstancesPerChunk * InstanceSizeApprox * 2 < MAX_uint16, "Too much data for potential chunk storage.");
        auto meta = (FoliageChunkMeta*)Memory.GetHandle();
        meta->InstanceCounter = (uint16)InstanceCounter;
        InstanceCounter = 0;

        // Compress with LZ4
        const int32 srcSize = (int32)Memory.GetPosition();
        const int32 maxSize = LZ4_compressBound(srcSize);
        Compressed.Resize(maxSize + sizeof(CompressedSize)); // Place decompressed size in the beginning
        const int32 dstSize = LZ4_compress_default((const char*)Memory.GetHandle(), (char*)Compressed.Get() + sizeof(CompressedSize), srcSize, maxSize);
        Compressed.Resize(dstSize + sizeof(CompressedSize));
        *(CompressedSize*)Compressed.Get() = (CompressedSize)srcSize;

        // Convert raw bytes into Base64 string and write to Json stream
        Encryption::Base64Encode(Compressed.Get(), Compressed.Count(), Base64);
        Stream.String(Base64.Get(), Base64.Count());

        // Reset memory writer
        Memory.Reset();
        Memory.Move<FoliageChunkMeta>();
    }
};

struct FoliageReader
{
    typedef FoliageWriter::CompressedSize CompressedSize;
    Array<byte> Base64;
    Array<byte> Decompressed;

    FoliageReader()
    {
    }

    void Read(Foliage& foliage, int32& instanceIndex, StringAnsiView base64)
    {
        // Convert Base64 string into raw bytes
        Encryption::Base64Decode(base64.Get(), base64.Length(), Base64);

        // Decompress with LZ4
        auto originalSize = *(CompressedSize*)Base64.Get();
        Decompressed.Resize(originalSize);
        const int32 res = LZ4_decompress_safe((const char*)Base64.Get() + sizeof(CompressedSize), (char*)Decompressed.Get(), Base64.Count() - sizeof(CompressedSize), originalSize);
        ASSERT(res >= 0);
        Decompressed.Resize(res);

        // Read instances from memory
        MemoryReadStream memory(Decompressed);
        FoliageChunkMeta meta;
        memory.Read(meta);
        FoliageInstanceData data;
        ASSERT(meta.InstanceCounter <= FoliageWriter::InstancesPerChunk);
        for (int32 i = 0; i < meta.InstanceCounter; i++)
        {
            memory.Read(data);

            auto& instance = foliage.Instances[instanceIndex++];
            instance.Type = data.Type;
            instance.Random = data.Random;
            instance.Transform.Translation = data.Position;
            instance.Transform.Scale = data.Scale;
            Quaternion q;
            q.X = (float)data.OrientationX * (1.0f / (float)MAX_int16);
            q.Y = (float)data.OrientationY * (1.0f / (float)MAX_int16);
            q.Z = (float)data.OrientationZ * (1.0f / (float)MAX_int16);
            q.W = Math::Sqrt(Math::Max(1.0f - q.X * q.X - q.Y * q.Y - q.Z * q.Z, 0.0f));
            if (data.OrientationNegativeW)
                q.W *= -1;
            q.Normalize();
            instance.Transform.Orientation = q;
            if (data.HasLightmap)
            {
                memory.Read(instance.LightmapTextureIndex);
                memory.Read(instance.LightmapUVsArea);
            }
        }
    }
};

void Foliage::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Foliage);
    if (FoliageTypes.IsEmpty())
        return;
    PROFILE_CPU();
    PROFILE_MEM(LevelFoliage);

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

    // Put some metadata
    stream.Int(Instances.Count());

    // Write instances in chunks
    FoliageWriter writer(stream);
    // TODO: run this in parallel for better performance on large foliage data (keep the order of instances in the stream, run job for each 64 instances, let one job keep writing results back to the stream to avoid too much memory usage)
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        writer.Write(*i);
    }
    writer.Flush();

    stream.EndArray();
}

void Foliage::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    PROFILE_CPU();
    PROFILE_MEM(LevelFoliage);

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
        foliageTypesCount = foliageTypesMember->value.Size();
    if (foliageTypesCount)
    {
        PROFILE_CPU_NAMED("Types");
        const DeserializeStream& items = foliageTypesMember->value;;
        FoliageTypes.Resize(foliageTypesCount, false);
        for (int32 i = 0; i < foliageTypesCount; i++)
        {
            auto& type = FoliageTypes[i];
            type.Foliage = this;
            type.Index = i;
            type.Deserialize((DeserializeStream&)items[i], modifier);
        }
    }

    // Skip if no foliage
    if (FoliageTypes.IsEmpty())
        return;

    // Deserialize foliage instances
    const auto& foliageInstancesMember = stream.FindMember("Instances");
    if (foliageInstancesMember == stream.MemberEnd() || !foliageInstancesMember->value.IsArray())
        return;
    if (modifier->EngineBuild >= 7001)
    {
        const DeserializeStream& items = foliageInstancesMember->value;
        int32 chunksCount = (int32)items.Size() - 1;
        if (chunksCount <= 0)
            return;
        int32 foliageInstancesCount = items[0].GetInt();
        Instances.Resize(foliageInstancesCount);
        PROFILE_CPU_NAMED("Instances");
        
        FoliageReader reader;
        int32 instanceIndex = 0;
        for (int32 i = 1; i <= chunksCount; i++)
        {
            reader.Read(*this, instanceIndex, items[i].GetStringAnsiView());
        }
    }
    else
    {
        // [Deprecated in v1.13]
        MARK_CONTENT_DEPRECATED();
        int32 foliageInstancesCount = foliageInstancesMember->value.Size();
        if (foliageInstancesCount)
        {
            const DeserializeStream& items = foliageInstancesMember->value;
            Instances.Resize(foliageInstancesCount);

            if (modifier->EngineBuild <= 6189)
            {
                // [Deprecated on 30.11.2019, expires on 30.11.2021]
                MARK_CONTENT_DEPRECATED();
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
                    instance.Transform.Translation = enc.Translation;
                    instance.Transform.Orientation = enc.Orientation;
                    instance.Transform.Scale = enc.Scale;
                    instance.LightmapTextureIndex = -1;
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
                    instance.Transform.Translation = enc.Translation;
                    instance.Transform.Orientation = enc.Orientation;
                    instance.Transform.Scale = enc.Scale;
                    instance.LightmapTextureIndex = enc.LightmapTextureIndex;
                    instance.LightmapUVsArea = Half4(enc.LightmapUVsArea);
                }
            }
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

void Foliage::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

void Foliage::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);

    // Base
    Actor::OnEnable();
}

void Foliage::OnDisable()
{
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void Foliage::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    PROFILE_CPU();

    UpdateBounds();
    RebuildClusters();
}
