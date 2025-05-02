// Copyright (c) Wojciech Figat. All rights reserved.

#include "RenderList.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Graphics/Materials/IMaterial.h"
#include "Engine/Graphics/Materials/MaterialShader.h"
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
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/PostFxVolume.h"

static_assert(sizeof(DrawCall) <= 288, "Too big draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Terrain), "Wrong draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Particle), "Wrong draw call data size.");
static_assert(sizeof(DrawCall::Surface) >= sizeof(DrawCall::Custom), "Wrong draw call data size.");
static_assert(sizeof(ShaderObjectData) == sizeof(Float4) * ARRAY_COUNT(ShaderObjectData::Raw), "Wrong object data.");

namespace
{
    Array<RenderList*> FreeRenderList;
    Array<Pair<void*, uintptr>> MemPool;
    CriticalSection MemPoolLocker;
}

void ShaderObjectData::Store(const Matrix& worldMatrix, const Matrix& prevWorldMatrix, const Rectangle& lightmapUVsArea, const Float3& geometrySize, float perInstanceRandom, float worldDeterminantSign, float lodDitherFactor)
{
    Half4 lightmapUVsAreaPacked(*(Float4*)&lightmapUVsArea);
    Float2 lightmapUVsAreaPackedAliased = *(Float2*)&lightmapUVsAreaPacked;
    Raw[0] = Float4(worldMatrix.M11, worldMatrix.M12, worldMatrix.M13, worldMatrix.M41);
    Raw[1] = Float4(worldMatrix.M21, worldMatrix.M22, worldMatrix.M23, worldMatrix.M42);
    Raw[2] = Float4(worldMatrix.M31, worldMatrix.M32, worldMatrix.M33, worldMatrix.M43);
    Raw[3] = Float4(prevWorldMatrix.M11, prevWorldMatrix.M12, prevWorldMatrix.M13, prevWorldMatrix.M41);
    Raw[4] = Float4(prevWorldMatrix.M21, prevWorldMatrix.M22, prevWorldMatrix.M23, prevWorldMatrix.M42);
    Raw[5] = Float4(prevWorldMatrix.M31, prevWorldMatrix.M32, prevWorldMatrix.M33, prevWorldMatrix.M43);
    Raw[6] = Float4(geometrySize, perInstanceRandom);
    Raw[7] = Float4(worldDeterminantSign, lodDitherFactor, lightmapUVsAreaPackedAliased.X, lightmapUVsAreaPackedAliased.Y);
    // TODO: pack WorldDeterminantSign and LODDitherFactor
}

void ShaderObjectData::Load(Matrix& worldMatrix, Matrix& prevWorldMatrix, Rectangle& lightmapUVsArea, Float3& geometrySize, float& perInstanceRandom, float& worldDeterminantSign, float& lodDitherFactor) const
{
    worldMatrix.SetRow1(Float4(Float3(Raw[0]), 0.0f));
    worldMatrix.SetRow2(Float4(Float3(Raw[1]), 0.0f));
    worldMatrix.SetRow3(Float4(Float3(Raw[2]), 0.0f));
    worldMatrix.SetRow4(Float4(Raw[0].W, Raw[1].W, Raw[2].W, 1.0f));
    prevWorldMatrix.SetRow1(Float4(Float3(Raw[3]), 0.0f));
    prevWorldMatrix.SetRow2(Float4(Float3(Raw[4]), 0.0f));
    prevWorldMatrix.SetRow3(Float4(Float3(Raw[5]), 0.0f));
    prevWorldMatrix.SetRow4(Float4(Raw[3].W, Raw[4].W, Raw[5].W, 1.0f));
    geometrySize = Float3(Raw[6]);
    perInstanceRandom = Raw[6].W;
    worldDeterminantSign = Raw[7].X;
    lodDitherFactor = Raw[7].Y;
    Float2 lightmapUVsAreaPackedAliased(Raw[7].Z, Raw[7].W);
    Half4 lightmapUVsAreaPacked(*(Half4*)&lightmapUVsAreaPackedAliased);
    *(Float4*)&lightmapUVsArea = lightmapUVsAreaPacked.ToFloat4();
}

