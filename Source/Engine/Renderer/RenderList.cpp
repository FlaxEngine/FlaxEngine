// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "RenderList.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Graphics/Materials/IMaterial.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/PostProcessEffect.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/PostFxVolume.h"

static_assert(sizeof(DrawCall) <= 288, "Too big draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Terrain), "Wrong draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Particle), "Wrong draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Custom), "Wrong draw call data size.");

namespace
{
    // Cached data for the draw calls sorting
    Array<uint64> SortingKeys[2];
    Array<int32> SortingIndices;
    Array<RenderList*> FreeRenderList;

    struct MemPoolEntry
    {
        void* Ptr;
        uintptr Size;
    };

    Array<MemPoolEntry> MemPool;
    CriticalSection MemPoolLocker;
}

void RendererDirectionalLightData::SetupLightData(LightData* data, bool useShadow) const
{
    data->SpotAngles.X = -2.0f;
    data->SpotAngles.Y = 1.0f;
    data->SourceRadius = 0;
    data->SourceLength = 0;
    data->Color = Color;
    data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data->Position = Float3::Zero;
    data->CastShadows = useShadow ? 1.0f : 0.0f;
    data->Direction = -Direction;
    data->Radius = 0;
    data->FalloffExponent = 0;
    data->InverseSquared = 0;
    data->RadiusInv = 0;
}

void RendererSpotLightData::SetupLightData(LightData* data, bool useShadow) const
{
    data->SpotAngles.X = CosOuterCone;
    data->SpotAngles.Y = InvCosConeDifference;
    data->SourceRadius = SourceRadius;
    data->SourceLength = 0.0f;
    data->Color = Color;
    data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data->Position = Position;
    data->CastShadows = useShadow ? 1.0f : 0.0f;
    data->Direction = Direction;
    data->Radius = Radius;
    data->FalloffExponent = FallOffExponent;
    data->InverseSquared = UseInverseSquaredFalloff ? 1.0f : 0.0f;
    data->RadiusInv = 1.0f / Radius;
}

void RendererPointLightData::SetupLightData(LightData* data, bool useShadow) const
{
    data->SpotAngles.X = -2.0f;
    data->SpotAngles.Y = 1.0f;
    data->SourceRadius = SourceRadius;
    data->SourceLength = SourceLength;
    data->Color = Color;
    data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data->Position = Position;
    data->CastShadows = useShadow ? 1.0f : 0.0f;
    data->Direction = Direction;
    data->Radius = Radius;
    data->FalloffExponent = FallOffExponent;
    data->InverseSquared = UseInverseSquaredFalloff ? 1.0f : 0.0f;
    data->RadiusInv = 1.0f / Radius;
}

void RendererSkyLightData::SetupLightData(LightData* data, bool useShadow) const
{
    data->SpotAngles.X = AdditiveColor.X;
    data->SpotAngles.Y = AdditiveColor.Y;
    data->SourceRadius = AdditiveColor.Z;
    data->SourceLength = Image ? Image->StreamingTexture()->TotalMipLevels() - 2.0f : 0.0f;
    data->Color = Color;
    data->MinRoughness = MIN_ROUGHNESS;
    data->Position = Position;
    data->CastShadows = useShadow ? 1.0f : 0.0f;
    data->Direction = Float3::Forward;
    data->Radius = Radius;
    data->FalloffExponent = 0;
    data->InverseSquared = 0;
    data->RadiusInv = 1.0f / Radius;
}

void* RendererAllocation::Allocate(uintptr size)
{
    void* result = nullptr;
    MemPoolLocker.Lock();
    for (int32 i = 0; i < MemPool.Count(); i++)
    {
        if (MemPool[i].Size == size)
        {
            result = MemPool[i].Ptr;
            MemPool.RemoveAt(i);
            break;
        }
    }
    MemPoolLocker.Unlock();
    if (!result)
    {
        result = Platform::Allocate(size, 16);
    }
    return result;
}

