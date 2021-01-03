// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "TerrainMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Terrain/TerrainPatch.h"

PACK_STRUCT(struct TerrainMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix ViewMatrix;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    Rectangle LightmapArea;
    Vector3 WorldInvScale;
    float WorldDeterminantSign;
    float PerInstanceRandom;
    float CurrentLOD; // Index of the current LOD
    float ChunkSizeNextLOD; // ChunkSize for the next current LOD (after applying LOD down-scaling)
    float TerrainChunkSizeLOD0; // Size of the terrain chunk in world units of the top-most LOD0
    Vector4 HeightmapUVScaleBias; // xy-scale, zw-offset for chunk geometry UVs into heightmap UVs (as single MAD instruction)
    Vector4 NeighborLOD; // Per component LOD index for chunk neighbors ordered: top, left, right, bottom
    Vector2 OffsetUV; // Offset applied to the texture coordinates (used to implement seamless UVs based on chunk location relative to terrain root)
    Vector2 Dummy0;
    });

DrawPass TerrainMaterialShader::GetDrawModes() const
{
    return DrawPass::Depth | DrawPass::GBuffer;
}

bool TerrainMaterialShader::CanUseLightmap() const
{
    return true;
}

void TerrainMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    const auto cb0 = _shader->GetCB(0);
    const bool hasCb0 = cb0->GetSize() != 0;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Buffer0 = hasCb0 ? _cb0Data.Get() + sizeof(TerrainMaterialShaderData) : nullptr;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = nullptr;
    bindMeta.CanSampleDepth = false;
    bindMeta.CanSampleGBuffer = false;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup material constants data
    auto data = reinterpret_cast<TerrainMaterialShaderData*>(_cb0Data.Get());
    if (hasCb0)
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), data->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, data->WorldMatrix);
        Matrix::Transpose(view.View, data->ViewMatrix);

        data->ViewPos = view.Position;
        data->ViewFar = view.Far;
        data->ViewDir = view.Direction;
        data->TimeParam = Time::Draw.UnscaledTime.GetTotalSeconds();
        data->ViewInfo = view.ViewInfo;
        data->ScreenSize = view.ScreenSize;

        // Extract per axis scales from LocalToWorld transform
        const float scaleX = Vector3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Vector3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Vector3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        data->WorldInvScale = Vector3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);

        data->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        data->PerInstanceRandom = drawCall.PerInstanceRandom;
        data->CurrentLOD = drawCall.TerrainData.CurrentLOD;
        data->ChunkSizeNextLOD = drawCall.TerrainData.ChunkSizeNextLOD;
        data->TerrainChunkSizeLOD0 = drawCall.TerrainData.TerrainChunkSizeLOD0;
        data->HeightmapUVScaleBias = drawCall.TerrainData.HeightmapUVScaleBias;
        data->NeighborLOD = drawCall.TerrainData.NeighborLOD;
        data->OffsetUV = drawCall.TerrainData.OffsetUV;
    }
    const bool useLightmap = view.Flags & ViewFlags::GI
#if USE_EDITOR
            && EnableLightmapsUsage
#endif
            && view.Pass == DrawPass::GBuffer
            && drawCall.Lightmap != nullptr;
    if (useLightmap)
    {
        // Bind lightmap textures
        GPUTexture *lightmap0, *lightmap1, *lightmap2;
        drawCall.Lightmap->GetTextures(&lightmap0, &lightmap1, &lightmap2);
        context->BindSR(0, lightmap0);
        context->BindSR(1, lightmap1);
        context->BindSR(2, lightmap2);

        // Set lightmap data
        data->LightmapArea = drawCall.LightmapUVsArea;
    }

    // Bind terrain textures
    const auto heightmap = drawCall.TerrainData.Patch->Heightmap.Get()->GetTexture();
    const auto splatmap0 = drawCall.TerrainData.Patch->Splatmap[0] ? drawCall.TerrainData.Patch->Splatmap[0]->GetTexture() : nullptr;
    const auto splatmap1 = drawCall.TerrainData.Patch->Splatmap[1] ? drawCall.TerrainData.Patch->Splatmap[1]->GetTexture() : nullptr;
    context->BindSR(3, heightmap);
    context->BindSR(4, splatmap0);
    context->BindSR(5, splatmap1);

    // Bind constants
    if (hasCb0)
    {
        context->UpdateCB(cb0, _cb0Data.Get());
        context->BindCB(0, cb0);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
#if USE_EDITOR
    if (IsRunningRadiancePass)
        cullMode = CullMode::TwoSided;
#endif
    if (cullMode != CullMode::TwoSided && drawCall.IsNegativeScale())
    {
        // Invert culling when scale is negative
        if (cullMode == CullMode::Normal)
            cullMode = CullMode::Inverted;
        else
            cullMode = CullMode::Normal;
    }
    const PipelineStateCache* psCache = _cache.GetPS(view.Pass, useLightmap);
    ASSERT(psCache);
    GPUPipelineState* state = ((PipelineStateCache*)psCache)->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void TerrainMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
}

bool TerrainMaterialShader::Load()
{
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthTestEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == 0;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == 0;

    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }

    // Support blending but then use only emissive channel
    switch (_info.BlendMode)
    {
    case MaterialBlendMode::Transparent:
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        break;
    case MaterialBlendMode::Additive:
        psDesc.BlendMode = BlendingMode::Additive;
        break;
    case MaterialBlendMode::Multiply:
        psDesc.BlendMode = BlendingMode::Multiply;
        break;
    default: ;
    }

    // GBuffer Pass
    psDesc.VS = _shader->GetVS("VS");
    psDesc.PS = _shader->GetPS("PS_GBuffer");
    _cache.Default.Init(psDesc);

    // GBuffer Pass with lightmap (use pixel shader permutation for USE_LIGHTMAP=1)
    psDesc.PS = _shader->GetPS("PS_GBuffer", 1);
    _cache.DefaultLightmap.Init(psDesc);

    // Depth Pass
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.BlendMode = BlendingMode::Opaque;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthTestEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    // TODO: masked terrain materials (depth pass should clip holes)
    psDesc.PS = nullptr;
    _cache.Depth.Init(psDesc);

    return false;
}