bool RenderLightData::CanRenderShadow(const RenderView& view) const
{
    bool result = false;
    switch (ShadowsMode)
    {
    case ShadowsCastingMode::StaticOnly:
        result = view.IsOfflinePass;
        break;
    case ShadowsCastingMode::DynamicOnly:
        result = !view.IsOfflinePass;
        break;
    case ShadowsCastingMode::All:
        result = true;
        break;
    }
    return result && ShadowsStrength > ZeroTolerance;
}

void RenderDirectionalLightData::SetShaderData(ShaderLightData& data, bool useShadow) const
{
    data.SpotAngles.X = -2.0f;
    data.SpotAngles.Y = 1.0f;
    data.SourceRadius = 0;
    data.SourceLength = 0;
    data.Color = Color;
    data.MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data.Position = Float3::Zero;
    data.ShadowsBufferAddress = useShadow ? ShadowsBufferAddress : 0;
    data.Direction = -Direction;
    data.Radius = 0;
    data.FalloffExponent = 0;
    data.InverseSquared = 0;
    data.RadiusInv = 0;
}

bool RenderLocalLightData::CanRenderShadow(const RenderView& view) const
{
    // Fade shadow on distance
    const float fadeDistance = Math::Max(ShadowsFadeDistance, 0.1f);
    const float dstLightToView = Float3::Distance(Position, view.Position);
    const float fade = 1 - Math::Saturate((dstLightToView - Radius - ShadowsDistance + fadeDistance) / fadeDistance);
    return fade > ZeroTolerance && Radius > 10 && RenderLightData::CanRenderShadow(view);
}

void RenderSpotLightData::SetShaderData(ShaderLightData& data, bool useShadow) const
{
    data.SpotAngles.X = CosOuterCone;
    data.SpotAngles.Y = InvCosConeDifference;
    data.SourceRadius = SourceRadius;
    data.SourceLength = 0.0f;
    data.Color = Color;
    data.MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data.Position = Position;
    data.ShadowsBufferAddress = useShadow ? ShadowsBufferAddress : 0;
    data.Direction = Direction;
    data.Radius = Radius;
    data.FalloffExponent = FallOffExponent;
    data.InverseSquared = UseInverseSquaredFalloff ? 1.0f : 0.0f;
    data.RadiusInv = 1.0f / Radius;
}

void RenderPointLightData::SetShaderData(ShaderLightData& data, bool useShadow) const
{
    data.SpotAngles.X = -2.0f;
    data.SpotAngles.Y = 1.0f;
    data.SourceRadius = SourceRadius;
    data.SourceLength = SourceLength;
    data.Color = Color;
    data.MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
    data.Position = Position;
    data.ShadowsBufferAddress = useShadow ? ShadowsBufferAddress : 0;
    data.Direction = Direction;
    data.Radius = Radius;
    data.FalloffExponent = FallOffExponent;
    data.InverseSquared = UseInverseSquaredFalloff ? 1.0f : 0.0f;
    data.RadiusInv = 1.0f / Radius;
}

void RenderSkyLightData::SetShaderData(ShaderLightData& data, bool useShadow) const
{
    data.SpotAngles.X = AdditiveColor.X;
    data.SpotAngles.Y = AdditiveColor.Y;
    data.SourceRadius = AdditiveColor.Z;
    data.SourceLength = Image ? Image->StreamingTexture()->TotalMipLevels() - 2.0f : 0.0f;
    data.Color = Color;
    data.MinRoughness = MIN_ROUGHNESS;
    data.Position = Position;
    data.ShadowsBufferAddress = useShadow ? ShadowsBufferAddress : 0;
    data.Direction = Float3::Forward;
    data.Radius = Radius;
    data.FalloffExponent = 0;
    data.InverseSquared = 0;
    data.RadiusInv = 1.0f / Radius;
}

