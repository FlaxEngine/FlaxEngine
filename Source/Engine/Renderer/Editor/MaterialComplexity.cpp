// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "MaterialComplexity.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Level/Actors/Decal.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/Lightmaps.h"

const MaterialInfo& MaterialComplexityMaterialShader::WrapperShader::GetInfo() const
{
    if (MaterialAsset)
        return MaterialAsset->GetInfo();
    return Info;
}

GPUShader* MaterialComplexityMaterialShader::WrapperShader::GetShader() const
{
    return MaterialAsset->GetShader();
}

bool MaterialComplexityMaterialShader::WrapperShader::IsReady() const
{
    return MaterialAsset && MaterialAsset->IsReady();
}

bool MaterialComplexityMaterialShader::WrapperShader::CanUseInstancing(InstancingHandler& handler) const
{
    return MaterialAsset && MaterialAsset->CanUseInstancing(handler);
}

DrawPass MaterialComplexityMaterialShader::WrapperShader::GetDrawModes() const
{
    return MaterialAsset->GetDrawModes();
}

void MaterialComplexityMaterialShader::WrapperShader::Bind(BindParameters& params)
{
    auto& drawCall = *params.DrawCall;

    // Get original material from the draw call
    IMaterial* material = nullptr;
    switch (Domain)
    {
    case MaterialDomain::Surface:
    case MaterialDomain::Terrain:
        material = *(IMaterial**)&drawCall.Surface.Lightmap;
        break;
    case MaterialDomain::Particle:
        material = *(IMaterial**)&drawCall.ObjectPosition;
        break;
    case MaterialDomain::Decal:
        material = drawCall.Material;
        break;
    }

    // Disable lightmaps
    const auto lightmapsEnable = EnableLightmapsUsage;
    EnableLightmapsUsage = false;

    // Estimate the shader complexity
    ASSERT_LOW_LAYER(material && material->IsReady());
    material->Bind(params);
    GPUPipelineState* materialPs = params.GPUContext->GetState();
    const float complexity = (float)Math::Min(materialPs->Complexity, MATERIAL_COMPLEXITY_LIMIT) / MATERIAL_COMPLEXITY_LIMIT;

    // Draw with custom color
    const Color color(complexity, complexity, complexity, 1.0f);
    MaterialAsset->SetParameterValue(TEXT("Color"), Variant(color));
    MaterialAsset->Bind(params);

    EnableLightmapsUsage = lightmapsEnable;
}

MaterialComplexityMaterialShader::MaterialComplexityMaterialShader()
{
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Editor/MaterialComplexity"));

    // Initialize material wrappers table with separate materials for each material domain type and shader configuration
#define INIT_WRAPPER(i, domain, asset) _wrappers[i].Domain = MaterialDomain::domain; _wrappers[i].MaterialAsset = Content::LoadAsyncInternal<Material>(TEXT(asset))
    INIT_WRAPPER(0, Surface, "Editor/DebugMaterials/SingleColor/Surface");
    INIT_WRAPPER(1, Surface, "Editor/DebugMaterials/SingleColor/SurfaceAdditive");
    INIT_WRAPPER(2, Terrain, "Editor/DebugMaterials/SingleColor/Terrain");
    INIT_WRAPPER(3, Particle, "Editor/DebugMaterials/SingleColor/Particle");
    INIT_WRAPPER(4, Decal, "Editor/DebugMaterials/SingleColor/Decal");
    // TODO: deformable splines rendering cost for complexity
    // TODO: volumetric fog particles rendering cost for complexity
#undef INIT_WRAPPER
}

void MaterialComplexityMaterialShader::DebugOverrideDrawCallsMaterial(RenderContext& renderContext)
{
    PROFILE_CPU();

    // Cache 'ready' state for wrappers
    bool isReady[ARRAY_COUNT(_wrappers) + 1];
    for (int32 i = 0; i < ARRAY_COUNT(_wrappers); i++)
        isReady[i] = _wrappers[i].IsReady();
    isReady[ARRAY_COUNT(_wrappers)] = false;

    // Override all draw calls
    for (auto& e : renderContext.List->DrawCalls)
        DebugOverrideDrawCallsMaterial(e, isReady);
    for (auto& e : renderContext.List->BatchedDrawCalls)
        DebugOverrideDrawCallsMaterial(e.DrawCall, isReady);
}