void RendererAllocation::Free(void* ptr, uintptr size)
{
    MemPoolLocker.Lock();
    MemPool.Add({ ptr, size });
    MemPoolLocker.Unlock();
}

RenderList* RenderList::GetFromPool()
{
    if (FreeRenderList.HasItems())
    {
        const auto result = FreeRenderList.Last();
        FreeRenderList.RemoveLast();
        return result;
    }

    return New<RenderList>();
}

void RenderList::ReturnToPool(RenderList* cache)
{
    if (!cache)
        return;

    ASSERT(!FreeRenderList.Contains(cache));
    FreeRenderList.Add(cache);
    cache->Clear();
}

void RenderList::CleanupCache()
{
    // Don't call it during rendering (data may be already in use)
    ASSERT(GPUDevice::Instance == nullptr || GPUDevice::Instance->CurrentTask == nullptr);

    SortingKeys[0].Resize(0);
    SortingKeys[1].Resize(0);
    SortingIndices.Resize(0);
    FreeRenderList.ClearDelete();
    for (auto& e : MemPool)
        Platform::Free(e.Ptr);
    MemPool.Clear();
}

bool RenderList::BlendableSettings::operator<(const BlendableSettings& other) const
{
    // Sort by higher priority
    if (Priority != other.Priority)
        return Priority < other.Priority;

    // Sort by lower size
    return other.VolumeSizeSqr < VolumeSizeSqr;
}

void RenderList::AddSettingsBlend(IPostFxSettingsProvider* provider, float weight, int32 priority, float volumeSizeSqr)
{
    BlendableSettings blend;
    blend.Provider = provider;
    blend.Weight = weight;
    blend.Priority = priority;
    blend.VolumeSizeSqr = volumeSizeSqr;
    Blendable.Add(blend);
}

void RenderList::BlendSettings()
{
    PROFILE_CPU();
    Sorting::QuickSort(Blendable);
    Settings = Graphics::PostProcessSettings;
    for (auto& b : Blendable)
    {
        b.Provider->Blend(Settings, b.Weight);
    }
}

void RenderList::RunPostFxPass(GPUContext* context, RenderContext& renderContext, MaterialPostFxLocation locationA, PostProcessEffectLocation locationB, GPUTexture*& inputOutput)
{
    // Note: during this stage engine is using additive rendering to the light buffer (given as inputOutput parameter).
    // Materials PostFx and Custom PostFx prefer sampling the input texture while rendering to the output.
    // So we need to allocate a temporary render target (or reuse from cache) and use it as a ping pong buffer.

    bool skipPass = true;
    bool needTempTarget = true;
    for (int32 i = 0; i < Settings.PostFxMaterials.Materials.Count(); i++)
    {
        const auto material = Settings.PostFxMaterials.Materials[i].Get();
        if (material && material->IsReady() && material->IsPostFx() && material->GetInfo().PostFxLocation == locationA)
        {
            skipPass = false;
            needTempTarget = true;
        }
    }
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::CustomPostProcess))
    {
        for (const PostProcessEffect* fx : renderContext.List->PostFx)
        {
            if (fx->Location == locationB)
            {
                skipPass = false;
                needTempTarget |= !fx->UseSingleTarget;
            }
        }
    }
    if (skipPass)
        return;

    auto tempDesc = inputOutput->GetDescription();
    auto temp = needTempTarget ? RenderTargetPool::Get(tempDesc) : nullptr;
    if (needTempTarget)
    {
        RENDER_TARGET_POOL_SET_NAME(temp, "RenderList.RunPostFxPassTemp");
    }

    auto input = inputOutput;
    auto output = temp;

    context->ResetRenderTarget();

    MaterialBase::BindParameters bindParams(context, renderContext);
    for (int32 i = 0; i < Settings.PostFxMaterials.Materials.Count(); i++)
    {
        auto material = Settings.PostFxMaterials.Materials[i].Get();
        if (material && material->IsReady() && material->IsPostFx() && material->GetInfo().PostFxLocation == locationA)
        {
            ASSERT(needTempTarget);
            context->SetRenderTarget(*output);
            bindParams.Input = *input;
            material->Bind(bindParams);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();
            Swap(output, input);
        }
    }
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::CustomPostProcess))
    {
        for (PostProcessEffect* fx : renderContext.List->PostFx)
        {
            if (fx->Location == locationB)
            {
                context->ResetSR();
                context->ResetUA();
                if (fx->UseSingleTarget || output == nullptr)
                {
                    fx->Render(context, renderContext, input, nullptr);
                }
                else
                {
                    ASSERT(needTempTarget);
                    fx->Render(context, renderContext, input, output);
                    Swap(input, output);
                }
                context->ResetRenderTarget();
            }
        }
    }

    inputOutput = input;

    if (needTempTarget)
        RenderTargetPool::Release(output);
}