void RenderEnvironmentProbeData::SetShaderData(ShaderEnvProbeData& data) const
{
    data.Data0 = Float4(Position, 0);
    data.Data1 = Float4(Radius, 1.0f / Radius, Brightness, 0);
}

void* RendererAllocation::Allocate(uintptr size)
{
    void* result = nullptr;
    MemPoolLocker.Lock();
    for (int32 i = 0; i < MemPool.Count(); i++)
    {
        if (MemPool.Get()[i].Second == size)
        {
            result = MemPool.Get()[i].First;
            MemPool.RemoveAt(i);
            break;
        }
    }
    MemPoolLocker.Unlock();
    if (!result)
        result = Platform::Allocate(size, 16);
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
    MemPoolLocker.Lock();
    if (FreeRenderList.HasItems())
    {
        const auto result = FreeRenderList.Last();
        FreeRenderList.RemoveLast();
        MemPoolLocker.Unlock();
        return result;
    }
    MemPoolLocker.Unlock();

    return New<RenderList>();
}

void RenderList::ReturnToPool(RenderList* cache)
{
    if (!cache)
        return;
    cache->Clear();

    MemPoolLocker.Lock();
    ASSERT(!FreeRenderList.Contains(cache));
    FreeRenderList.Add(cache);
    MemPoolLocker.Unlock();
}

void RenderList::CleanupCache()
{
    // Don't call it during rendering (data may be already in use)
    ASSERT(GPUDevice::Instance == nullptr || GPUDevice::Instance->CurrentTask == nullptr);

    MemPoolLocker.Lock();
    FreeRenderList.ClearDelete();
    for (auto& e : MemPool)
        Platform::Free(e.First);
    MemPool.Clear();
    MemPoolLocker.Unlock();
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
            context->ResetSR();
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
    , ObjectBuffer(0, PixelFormat::R32G32B32A32_Float, false, TEXT("Object Bufffer"))
    , TempObjectBuffer(0, PixelFormat::R32G32B32A32_Float, false, TEXT("Object Bufffer"))
    , _instanceBuffer(0, sizeof(ShaderObjectDrawInstanceData), TEXT("Instance Buffer"), GPUVertexLayout::Get({ { VertexElement::Types::Attribute0, 3, 0, 1, PixelFormat::R32_UInt } }))
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
    ObjectBuffer.Clear();
    TempObjectBuffer.Clear();
}

struct PackedSortKey
{
    union
    {
        uint64 Data;

        PACK_BEGIN()

        struct
        {
            // Sorting order: By Sort Order -> By Distance -> By Material -> By Geometry
            uint8 DrawKey;
            uint16 BatchKey;
            uint32 DistanceKey;
            uint8 SortKey;
        } PACK_END();
    };
};

FORCE_INLINE void CalculateSortKey(const RenderContext& renderContext, DrawCall& drawCall, int8 sortOrder)
{
    const Float3 planeNormal = renderContext.View.Direction;
    const float planePoint = -Float3::Dot(planeNormal, renderContext.View.Position);
    const float distance = Float3::Dot(planeNormal, drawCall.ObjectPosition) - planePoint;
    PackedSortKey key;
    key.DistanceKey = RenderTools::ComputeDistanceSortKey(distance);
    uint32 batchKey = GetHash(drawCall.Material);
    IMaterial::InstancingHandler handler;
    if (drawCall.Material->CanUseInstancing(handler))
        handler.GetHash(drawCall, batchKey);
    key.BatchKey = (uint16)batchKey;
    uint32 drawKey = (uint32)(471 * drawCall.WorldDeterminantSign);
    drawKey = (drawKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[0]);
    drawKey = (drawKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[1]);
    drawKey = (drawKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[2]);
    drawKey = (drawKey * 397) ^ GetHash(drawCall.Geometry.IndexBuffer);
    key.DrawKey = (uint8)drawKey;
    key.SortKey = (uint8)(sortOrder - MIN_int8);
    drawCall.SortKey = key.Data;
}

