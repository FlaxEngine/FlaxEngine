// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "RenderList.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Graphics/Materials/IMaterial.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/PostProcessBase.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/PostFxVolume.h"

// Amount of bits to use for draw calls batches hash key
#define USE_BATCH_KEY_MASK 0
#define BATCH_KEY_BITS 32
#define BATCH_KEY_MASK ((1 << BATCH_KEY_BITS) - 1)

static_assert(sizeof(DrawCall) <= 280, "Too big draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Terrain), "Wrong draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Particle), "Wrong draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Custom), "Wrong draw call data size.");

namespace
{
    // Cached data for the draw calls sorting
    Array<uint64> SortingKeys[2];
    Array<int32> SortingIndices;
    Array<RenderList*> FreeRenderList;
}

#define PREPARE_CACHE(list) (list).Clear(); (list).Resize(listSize)

void RendererDirectionalLightData::SetupLightData(LightData* data, const RenderView& view, bool useShadow) const
{
    data->SpotAngles.X = -2.0f;
    data->SpotAngles.Y = 1.0f;
    data->SourceRadius = 0;
    data->SourceLength = 0;
    data->Color = Color;
    data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data->Position = Vector3::Zero;
    data->CastShadows = useShadow ? 1.0f : 0.0f;
    data->Direction = -Direction;
    data->Radius = 0;
    data->FalloffExponent = 0;
    data->InverseSquared = 0;
    data->RadiusInv = 0;
}

void RendererSpotLightData::SetupLightData(LightData* data, const RenderView& view, bool useShadow) const
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

void RendererPointLightData::SetupLightData(LightData* data, const RenderView& view, bool useShadow) const
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