void RenderList::RunMaterialPostFxPass(GPUContext* context, RenderContext& renderContext, MaterialPostFxLocation location, GPUTexture*& input, GPUTexture*& output)
{
    MaterialBase::BindParameters bindParams(context, renderContext);
    for (int32 i = 0; i < Settings.PostFxMaterials.Materials.Count(); i++)
    {
        auto material = Settings.PostFxMaterials.Materials[i].Get();
        if (material && material->IsReady() && material->IsPostFx() && material->GetInfo().PostFxLocation == location)
        {
            context->SetRenderTarget(*output);
            bindParams.Input = *input;
            material->Bind(bindParams);
            context->DrawFullscreenTriangle();
            Swap(output, input);
        }
        context->ResetRenderTarget();
    }
}

void RenderList::RunCustomPostFxPass(GPUContext* context, RenderContext& renderContext, PostProcessEffectLocation location, GPUTexture*& input, GPUTexture*& output)
{
    if (!(renderContext.View.Flags & ViewFlags::CustomPostProcess))
        return;
    for (PostProcessEffect* fx : renderContext.List->PostFx)
    {
        if (fx->Location == location)
        {
            if (fx->UseSingleTarget || output == nullptr)
            {
                fx->Render(context, renderContext, input, nullptr);
            }
            else
            {
                fx->Render(context, renderContext, input, output);
                Swap(input, output);
            }
            context->ResetRenderTarget();
        }
    }
}

bool RenderList::HasAnyPostFx(const RenderContext& renderContext, PostProcessEffectLocation postProcess) const
{
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::CustomPostProcess))
    {
        for (const PostProcessEffect* fx : renderContext.List->PostFx)
        {
            if (fx->Location == postProcess)
                return true;
        }
    }
    return false;
}

bool RenderList::HasAnyPostFx(const RenderContext& renderContext, MaterialPostFxLocation materialPostFx) const
{
    for (int32 i = 0; i < Settings.PostFxMaterials.Materials.Count(); i++)
    {
        auto material = Settings.PostFxMaterials.Materials[i].Get();
        if (material && material->IsReady() && material->IsPostFx() && material->GetInfo().PostFxLocation == materialPostFx)
        {
            return true;
        }
    }
    return false;
}

void DrawCallsList::Clear()
{
    Indices.Clear();
    PreBatchedDrawCalls.Clear();
    Batches.Clear();
    CanUseInstancing = true;
}

bool DrawCallsList::IsEmpty() const
{
    return Indices.Count() + PreBatchedDrawCalls.Count() == 0;
}

RenderList::RenderList(const SpawnParams& params)
    : ScriptingObject(params)
    , DirectionalLights(4)
    , PointLights(32)
    , SpotLights(32)
    , SkyLights(4)
    , EnvironmentProbes(32)
    , Decals(64)
    , Sky(nullptr)
    , AtmosphericFog(nullptr)
    , Fog(nullptr)
    , Blendable(32)
    , _instanceBuffer(1024 * sizeof(InstanceData), sizeof(InstanceData), TEXT("Instance Buffer"))
{
}