void RenderList::AddDrawCall(const RenderContext& renderContext, DrawPass drawModes, StaticFlags staticFlags, DrawCall& drawCall, bool receivesDecals, int8 sortOrder)
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

void RenderList::AddDrawCall(const RenderContextBatch& renderContextBatch, DrawPass drawModes, StaticFlags staticFlags, ShadowsCastingMode shadowsMode, const BoundingSphere& bounds, DrawCall& drawCall, bool receivesDecals, int8 sortOrder)
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
        if (drawModes != DrawPass::None &&
            (staticFlags & renderContext.View.StaticFlagsMask) == renderContext.View.StaticFlagsCompare &&
            renderContext.View.CullingFrustum.Intersects(bounds))
        {
            renderContext.List->ShadowDepthDrawCallsList.Indices.Add(index);
        }
    }
}

void RenderList::BuildObjectsBuffer()
{
    int32 count = DrawCalls.Count();
    for (const auto& e : BatchedDrawCalls)
        count += e.Instances.Count();
    ObjectBuffer.Clear();
    if (count == 0)
        return;
    PROFILE_CPU();
    ObjectBuffer.Data.Resize(count * sizeof(ShaderObjectData));
    auto* src = (const DrawCall*)DrawCalls.Get();
    auto* dst = (ShaderObjectData*)ObjectBuffer.Data.Get();
    for (int32 i = 0; i < DrawCalls.Count(); i++)
    {
        dst->Store(src[i]);
        dst++;
    }
    int32 startIndex = DrawCalls.Count();
    for (auto& batch : BatchedDrawCalls)
    {
        batch.ObjectsStartIndex = startIndex;
        Platform::MemoryCopy(dst, batch.Instances.Get(), batch.Instances.Count() * sizeof(ShaderObjectData));
        dst += batch.Instances.Count();
        startIndex += batch.Instances.Count();
    }
    ZoneValue(ObjectBuffer.Data.Count() / 1024); // Objects Buffer size in kB
}

void RenderList::SortDrawCalls(const RenderContext& renderContext, bool reverseDistance, DrawCallsList& list, const RenderListBuffer<DrawCall>& drawCalls, DrawPass pass, bool stable)
{
    PROFILE_CPU();
    const auto* drawCallsData = drawCalls.Get();
    const auto* listData = list.Indices.Get();
    const int32 listSize = list.Indices.Count();
    ZoneValue(listSize);

    // Use shared memory from renderer allocator
    Array<uint64, RendererAllocation> SortingKeys[2];
    Array<int32, RendererAllocation> SortingIndices;
    SortingKeys[0].Resize(listSize);
    SortingKeys[1].Resize(listSize);
    SortingIndices.Resize(listSize);
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
            key.SortKey ^= MAX_uint8; // Reverse sort order
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
        IMaterial::InstancingHandler drawCallHandler, otherHandler;
        if (instanceCount != 0 && drawCall.Material->CanUseInstancing(drawCallHandler))
        {
            // Check the following draw calls sequence to merge them
            for (int32 j = i + 1; j < listSize; j++)
            {
                const DrawCall& other = drawCallsData[listData[j]];
                const bool canBatch =
                        other.Material->CanUseInstancing(otherHandler) &&
                        other.InstanceCount != 0 &&
                        drawCallHandler.CanBatch == otherHandler.CanBatch &&
                        drawCallHandler.CanBatch(drawCall, other, pass) &&
                        drawCall.WorldDeterminantSign * other.WorldDeterminantSign > 0;
                if (!canBatch)
                    break;
                batchSize++;
                instanceCount += other.InstanceCount;
            }
        }

        DrawBatch batch;
        static_assert(sizeof(DrawBatch) == sizeof(uint64) * 2, "Fix the size of draw batch to optimize memory access.");
        batch.SortKey = sortedKeys[i];
        batch.StartIndex = i;
        batch.BatchSize = batchSize;
        batch.InstanceCount = instanceCount;
        list.Batches.Add(batch);

        i += batchSize;
    }

    // When using depth buffer draw calls are already almost ideally sorted by Radix Sort but transparency needs more stability to prevent flickering
    if (stable)
    {
        // Sort draw calls batches by depth
        Array<DrawBatch, RendererAllocation> sortingBatches;
        Sorting::MergeSort(list.Batches, &sortingBatches);
    }
}