void RendererSkyLightData::SetupLightData(LightData* data, const RenderView& view, bool useShadow) const
{
    data->SpotAngles.X = AdditiveColor.X;
    data->SpotAngles.Y = AdditiveColor.Y;
    data->SourceRadius = AdditiveColor.Z;
    data->SourceLength = Image ? Image->StreamingTexture()->TotalMipLevels() - 2.0f : 0.0f;
    data->Color = Color;
    data->MinRoughness = MIN_ROUGHNESS;
    data->Position = Position;
    data->CastShadows = useShadow ? 1.0f : 0.0f;
    data->Direction = Vector3::Forward;
    data->Radius = Radius;
    data->FalloffExponent = 0;
    data->InverseSquared = 0;
    data->RadiusInv = 1.0f / Radius;
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

    Sorting::QuickSort(Blendable.Get(), Blendable.Count());

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
    if (renderContext.View.Flags & ViewFlags::CustomPostProcess)
    {
        for (int32 i = 0; i < renderContext.List->PostFx.Count(); i++)
        {
            const auto fx = renderContext.List->PostFx[i];
            if (fx->IsReady() && fx->GetLocation() == locationB)
            {
                skipPass = false;
                needTempTarget |= !fx->GetUseSingleTarget();
            }
        }
    }
    if (skipPass)
        return;

    auto tempDesc = inputOutput->GetDescription();
    auto temp = needTempTarget ? RenderTargetPool::Get(tempDesc) : nullptr;

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
    if (renderContext.View.Flags & ViewFlags::CustomPostProcess)
    {
        for (int32 i = 0; i < renderContext.List->PostFx.Count(); i++)
        {
            auto fx = renderContext.List->PostFx[i];
            if (fx->IsReady() && fx->GetLocation() == locationB)
            {
                if (fx->GetUseSingleTarget())
                {
                    fx->Render(renderContext, input, nullptr);
                }
                else
                {
                    ASSERT(needTempTarget);
                    fx->Render(renderContext, input, output);
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
            context->ResetRenderTarget();
            Swap(output, input);
        }
    }
}

void RenderList::RunCustomPostFxPass(GPUContext* context, RenderContext& renderContext, PostProcessEffectLocation location, GPUTexture*& input, GPUTexture*& output)
{
    if (!(renderContext.View.Flags & ViewFlags::CustomPostProcess))
        return;

    for (int32 i = 0; i < renderContext.List->PostFx.Count(); i++)
    {
        auto fx = renderContext.List->PostFx[i];
        if (fx->IsReady() && fx->GetLocation() == location)
        {
            if (fx->GetUseSingleTarget())
            {
                fx->Render(renderContext, input, nullptr);
            }
            else
            {
                fx->Render(renderContext, input, output);
                Swap(input, output);
            }
            context->ResetRenderTarget();
        }
    }
}

bool RenderList::HasAnyPostAA(RenderContext& renderContext) const
{
    for (int32 i = 0; i < Settings.PostFxMaterials.Materials.Count(); i++)
    {
        auto material = Settings.PostFxMaterials.Materials[i].Get();
        if (material && material->IsReady() && material->IsPostFx() && material->GetInfo().PostFxLocation == MaterialPostFxLocation::AfterAntiAliasingPass)
        {
            return true;
        }
    }
    if (renderContext.View.Flags & ViewFlags::CustomPostProcess)
    {
        for (int32 i = 0; i < renderContext.List->PostFx.Count(); i++)
        {
            auto fx = renderContext.List->PostFx[i];
            if (fx->IsReady() && fx->GetLocation() == PostProcessEffectLocation::AfterAntiAliasingPass)
            {
                return true;
            }
        }
    }

    return false;
}

RenderList::RenderList(const SpawnParams& params)
    : PersistentScriptingObject(params)
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
    Vector3::Transform(FrustumCornersWs, renderContext.View.View, FrustumCornersVs, 8);
}

void RenderList::Clear()
{
    DrawCalls.Clear();
    for (auto& list : DrawCallsLists)
        list.Clear();
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

void RenderList::AddDrawCall(DrawPass drawModes, StaticFlags staticFlags, DrawCall& drawCall, bool receivesDecals)
{
    ASSERT_LOW_LAYER(drawCall.Geometry.IndexBuffer);

    // Mix object mask with material mask
    const auto mask = (DrawPass)(drawModes & drawCall.Material->GetDrawModes());
    if (mask == DrawPass::None)
        return;

    // Append draw call data
    const int32 index = DrawCalls.Count();
    DrawCalls.Add(drawCall);

    // Add draw call to proper draw lists
    if (mask & DrawPass::Depth)
    {
        DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
    }
    if (mask & DrawPass::GBuffer)
    {
        if (receivesDecals)
            DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
        else
            DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
    }
    if (mask & DrawPass::Forward)
    {
        DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
    }
    if (mask & DrawPass::Distortion)
    {
        DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
    }
    if (mask & DrawPass::MotionVectors && (staticFlags & StaticFlags::Transform) == 0)
    {
        DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
    }
}

uint32 ComputeDistance(float distance)
{
    // Compute sort key (http://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/)
    uint32 distanceI = *((uint32*)&distance);
    return ((uint32)(-(int32)(distanceI >> 31)) | 0x80000000) ^ distanceI;
}

/// <summary>
/// Sorts the linear data array using Radix Sort algorithm (uses temporary keys collection).
/// </summary>
/// <param name="inputKeys">The data pointer to the input sorting keys array. When this method completes it contains a pointer to the original data or the temporary depending on the algorithm passes count. Use it as a results container.</param>
/// <param name="inputValues">The data pointer to the input values array. When this method completes it contains a pointer to the original data or the temporary depending on the algorithm passes count. Use it as a results container.</param>
/// <param name="tmpKeys">The data pointer to the temporary sorting keys array.</param>
/// <param name="tmpValues">The data pointer to the temporary values array.</param>
/// <param name="count">The elements count.</param>
template<typename T, typename U>
static void RadixSort(T*& inputKeys, U* inputValues, T* tmpKeys, U* tmpValues, int32 count)
{
    // Based on: https://github.com/bkaradzic/bx/blob/master/include/bx/inline/sort.inl
    enum
    {
        RADIXSORT_BITS = 11,
        RADIXSORT_HISTOGRAM_SIZE = 1 << RADIXSORT_BITS,
        RADIXSORT_BIT_MASK = RADIXSORT_HISTOGRAM_SIZE - 1
    };

    if (count < 2)
        return;

    T* keys = inputKeys;
    T* tempKeys = tmpKeys;
    U* values = inputValues;
    U* tempValues = tmpValues;

    uint32 histogram[RADIXSORT_HISTOGRAM_SIZE];
    uint16 shift = 0;
    int32 pass = 0;
    for (; pass < 6; pass++)
    {
        Platform::MemoryClear(histogram, sizeof(uint32) * RADIXSORT_HISTOGRAM_SIZE);

        bool sorted = true;
        T key = keys[0];
        T prevKey = key;
        for (int32 i = 0; i < count; i++)
        {
            key = keys[i];
            const uint16 index = (key >> shift) & RADIXSORT_BIT_MASK;
            ++histogram[index];
            sorted &= prevKey <= key;
            prevKey = key;
        }

        if (sorted)
        {
            goto end;
        }

        uint32 offset = 0;
        for (int32 i = 0; i < RADIXSORT_HISTOGRAM_SIZE; ++i)
        {
            const uint32 cnt = histogram[i];
            histogram[i] = offset;
            offset += cnt;
        }

        for (int32 i = 0; i < count; i++)
        {
            const T k = keys[i];
            const uint16 index = (k >> shift) & RADIXSORT_BIT_MASK;
            const uint32 dest = histogram[index]++;
            tempKeys[dest] = k;
            tempValues[dest] = values[i];
        }

        T* const swapKeys = tempKeys;
        tempKeys = keys;
        keys = swapKeys;

        U* const swapValues = tempValues;
        tempValues = values;
        values = swapValues;

        shift += RADIXSORT_BITS;
    }

end:
    if (pass & 1)
    {
        // Use temporary keys as a result
        inputKeys = tmpKeys;

#if 0
        // Use temporary values as a result
        inputValues = tmpValues;
#else
        // Odd number of passes needs to do copy to the destination
        Platform::MemoryCopy(inputValues, tmpValues, sizeof(U) * count);
#endif
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
                a.WorldDeterminantSign == b.WorldDeterminantSign;
    }
}

void RenderList::SortDrawCalls(const RenderContext& renderContext, bool reverseDistance, DrawCallsList& list)
{
    PROFILE_CPU();

    const int32 listSize = (int32)list.Indices.Count();
    const Plane plane(renderContext.View.Position, renderContext.View.Direction);

    // Peek shared memory
    PREPARE_CACHE(SortingKeys[0]);
    PREPARE_CACHE(SortingKeys[1]);
    PREPARE_CACHE(SortingIndices);
    uint64* sortedKeys = SortingKeys[0].Get();

    // Generate sort keys (by depth) and batch keys (higher bits)
    const uint32 sortKeyXor = reverseDistance ? MAX_uint32 : 0;
    for (int32 i = 0; i < listSize; i++)
    {
        auto& drawCall = DrawCalls[list.Indices[i]];
        const auto distance = CollisionsHelper::DistancePlanePoint(plane, drawCall.ObjectPosition);
        const uint32 sortKey = ComputeDistance(distance) ^ sortKeyXor;
        int32 batchKey = GetHash(drawCall.Geometry.IndexBuffer);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[0]);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[1]);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[2]);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Material);
        IMaterial::InstancingHandler handler;
        if (drawCall.Material->CanUseInstancing(handler))
            handler.GetHash(drawCall, batchKey);
        batchKey += (int32)(471 * drawCall.WorldDeterminantSign);