void RenderList::Init(RenderContext& renderContext)
{
    renderContext.View.Frustum.GetCorners(FrustumCornersWs);
    for (int32 i = 0; i < 8; i++)
        Float3::Transform(FrustumCornersWs[i], renderContext.View.View, FrustumCornersVs[i]);
}

void RenderList::Clear()
{
    Scenes.Clear();
    DrawCalls.Clear();
    BatchedDrawCalls.Clear();
    for (auto& list : DrawCallsLists)
        list.Clear();
    ShadowDepthDrawCallsList.Clear();
    PointLights.Clear();
    SpotLights.Clear();
    SkyLights.Clear();
    DirectionalLights.Clear();
    EnvironmentProbes.Clear();
    Decals.Clear();
    VolumetricFogParticles.Clear();
    Sky = nullptr;
    AtmosphericFog = nullptr;
    Fog = nullptr;
    PostFx.Clear();
    Settings = PostProcessSettings();
    Blendable.Clear();
    _instanceBuffer.Clear();
}

struct PackedSortKey
{
    union
    {
        uint64 Data;

        struct
        {
            uint32 DistanceKey;
            uint16 BatchKey;
            uint16 SortKey;
        };
    };
};

FORCE_INLINE void CalculateSortKey(const RenderContext& renderContext, DrawCall& drawCall, int16 sortOrder)
{
    const Float3 planeNormal = renderContext.View.Direction;
    const float planePoint = -Float3::Dot(planeNormal, renderContext.View.Position);
    const float distance = Float3::Dot(planeNormal, drawCall.ObjectPosition) - planePoint;
    PackedSortKey key;
    key.DistanceKey = RenderTools::ComputeDistanceSortKey(distance);
    uint32 batchKey = GetHash(drawCall.Material);
    batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[0]);
    batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[1]);
    batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[2]);
    batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.IndexBuffer);
    IMaterial::InstancingHandler handler;
    if (drawCall.Material->CanUseInstancing(handler))
        handler.GetHash(drawCall, batchKey);
    batchKey += (int32)(471 * drawCall.WorldDeterminantSign);
    key.SortKey = (uint16)(sortOrder - MIN_int16);
    key.BatchKey = (uint16)batchKey;
    drawCall.SortKey = key.Data;
}

void RenderList::AddDrawCall(const RenderContext& renderContext, DrawPass drawModes, StaticFlags staticFlags, DrawCall& drawCall, bool receivesDecals, int16 sortOrder)
{
#if ENABLE_ASSERTION_LOW_LAYERS
    // Ensure that draw modes are non-empty and in conjunction with material draw modes
    auto materialDrawModes = drawCall.Material->GetDrawModes();
    ASSERT_LOW_LAYER(drawModes != DrawPass::None && ((uint32)drawModes & ~(uint32)materialDrawModes) == 0);
#endif

    // Append draw call data
    CalculateSortKey(renderContext, drawCall, sortOrder);
    const int32 index = DrawCalls.Add(drawCall);

    // Add draw call to proper draw lists
    if ((drawModes & DrawPass::Depth) != DrawPass::None)
    {
        DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
    }
    if ((drawModes & (DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas)) != DrawPass::None)
    {
        if (receivesDecals)
            DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
        else
            DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
    }
    if ((drawModes & DrawPass::Forward) != DrawPass::None)
    {
        DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
    }
    if ((drawModes & DrawPass::Distortion) != DrawPass::None)
    {
        DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
    }
    if ((drawModes & DrawPass::MotionVectors) != DrawPass::None && (staticFlags & StaticFlags::Transform) == StaticFlags::None)
    {
        DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
    }
}