void MaterialComplexityMaterialShader::Draw(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer)
{
    // Draw decals into Light buffer to include them into complexity drawing
    auto& decals = renderContext.List->Decals;
    auto boxModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/SimpleBox"));
    auto& decalsWrapper = _wrappers[4];
    if (decals.HasItems() && boxModel && boxModel->CanBeRendered() && decalsWrapper.IsReady())
    {
        PROFILE_GPU_CPU_NAMED("Decals");
        DrawCall drawCall;
        MaterialBase::BindParameters bindParams(context, renderContext, drawCall);
        bindParams.BindViewData();
        drawCall.WorldDeterminantSign = 1.0f;
        drawCall.PerInstanceRandom = 0.0f;
        context->SetRenderTarget(lightBuffer);
        for (int32 i = 0; i < decals.Count(); i++)
        {
            const RenderDecalData& decal = decals.Get()[i];
            drawCall.World = decal.World;
            drawCall.ObjectPosition = drawCall.World.GetTranslation();
            drawCall.Material = decal.Material;
            decalsWrapper.Bind(bindParams);
            boxModel->Render(context);
        }
        context->ResetSR();
    }

    // Draw transparency into Light buffer to include it into complexity drawing
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    GPUTextureView* readOnlyDepthBuffer = depthBuffer->View();
    if (EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView))
        readOnlyDepthBuffer = depthBuffer->ViewReadOnlyDepth();
    context->SetRenderTarget(readOnlyDepthBuffer, lightBuffer);
    auto& distortionList = renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Distortion];
    if (!distortionList.IsEmpty())
    {
        PROFILE_GPU_CPU_NAMED("Distortion");
        renderContext.View.Pass = DrawPass::Distortion;
        renderContext.List->ExecuteDrawCalls(renderContext, distortionList);
    }
    auto& forwardList = renderContext.List->DrawCallsLists[(int32)DrawCallsListType::Forward];
    if (!forwardList.IsEmpty())
    {
        PROFILE_GPU_CPU_NAMED("Forward");
        renderContext.View.Pass = DrawPass::Forward;
        renderContext.List->ExecuteDrawCalls(renderContext, forwardList);
    }

    // Draw accumulated complexity into colors gradient
    context->ResetRenderTarget();
    context->SetRenderTarget(renderContext.Task->GetOutputView());
    context->SetViewportAndScissors(renderContext.Task->GetOutputViewport());
    if (_shader && _shader->IsLoaded())
    {
        if (!_ps)
        {
            _ps = GPUDevice::Instance->CreatePipelineState();
            auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
            psDesc.PS = _shader->GetShader()->GetPS("PS");
            _ps->Init(psDesc);
        }
        context->BindSR(0, lightBuffer);
        context->SetState(_ps);
        context->DrawFullscreenTriangle();
        return;
    }
    context->Draw(lightBuffer);
}

void MaterialComplexityMaterialShader::DebugOverrideDrawCallsMaterial(DrawCall& drawCall, const bool isReady[ARRAY_COUNT(_wrappers) + 1])
{
    auto domain = drawCall.Material->GetInfo().Domain;
    auto wrapperIndex = ARRAY_COUNT(_wrappers);
    switch (domain)
    {
    case MaterialDomain::Surface:
        wrapperIndex = EnumHasAnyFlags(drawCall.Material->GetDrawModes(), DrawPass::Forward) ? 1 : 0;
        break;
    case MaterialDomain::Terrain:
        wrapperIndex = 2;
        break;
    case MaterialDomain::Particle:
        wrapperIndex = 3;
        break;
    case MaterialDomain::Decal:
        wrapperIndex = 4;
        break;
    }
    if (isReady[wrapperIndex])
    {
        // Override draw call material and cache original material for later
        switch (domain)
        {
        case MaterialDomain::Surface:
        case MaterialDomain::Terrain:
            *(void**)&drawCall.Surface.Lightmap = drawCall.Material;
            break;
        case MaterialDomain::Particle:
        case MaterialDomain::Decal:
            *(void**)&drawCall.ObjectPosition = drawCall.Material;
            break;
        }
        drawCall.Material = &_wrappers[wrapperIndex];
    }
}

#endif