#if USE_BATCH_KEY_MASK
		const uint32 batchHashKey = (uint32)batchKey & BATCH_KEY_MASK;
#else
        const uint32 batchHashKey = (uint32)batchKey;
#endif
        sortedKeys[i] = (uint64)batchHashKey << 32 | (uint64)sortKey;
    }

    // Sort draw calls indices
    RadixSort(sortedKeys, list.Indices.Get(), SortingKeys[1].Get(), SortingIndices.Get(), listSize);

    // Perform draw calls batching
    list.Batches.Clear();
    for (int32 i = 0; i < listSize;)
    {
        const auto& drawCall = DrawCalls[list.Indices[i]];
        int32 batchSize = 1;
        int32 instanceCount = drawCall.InstanceCount;

        // Check the following draw calls to merge them (using instancing)
        for (int32 j = i + 1; j < listSize; j++)
        {
            const auto& other = DrawCalls[list.Indices[j]];
            if (!CanBatchWith(drawCall, other))
                break;

            batchSize++;
            instanceCount += other.InstanceCount;
        }

        DrawBatch batch;
        batch.SortKey = sortedKeys[i] & MAX_uint32;
        batch.StartIndex = i;
        batch.BatchSize = batchSize;
        batch.InstanceCount = instanceCount;
        list.Batches.Add(batch);

        i += batchSize;
    }

    // Sort draw calls batches by depth
    Sorting::QuickSort(list.Batches.Get(), list.Batches.Count());
}