void RenderList::AddDrawCall(const RenderContextBatch& renderContextBatch, DrawPass drawModes, StaticFlags staticFlags, ShadowsCastingMode shadowsMode, const BoundingSphere& bounds, DrawCall& drawCall, bool receivesDecals, int16 sortOrder)
{
#if ENABLE_ASSERTION_LOW_LAYERS
    // Ensure that draw modes are non-empty and in conjunction with material draw modes
    auto materialDrawModes = drawCall.Material->GetDrawModes();
    ASSERT_LOW_LAYER(drawModes != DrawPass::None && ((uint32)drawModes & ~(uint32)materialDrawModes) == 0);
#endif
    const RenderContext& mainRenderContext = renderContextBatch.Contexts.Get()[0];

    // Append draw call data
    CalculateSortKey(mainRenderContext, drawCall, sortOrder);
    const int32 index = DrawCalls.Add(drawCall);

    // Add draw call to proper draw lists
    DrawPass modes = drawModes & mainRenderContext.View.GetShadowsDrawPassMask(shadowsMode);
    drawModes = modes & mainRenderContext.View.Pass;
    if (drawModes != DrawPass::None && mainRenderContext.View.CullingFrustum.Intersects(bounds))
    {
        if ((drawModes & DrawPass::Depth) != DrawPass::None)
        {
            DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
        }
        if ((drawModes & (DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas)) != DrawPass::None)
        {
            if (receivesDecals)
                DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
            else
                DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
        }
        if ((drawModes & DrawPass::Forward) != DrawPass::None)
        {
            DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
        }
        if ((drawModes & DrawPass::Distortion) != DrawPass::None)
        {
            DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
        }
        if ((drawModes & DrawPass::MotionVectors) != DrawPass::None && (staticFlags & StaticFlags::Transform) == StaticFlags::None)
        {
            DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
        }
    }
    for (int32 i = 1; i < renderContextBatch.Contexts.Count(); i++)
    {
        const RenderContext& renderContext = renderContextBatch.Contexts.Get()[i];
        ASSERT_LOW_LAYER(renderContext.View.Pass == DrawPass::Depth);
        drawModes = modes & renderContext.View.Pass;
        if (drawModes != DrawPass::None && renderContext.View.CullingFrustum.Intersects(bounds))
        {
            renderContext.List->ShadowDepthDrawCallsList.Indices.Add(index);
        }
    }
}

namespace
{
    /// <summary>
    /// Checks if this draw call be batched together with the other one.
    /// </summary>
    /// <param name="a">The first draw call.</param>
    /// <param name="b">The second draw call.</param>
    /// <returns>True if can merge them, otherwise false.</returns>
    FORCE_INLINE bool CanBatchWith(const DrawCall& a, const DrawCall& b)
    {
        IMaterial::InstancingHandler handler;
        return a.Material == b.Material &&
                a.Material->CanUseInstancing(handler) &&
                Platform::MemoryCompare(&a.Geometry, &b.Geometry, sizeof(a.Geometry)) == 0 &&
                a.InstanceCount != 0 &&
                b.InstanceCount != 0 &&
                handler.CanBatch(a, b) &&
                a.WorldDeterminantSign * b.WorldDeterminantSign > 0;
    }
}