FORCE_INLINE bool CanUseInstancing(DrawPass pass)
{
    return pass == DrawPass::GBuffer || pass == DrawPass::Depth;
}

FORCE_INLINE bool DrawsEqual(const DrawCall* a, const DrawCall* b)
{
    return a->Geometry.IndexBuffer == b->Geometry.IndexBuffer &&
            a->Draw.IndicesCount == b->Draw.IndicesCount &&
            a->Draw.StartIndex == b->Draw.StartIndex &&
            Platform::MemoryCompare(a->Geometry.VertexBuffers, b->Geometry.VertexBuffers, sizeof(a->Geometry.VertexBuffers) + sizeof(a->Geometry.VertexBuffersOffsets)) == 0;
}

void RenderList::ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list, RenderList* drawCallsList, GPUTextureView* input)
{
    if (list.IsEmpty())
        return;
    PROFILE_GPU_CPU("Drawing");
    const auto* drawCallsData = drawCallsList->DrawCalls.Get();
    const auto* listData = list.Indices.Get();
    const auto* batchesData = list.Batches.Get();
    const auto context = GPUDevice::Instance->GetMainContext();
    bool useInstancing = list.CanUseInstancing && CanUseInstancing(renderContext.View.Pass) && GPUDevice::Instance->Limits.HasInstancing;
    TaaJitterRemoveContext taaJitterRemove(renderContext.View);

    // Lazy-init objects buffer (if user didn't do it)
    if (drawCallsList->ObjectBuffer.Data.IsEmpty())
    {
        drawCallsList->BuildObjectsBuffer();
        drawCallsList->ObjectBuffer.Flush(context);
    }

    // Clear SR slots to prevent any resources binding issues (leftovers from the previous passes)
    context->ResetSR();

    // Prepare instance buffer
    if (useInstancing)
    {
        // Estimate the maximum amount of elements for all instanced draws
        int32 instancesCount = 0;
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            const DrawBatch& batch = batchesData[i];
            if (batch.BatchSize > 1)
                instancesCount += batch.BatchSize;
        }
        for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
        {
            const BatchedDrawCall& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            instancesCount += batch.Instances.Count();
        }
        if (instancesCount != 0)
        {
            PROFILE_CPU_NAMED("Build Instancing");
            _instanceBuffer.Clear();
            _instanceBuffer.Data.Resize(instancesCount * sizeof(ShaderObjectDrawInstanceData));
            auto instanceData = (ShaderObjectDrawInstanceData*)_instanceBuffer.Data.Get();

            // Write to instance buffer
            for (int32 i = 0; i < list.Batches.Count(); i++)
            {
                const DrawBatch& batch = batchesData[i];
                if (batch.BatchSize > 1)
                {
                    for (int32 j = 0; j < batch.BatchSize; j++)
                    {
                        instanceData->ObjectIndex = listData[batch.StartIndex + j];
                        instanceData++;
                    }
                }
            }
            for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
            {
                const BatchedDrawCall& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
                for (int32 j = 0; j < batch.Instances.Count(); j++)
                {
                    instanceData->ObjectIndex = batch.ObjectsStartIndex + j;
                    instanceData++;
                }
            }
            ASSERT((byte*)instanceData == _instanceBuffer.Data.end());

            // Upload data
            _instanceBuffer.Flush(context);
            ZoneValue(instancesCount);
        }
        else
        {
            // No batches so no instancing
            useInstancing = false;
        }
    }

    // Execute draw calls
    int32 materialBinds = list.Batches.Count();
    MaterialBase::BindParameters bindParams(context, renderContext);
    bindParams.ObjectBuffer = drawCallsList->ObjectBuffer.GetBuffer()->View();
    bindParams.Input = input;
    bindParams.BindViewData();
    MaterialShaderDataPerDraw perDraw;
    perDraw.DrawPadding = Float3::Zero;
    GPUConstantBuffer* perDrawCB = IMaterial::BindParameters::PerDrawConstants;
    context->BindCB(2, perDrawCB); // TODO: use rootSignature/pushConstants on D3D12/Vulkan
    constexpr int32 vbMax = ARRAY_COUNT(DrawCall::Geometry.VertexBuffers);
    if (useInstancing)
    {
        GPUBuffer* vb[vbMax + 1];
        uint32 vbOffsets[vbMax + 1];
        vb[3] = _instanceBuffer.GetBuffer(); // Pass object index in a vertex stream at slot 3 (used by VS in Surface.shader)
        vbOffsets[3] = 0;
        int32 instanceBufferOffset = 0;
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            const DrawBatch& batch = batchesData[i];
            uint32 drawCallIndex = listData[batch.StartIndex];
            const DrawCall& drawCall = drawCallsData[drawCallIndex];

            bindParams.Instanced = batch.BatchSize != 1;
            bindParams.DrawCall = &drawCall;
            bindParams.DrawCall->Material->Bind(bindParams);

            if (bindParams.Instanced)
            {
                // One or more draw calls per batch
                const DrawCall* activeDraw = &drawCall;
                int32 activeCount = 1;
                for (int32 j = 1; j <= batch.BatchSize; j++)
                {
                    if (j != batch.BatchSize && DrawsEqual(activeDraw, drawCallsData + listData[batch.StartIndex + j]))
                    {
                        // Group two draw calls into active draw call
                        activeCount++;
                        continue;
                    }

                    // Draw whole active draw (instanced)
                    Platform::MemoryCopy(vb, activeDraw->Geometry.VertexBuffers, sizeof(DrawCall::Geometry.VertexBuffers));
                    Platform::MemoryCopy(vbOffsets, activeDraw->Geometry.VertexBuffersOffsets, sizeof(DrawCall::Geometry.VertexBuffersOffsets));
                    context->BindIB(activeDraw->Geometry.IndexBuffer);
                    context->BindVB(ToSpan(vb, ARRAY_COUNT(vb)), vbOffsets);
                    context->DrawIndexedInstanced(activeDraw->Draw.IndicesCount, activeCount, instanceBufferOffset, 0, activeDraw->Draw.StartIndex);
                    instanceBufferOffset += activeCount;

                    // Reset active draw
                    activeDraw = drawCallsData + listData[batch.StartIndex + j];
                    activeCount = 1;
                }
            }
            else
            {
                // Pass object index in constant buffer
                perDraw.DrawObjectIndex = drawCallIndex;
                context->UpdateCB(perDrawCB, &perDraw);

                // Single-draw call batch
                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, vbMax), drawCall.Geometry.VertexBuffersOffsets);
                if (drawCall.InstanceCount == 0)
                {
                    context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                }
                else
                {
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                }
            }
        }
        for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
        {
            const BatchedDrawCall& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            const DrawCall& drawCall = batch.DrawCall;

            bindParams.Instanced = true;
            bindParams.DrawCall = &drawCall;
            bindParams.DrawCall->Material->Bind(bindParams);

            Platform::MemoryCopy(vb, drawCall.Geometry.VertexBuffers, sizeof(DrawCall::Geometry.VertexBuffers));
            Platform::MemoryCopy(vbOffsets, drawCall.Geometry.VertexBuffersOffsets, sizeof(DrawCall::Geometry.VertexBuffersOffsets));
            context->BindIB(drawCall.Geometry.IndexBuffer);
            context->BindVB(ToSpan(vb, vbMax + 1), vbOffsets);

            if (drawCall.InstanceCount == 0)
            {
                context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
            }
            else
            {
                context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.Instances.Count(), instanceBufferOffset, 0, drawCall.Draw.StartIndex);
                instanceBufferOffset += batch.Instances.Count();
            }
        }
        materialBinds += list.PreBatchedDrawCalls.Count();
    }
    else
    {
        for (int32 i = 0; i < list.Batches.Count(); i++)
        {
            const DrawBatch& batch = batchesData[i];

            bindParams.DrawCall = drawCallsData + listData[batch.StartIndex];
            bindParams.DrawCall->Material->Bind(bindParams);

            for (int32 j = 0; j < batch.BatchSize; j++)
            {
                perDraw.DrawObjectIndex = listData[batch.StartIndex + j];
                context->UpdateCB(perDrawCB, &perDraw);

                const DrawCall& drawCall = drawCallsData[perDraw.DrawObjectIndex];
                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, vbMax), drawCall.Geometry.VertexBuffersOffsets);

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
            const BatchedDrawCall& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
            const DrawCall& drawCall = batch.DrawCall;

            bindParams.DrawCall = &drawCall;
            bindParams.DrawCall->Material->Bind(bindParams);

            context->BindIB(drawCall.Geometry.IndexBuffer);
            context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, vbMax), drawCall.Geometry.VertexBuffersOffsets);

            for (int32 j = 0; j < batch.Instances.Count(); j++)
            {
                perDraw.DrawObjectIndex = batch.ObjectsStartIndex + j;
                context->UpdateCB(perDrawCB, &perDraw);

                context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
            }
        }
        materialBinds += list.PreBatchedDrawCalls.Count();
        if (list.Batches.IsEmpty() && list.Indices.Count() != 0)
        {
            // Draw calls list has bot been batched so execute draw calls separately
            for (int32 j = 0; j < list.Indices.Count(); j++)
            {
                perDraw.DrawObjectIndex = listData[j];
                context->UpdateCB(perDrawCB, &perDraw);

                const DrawCall& drawCall = drawCallsData[perDraw.DrawObjectIndex];
                bindParams.DrawCall = &drawCall;
                drawCall.Material->Bind(bindParams);

                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, vbMax), drawCall.Geometry.VertexBuffersOffsets);

                if (drawCall.InstanceCount == 0)
                {
                    context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                }
                else
                {
                    context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                }
            }
            materialBinds += list.Indices.Count();
        }
    }
    ZoneValue(materialBinds); // Material shaders bindings count
}

