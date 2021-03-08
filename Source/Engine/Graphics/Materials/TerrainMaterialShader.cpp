// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "TerrainMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
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
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<TerrainMaterialShaderData*>(cb);
    cb += sizeof(TerrainMaterialShaderData);
    int32 srv = 3;

    // Setup features
    const bool useLightmap = LightmapFeature::Bind(params, cb, srv);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = nullptr;
    bindMeta.CanSampleDepth = false;
    bindMeta.CanSampleGBuffer = false;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup material constants
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        const float scaleX = Vector3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Vector3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Vector3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        materialData->WorldInvScale = Vector3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);
        materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->CurrentLOD = drawCall.Terrain.CurrentLOD;
        materialData->ChunkSizeNextLOD = drawCall.Terrain.ChunkSizeNextLOD;
        materialData->TerrainChunkSizeLOD0 = drawCall.Terrain.TerrainChunkSizeLOD0;
        materialData->HeightmapUVScaleBias = drawCall.Terrain.HeightmapUVScaleBias;
        materialData->NeighborLOD = drawCall.Terrain.NeighborLOD;
        materialData->OffsetUV = drawCall.Terrain.OffsetUV;
    }

    // Bind terrain textures
    const auto heightmap = drawCall.Terrain.Patch->Heightmap->GetTexture();
    const auto splatmap0 = drawCall.Terrain.Patch->Splatmap[0] ? drawCall.Terrain.Patch->Splatmap[0]->GetTexture() : nullptr;
    const auto splatmap1 = drawCall.Terrain.Patch->Splatmap[1] ? drawCall.Terrain.Patch->Splatmap[1]->GetTexture() : nullptr;
    context->BindSR(0, heightmap);
    context->BindSR(1, splatmap0);
    context->BindSR(2, splatmap1);

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
#if USE_EDITOR
    if (IsRunningRadiancePass)
        cullMode = CullMode::TwoSided;
#endif
    if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
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