void RenderList::SortDrawCalls(const RenderContext& renderContext, bool reverseDistance, DrawCallsList& list, const RenderListBuffer<DrawCall>& drawCalls)
{
    PROFILE_CPU();
    const auto* drawCallsData = drawCalls.Get();
    const auto* listData = list.Indices.Get();
    const int32 listSize = list.Indices.Count();

    // Peek shared memory
#define PREPARE_CACHE(list) (list).Clear(); (list).Resize(listSize)
    PREPARE_CACHE(SortingKeys[0]);
    PREPARE_CACHE(SortingKeys[1]);
    PREPARE_CACHE(SortingIndices);
#undef PREPARE_CACHE
    uint64* sortedKeys = SortingKeys[0].Get();

    // Setup sort keys
    if (reverseDistance)
    {
        for (int32 i = 0; i < listSize; i++)
        {
            const DrawCall& drawCall = drawCallsData[listData[i]];
            PackedSortKey key;
            key.Data = drawCall.SortKey;
            key.DistanceKey ^= MAX_uint32; // Reverse depth
            key.SortKey ^= MAX_uint16; // Reverse sort order
            sortedKeys[i] = key.Data;
        }
    }
    else
    {
        for (int32 i = 0; i < listSize; i++)
        {
            const DrawCall& drawCall = drawCallsData[listData[i]];
            sortedKeys[i] = drawCall.SortKey;
        }
    }

    // Sort draw calls indices
    int32* resultIndices = list.Indices.Get();
    Sorting::RadixSort(sortedKeys, resultIndices, SortingKeys[1].Get(), SortingIndices.Get(), listSize);
    if (resultIndices != list.Indices.Get())
        Platform::MemoryCopy(list.Indices.Get(), resultIndices, sizeof(int32) * listSize);

    // Perform draw calls batching
    list.Batches.Clear();
    for (int32 i = 0; i < listSize;)
    {
        const DrawCall& drawCall = drawCallsData[listData[i]];
        int32 batchSize = 1;
        int32 instanceCount = drawCall.InstanceCount;

        // Check the following draw calls to merge them (using instancing)
        for (int32 j = i + 1; j < listSize; j++)
        {
            const DrawCall& other = drawCallsData[listData[j]];
            if (!CanBatchWith(drawCall, other))
                break;
            batchSize++;
            instanceCount += other.InstanceCount;
        }

        DrawBatch batch;
        batch.SortKey = sortedKeys[i];
        batch.StartIndex = i;
        batch.BatchSize = batchSize;
        batch.InstanceCount = instanceCount;
        list.Batches.Add(batch);

        i += batchSize;
    }

    // Sort draw calls batches by depth
    Sorting::QuickSort(list.Batches);
}

FORCE_INLINE bool CanUseInstancing(DrawPass pass)
{
    return pass == DrawPass::GBuffer || pass == DrawPass::Depth;
}

void RenderList::ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list, const RenderListBuffer<DrawCall>& drawCalls, GPUTextureView* input)
{
    if (list.IsEmpty())
        return;
    PROFILE_GPU_CPU("Drawing");
    const auto* drawCallsData = drawCalls.Get();
    const auto* listData = list.Indices.Get();
    const auto* batchesData = list.Batches.Get();
    const auto context = GPUDevice::Instance->GetMainContext();
    bool useInstancing = list.CanUseInstancing && CanUseInstancing(renderContext.View.Pass) && GPUDevice::Instance->Limits.HasInstancing;
    TaaJitterRemoveContext taaJitterRemove(renderContext.View);

    // Clear SR slots to prevent any resources binding issues (leftovers from the previous passes)
    context->ResetSR();

    // Prepare instance buffer
    if (useInstancing)
    {
        // Prepare buffer memory
        int32 instancedBatchesCount = 0;
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            auto& batch = batchesData[i];
            if (batch.BatchSize > 1)
                instancedBatchesCount += batch.BatchSize;
        }
        for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
        {
            auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            if (batch.Instances.Count() > 1)
                instancedBatchesCount += batch.Instances.Count();
        }
        if (instancedBatchesCount == 0)
        {
            // Faster path if none of the draw batches requires instancing
            useInstancing = false;
            goto DRAW;
        }
        _instanceBuffer.Clear();
        _instanceBuffer.Data.Resize(instancedBatchesCount * sizeof(InstanceData));
        auto instanceData = (InstanceData*)_instanceBuffer.Data.Get();

        // Write to instance buffer
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            auto& batch = batchesData[i];
            if (batch.BatchSize > 1)
            {
                IMaterial::InstancingHandler handler;
                drawCallsData[listData[batch.StartIndex]].Material->CanUseInstancing(handler);
                for (int32 j = 0; j < batch.BatchSize; j++)
                {
                    auto& drawCall = drawCallsData[listData[batch.StartIndex + j]];
                    handler.WriteDrawCall(instanceData, drawCall);
                    instanceData++;
                }
            }
        }
        for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
        {
            auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            if (batch.Instances.Count() > 1)
            {
                Platform::MemoryCopy(instanceData, batch.Instances.Get(), batch.Instances.Count() * sizeof(InstanceData));
                instanceData += batch.Instances.Count();
            }
        }

        // Upload data
        _instanceBuffer.Flush(context);
    }