void SurfaceDrawCallHandler::GetHash(const DrawCall& drawCall, uint32& batchKey)
{
    batchKey = (batchKey * 397) ^ ::GetHash(drawCall.Surface.Lightmap);
}

bool SurfaceDrawCallHandler::CanBatch(const DrawCall& a, const DrawCall& b, DrawPass pass)
{
    // TODO: find reason why batching static meshes with lightmap causes problems with sampling in shader (flickering when meshes in batch order gets changes due to async draw calls collection)
    if (a.Surface.Lightmap == nullptr && b.Surface.Lightmap == nullptr &&
        a.Surface.Skinning == nullptr && b.Surface.Skinning == nullptr)
    {
        if (a.Material != b.Material)
        {
            // Batch simple materials during depth-only drawing (when using default vertex shader and no pixel shader)
            if (pass == DrawPass::Depth)
            {
                const MaterialInfo& aInfo = a.Material->GetInfo();
                const MaterialInfo& bInfo = b.Material->GetInfo();
                constexpr MaterialUsageFlags complexUsageFlags = MaterialUsageFlags::UseMask | MaterialUsageFlags::UsePositionOffset | MaterialUsageFlags::UseDisplacement;
                const bool aIsSimple = EnumHasNoneFlags(aInfo.UsageFlags, complexUsageFlags) && aInfo.BlendMode == MaterialBlendMode::Opaque;
                const bool bIsSimple = EnumHasNoneFlags(bInfo.UsageFlags, complexUsageFlags) && bInfo.BlendMode == MaterialBlendMode::Opaque;
                return aIsSimple && bIsSimple;
            }
            return false;
        }
        return true;
    }
    return false;
}