bool CanUseInstancing(DrawPass pass)
{
    return pass == DrawPass::GBuffer || pass == DrawPass::Depth;
}

void RenderList::ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list)
{
    // Skip if no rendering to perform
    if (list.Batches.IsEmpty())
        return;

    PROFILE_GPU_CPU("Drawing");

    const int32 batchesSize = list.Batches.Count();
    const auto context = GPUDevice::Instance->GetMainContext();
    bool useInstancing = list.CanUseInstancing && CanUseInstancing(renderContext.View.Pass) && GPUDevice::Instance->Limits.HasInstancing;

    // Clear SR slots to prevent any resources binding issues (leftovers from the previous passes)
    context->ResetSR();

    // Prepare instance buffer
    if (useInstancing)
    {
        // Prepare buffer memory
        int32 batchesCount = 0;
        for (int32 i = 0; i < batchesSize; i++)
        {
            auto& batch = list.Batches[i];
            if (batch.BatchSize > 1)
            {
                batchesCount += batch.BatchSize;
            }
        }
        if (batchesCount == 0)
        {
            // Faster path if none of the draw batches requires instancing
            useInstancing = false;
            goto DRAW;
        }
        _instanceBuffer.Clear();
        _instanceBuffer.Data.Resize(batchesCount * sizeof(InstanceData));
        auto instanceData = (InstanceData*)_instanceBuffer.Data.Get();

        // Write to instance buffer
        for (int32 i = 0; i < batchesSize; i++)
        {
            auto& batch = list.Batches[i];
            if (batch.BatchSize > 1)
            {
                IMaterial::InstancingHandler handler;
                DrawCalls[list.Indices[batch.StartIndex]].Material->CanUseInstancing(handler);
                for (int32 j = 0; j < batch.BatchSize; j++)
                {
                    auto& drawCall = DrawCalls[list.Indices[batch.StartIndex + j]];
                    handler.WriteDrawCall(instanceData, drawCall);
                    instanceData++;
                }
            }
        }

        // Upload data
        _instanceBuffer.Flush(context);
    }

DRAW:

    // Execute draw calls
    MaterialBase::BindParameters bindParams(context, renderContext);
    if (useInstancing)
    {
        int32 instanceBufferOffset = 0;
        GPUBuffer* vb[4];
        uint32 vbOffsets[4];
        for (int32 i = 0; i < batchesSize; i++)
        {
            auto& batch = list.Batches[i];
            auto& drawCall = DrawCalls[list.Indices[batch.StartIndex]];

            int32 vbCount = 0;
            while (drawCall.Geometry.VertexBuffers[vbCount] && vbCount < ARRAY_COUNT(drawCall.Geometry.VertexBuffers))
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
                ASSERT(batch.BatchSize == 1);

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
    }
    else
    {
        bindParams.DrawCallsCount = 1;
        for (int32 i = 0; i < batchesSize; i++)
        {
            auto& batch = list.Batches[i];

            for (int32 j = 0; j < batch.BatchSize; j++)
            {
                auto& drawCall = DrawCalls[list.Indices[batch.StartIndex + j]];
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

void SurfaceDrawCallHandler::GetHash(const DrawCall& drawCall, int32& batchKey)
{
    batchKey = (batchKey * 397) ^ ::GetHash(drawCall.Surface.Lightmap);
}

bool SurfaceDrawCallHandler::CanBatch(const DrawCall& a, const DrawCall& b)
{
    return a.Surface.Lightmap == b.Surface.Lightmap &&
            a.Surface.Skinning == nullptr &&
            b.Surface.Skinning == nullptr;
}

void SurfaceDrawCallHandler::WriteDrawCall(InstanceData* instanceData, const DrawCall& drawCall)
{
    instanceData->InstanceOrigin = Vector3(drawCall.World.M41, drawCall.World.M42, drawCall.World.M43);
    instanceData->PerInstanceRandom = drawCall.PerInstanceRandom;
    instanceData->InstanceTransform1 = Vector3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13);
    instanceData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
    instanceData->InstanceTransform2 = Vector3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23);
    instanceData->InstanceTransform3 = Vector3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33);
    instanceData->InstanceLightmapArea = Half4(drawCall.Surface.LightmapUVsArea);
}