DRAW:

    // Execute draw calls
    MaterialBase::BindParameters bindParams(context, renderContext);
    bindParams.Input = input;
    bindParams.BindViewData();
    if (useInstancing)
    {
        int32 instanceBufferOffset = 0;
        GPUBuffer* vb[4];
        uint32 vbOffsets[4];
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            auto& batch = batchesData[i];
            const DrawCall& drawCall = drawCallsData[listData[batch.StartIndex]];

            int32 vbCount = 0;
            while (vbCount < ARRAY_COUNT(drawCall.Geometry.VertexBuffers) && drawCall.Geometry.VertexBuffers[vbCount])
            {
                vb[vbCount] = drawCall.Geometry.VertexBuffers[vbCount];
                vbOffsets[vbCount] = drawCall.Geometry.VertexBuffersOffsets[vbCount];
                vbCount++;
            }
            for (int32 j = vbCount; j < ARRAY_COUNT(drawCall.Geometry.VertexBuffers); j++)
            {
                vb[vbCount] = nullptr;
                vbOffsets[vbCount] = 0;
            }

            bindParams.FirstDrawCall = &drawCall;
            bindParams.DrawCallsCount = batch.BatchSize;
            drawCall.Material->Bind(bindParams);

            context->BindIB(drawCall.Geometry.IndexBuffer);

            if (drawCall.InstanceCount == 0)
            {
                // No support for batching indirect draw calls
                ASSERT_LOW_LAYER(batch.BatchSize == 1);

                context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
            }
            else
            {
                if (batch.BatchSize == 1)
                {
                    context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                }
                else
                {
                    vbCount = 3;
                    vb[vbCount] = _instanceBuffer.GetBuffer();
                    vbOffsets[vbCount] = 0;
                    vbCount++;
                    context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.InstanceCount, instanceBufferOffset, 0, drawCall.Draw.StartIndex);
                    instanceBufferOffset += batch.BatchSize;
                }
            }
        }
        for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
        {
            auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            auto& drawCall = batch.DrawCall;

            int32 vbCount = 0;
            while (vbCount < ARRAY_COUNT(drawCall.Geometry.VertexBuffers) && drawCall.Geometry.VertexBuffers[vbCount])
            {
                vb[vbCount] = drawCall.Geometry.VertexBuffers[vbCount];
                vbOffsets[vbCount] = drawCall.Geometry.VertexBuffersOffsets[vbCount];
                vbCount++;
            }
            for (int32 j = vbCount; j < ARRAY_COUNT(drawCall.Geometry.VertexBuffers); j++)
            {
                vb[vbCount] = nullptr;
                vbOffsets[vbCount] = 0;
            }

            bindParams.FirstDrawCall = &drawCall;
            bindParams.DrawCallsCount = batch.Instances.Count();
            drawCall.Material->Bind(bindParams);

            context->BindIB(drawCall.Geometry.IndexBuffer);

            if (drawCall.InstanceCount == 0)
            {
                ASSERT_LOW_LAYER(batch.Instances.Count() == 1);
                context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
            }
            else
            {
                if (batch.Instances.Count() == 1)
                {
                    context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.Instances.Count(), 0, 0, drawCall.Draw.StartIndex);
                }
                else
                {
                    vbCount = 3;
                    vb[vbCount] = _instanceBuffer.GetBuffer();
                    vbOffsets[vbCount] = 0;
                    vbCount++;
                    context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.Instances.Count(), instanceBufferOffset, 0, drawCall.Draw.StartIndex);
                    instanceBufferOffset += batch.Instances.Count();
                }
            }
        }
    }
    else
    {
        bindParams.DrawCallsCount = 1;
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            auto& batch = batchesData[i];

            for (int32 j = 0; j < batch.BatchSize; j++)
            {
                const DrawCall& drawCall = drawCalls[listData[batch.StartIndex + j]];
                bindParams.FirstDrawCall = &drawCall;
                drawCall.Material->Bind(bindParams);

                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3), drawCall.Geometry.VertexBuffersOffsets);

                if (drawCall.InstanceCount == 0)
                {
                    context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                }
                else
                {
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                }
            }
        }
        for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
        {
            auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            auto drawCall = batch.DrawCall;
            drawCall.ObjectRadius = 0.0f;
            bindParams.FirstDrawCall = &drawCall;
            const auto* instancesData = batch.Instances.Get();

            for (int32 j = 0; j < batch.Instances.Count(); j++)
            {
                auto& instance = instancesData[j];
                drawCall.ObjectPosition = instance.InstanceOrigin;
                drawCall.PerInstanceRandom = instance.PerInstanceRandom;
                auto lightmapArea = instance.InstanceLightmapArea.ToFloat4();
                drawCall.Surface.LightmapUVsArea = *(Rectangle*)&lightmapArea;
                drawCall.Surface.LODDitherFactor = instance.LODDitherFactor;
                drawCall.World.SetRow1(Float4(instance.InstanceTransform1, 0.0f));
                drawCall.World.SetRow2(Float4(instance.InstanceTransform2, 0.0f));
                drawCall.World.SetRow3(Float4(instance.InstanceTransform3, 0.0f));
                drawCall.World.SetRow4(Float4(instance.InstanceOrigin, 1.0f));
                drawCall.Material->Bind(bindParams);

                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3), drawCall.Geometry.VertexBuffersOffsets);
                context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
            }
        }
        if (list.Batches.IsEmpty() && list.Indices.Count() != 0)
        {
            // Draw calls list has nto been batched so execute draw calls separately
            for (int32 j = 0; j < list.Indices.Count(); j++)
            {
                const DrawCall& drawCall = drawCalls[listData[j]];
                bindParams.FirstDrawCall = &drawCall;
                drawCall.Material->Bind(bindParams);

                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3), drawCall.Geometry.VertexBuffersOffsets);

                if (drawCall.InstanceCount == 0)
                {
                    context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                }
                else
                {
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                }
            }
        }
    }
}

void SurfaceDrawCallHandler::GetHash(const DrawCall& drawCall, uint32& batchKey)
{
    batchKey = (batchKey * 397) ^ ::GetHash(drawCall.Surface.Lightmap);
}

bool SurfaceDrawCallHandler::CanBatch(const DrawCall& a, const DrawCall& b)
{
    // TODO: find reason why batching static meshes with lightmap causes problems with sampling in shader (flickering when meshes in batch order gets changes due to async draw calls collection)
    return a.Surface.Lightmap == nullptr && b.Surface.Lightmap == nullptr &&
            //return a.Surface.Lightmap == b.Surface.Lightmap &&
            a.Surface.Skinning == nullptr &&
            b.Surface.Skinning == nullptr;
}

void SurfaceDrawCallHandler::WriteDrawCall(InstanceData* instanceData, const DrawCall& drawCall)
{
    instanceData->InstanceOrigin = Float3(drawCall.World.M41, drawCall.World.M42, drawCall.World.M43);
    instanceData->PerInstanceRandom = drawCall.PerInstanceRandom;
    instanceData->InstanceTransform1 = Float3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13);
    instanceData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
    instanceData->InstanceTransform2 = Float3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23);
    instanceData->InstanceTransform3 = Float3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33);
    instanceData->InstanceLightmapArea = Half4(drawCall.Surface.LightmapUVsArea);
}
