// Copyright (c) Wojciech Figat. All rights reserved.

#include "ReflectionsPass.h"
#include "GBufferPass.h"
#include "RenderList.h"
#include "ScreenSpaceReflectionsPass.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"

GPU_CB_STRUCT(Data {
    ShaderEnvProbeData PData;
    Matrix WVP;
    ShaderGBufferData GBuffer;
    Float2 SSRTexelSize;
    Float2 Dummy0;
    });

#if GENERATE_GF_CACHE

// This code below (PreIntegratedGF namespace) is based on many Siggraph presentations about BRDF shading:
// https://blog.selfshadow.com/publications/s2015-shading-course/
// https://blog.selfshadow.com/publications/s2012-shading-course/

#define COMPILE_WITH_ASSETS_IMPORTER 1
#define COMPILE_WITH_TEXTURE_TOOL 1
#include "Engine/Engine/Globals.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/ContentImporters/ImportTexture.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"

namespace PreIntegratedGF
{
	static const int ResolutionX = 128;
	static const int ResolutionY = 32;
	static const int NumSamples = 256;

	Float2 Hammersley(int32 i, int32 sampleCount)
	{
		float E1 = (float)i / (float)sampleCount;
        float E2 = (float)((double)ReverseBits((uint32)i) * 2.3283064365386963e-10);
		return Float2(E1, E2);
	}

	Float3 ImportanceSampleGGX(Float2 E, float roughness)
	{
        float m = roughness * roughness;
        float m2 = m * m;

        float phi = 2 * PI * E.X;
        float cosTheta = sqrtf((1 - E.Y) / (1 + (m2 - 1) * E.Y));
        float sinTheta = sqrtf(1 - cosTheta * cosTheta);

        return Float3(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);
	}

    float Vis_SmithJointApprox(float roughness, float NoV, float NoL)
	{
        float a = roughness * roughness;
        float Vis_SmithV = NoL * (NoV * (1 - a) + a);
        float Vis_SmithL = NoV * (NoL * (1 - a) + a);
		return 0.5f / (Vis_SmithV + Vis_SmithL);
	}

    Float2 IntegrateBRDF(float roughness, float NoV)
	{
		if (roughness < 0.04f) roughness = 0.04f;

        Float3 V(sqrtf(1 - NoV * NoV), 0, NoV);
        float a = 0, b = 0;
		for (int32 i = 0; i < NumSamples; i++)
		{
            Float2 E = Hammersley(i, NumSamples);
            Float3 H = ImportanceSampleGGX(E, roughness);
            Float3 L = 2 * Float3::Dot(V, H) * H - V;

            float NoL = Math::Saturate(L.Z);
            float NoH = Math::Saturate(H.Z);
            float VoH = Math::Saturate(Float3::Dot(V, H));

			if (NoL > 0)
			{
                float Vis = Vis_SmithJointApprox(roughness, NoV, NoL);
                float NoL_Vis_PDF = NoL * Vis * (4 * VoH / NoH);

                float Fc = powf(1 - VoH, 5);
				a += NoL_Vis_PDF * (1 - Fc);
				b += NoL_Vis_PDF * Fc;
			}
		}
		return Float2(a, b) / (float)NumSamples;
	}

    bool OnGenerate(TextureData& image)
	{
        // Setup image
        image.Width = ResolutionX;
        image.Height = ResolutionY;
        image.Depth = 1;
        image.Format = PixelFormat::R16G16_UNorm;
        image.Items.Resize(1);
        image.Items[0].Mips.Resize(1);
        auto& mip = image.Items[0].Mips[0];
        mip.RowPitch = 4 * image.Width;
        mip.DepthPitch = mip.RowPitch * image.Height;
        mip.Lines = image.Height;
        mip.Data.Allocate(mip.DepthPitch);

        // Generate GF pairs to be sampled in [NoV, roughness] space
        auto pos = (uint16*)mip.Data.Get();
        for (int32 y = 0; y < image.Height; y++)
        {
            float roughness = ((float)y + 0.5f) / (float)image.Height;
            for (int32 x = 0; x < image.Width; x++)
            {
                float NoV = ((float)x + 0.5f) / (float)image.Width;
                Float2 brdf = IntegrateBRDF(roughness, NoV);
                *pos++ = (uint16)(Math::Saturate(brdf.X) * MAX_uint16 + 0.5f);
                *pos++ = (uint16)(Math::Saturate(brdf.Y) * MAX_uint16 + 0.5f);
            }
        }

        return false;
	}

	void Generate()
	{
        Guid id = Guid::Empty;
        ImportTexture::Options options;
        options.Type = TextureFormatType::HdrRGB;
        options.InternalFormat = PixelFormat::R16G16_UNorm;
        options.IndependentChannels = true;
        options.IsAtlas = false;
        options.sRGB = false;
        options.NeverStream = true;
        options.GenerateMipMaps = false;
        options.Compress = false;
        options.InternalLoad.Bind(&OnGenerate);
        const String path = Globals::EngineContentFolder / PRE_INTEGRATED_GF_ASSET_NAME + ASSET_FILES_EXTENSION_WITH_DOT;
        AssetsImportingManager::Create(AssetsImportingManager::CreateTextureTag, path, id, &options);
	}
};

#endif

class Model;

String ReflectionsPass::ToString() const
{
    return TEXT("ReflectionsPass");
}

bool ReflectionsPass::Init()
{
#if GENERATE_GF_CACHE
	// Generate cache
	PreIntegratedGF::Generate();
#endif

    // Create pipeline states
    _psProbe = GPUDevice::Instance->CreatePipelineState();
    _psProbeInside = GPUDevice::Instance->CreatePipelineState();
    _psCombinePass = GPUDevice::Instance->CreatePipelineState();
    _depthBounds = GPUDevice::Instance->Limits.HasDepthBounds && GPUDevice::Instance->Limits.HasReadOnlyDepth;

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Reflections"));
    _sphereModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Sphere"));
    _boxModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Box"));
    _preIntegratedGF = Content::LoadAsyncInternal<Texture>(PRE_INTEGRATED_GF_ASSET_NAME);
    if (_shader == nullptr || _sphereModel == nullptr || _boxModel == nullptr || _preIntegratedGF == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ReflectionsPass, &ReflectionsPass::OnShaderReloading>(this);
#endif

    return false;
}

bool ReflectionsPass::setupResources()
{
    // Wait for the assets
    if (!_sphereModel->CanBeRendered() || !_boxModel->CanBeRendered() || !_preIntegratedGF->IsLoaded() || !_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psProbe->IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.PS = shader->GetPS("PS_EnvProbe");
        psDesc.CullMode = CullMode::Normal;
        psDesc.DepthEnable = true;
        psDesc.DepthBoundsEnable = _depthBounds;
        if (_psProbe->Init(psDesc))
            return true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        psDesc.CullMode = CullMode::Inverted;
        if (_psProbeInside->Init(psDesc))
            return true;
    }
    psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psCombinePass->IsValid())
    {
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        if (_depthBounds)
        {
            psDesc.DepthEnable = psDesc.DepthBoundsEnable = true;
            psDesc.DepthWriteEnable = false;
        }
        psDesc.PS = shader->GetPS("PS_CombinePass");
        if (_psCombinePass->Init(psDesc))
            return true;
    }

    return false;
}

void ReflectionsPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psProbe);
    SAFE_DELETE_GPU_RESOURCE(_psProbeInside);
    SAFE_DELETE_GPU_RESOURCE(_psCombinePass);
    SAFE_DELETE_GPU_RESOURCE(_psDrawSSR);
    _shader = nullptr;
    _boxModel = nullptr;
    _sphereModel = nullptr;
    _preIntegratedGF = nullptr;
}

bool SortProbes(RenderEnvironmentProbeData const& p1, RenderEnvironmentProbeData const& p2)
{
    // Compare by Sort Order
    int32 res = p1.SortOrder - p2.SortOrder;
    if (res == 0)
    {
        // Compare by radius
        res = static_cast<int32>(p2.Radius - p1.Radius);
        if (res == 0)
        {
            // Compare by ID to prevent flickering
            res = p2.HashID - p1.HashID;
        }
    }
    return res < 0;
}

void ReflectionsPass::Render(RenderContext& renderContext, GPUTextureView* lightBuffer)
{
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    if (checkIfSkipPass())
    {
        // Skip pass (just clear buffer when doing debug preview)
        if (renderContext.View.Mode == ViewMode::Reflections)
            context->Clear(lightBuffer, Color::Black);
        return;
    }

    // Cache data
    auto& view = renderContext.View;
    bool useReflections = EnumHasAnyFlags(view.Flags, ViewFlags::Reflections);
    bool useSSR = EnumHasAnyFlags(view.Flags, ViewFlags::SSR) && renderContext.List->Settings.ScreenSpaceReflections.Intensity > ZeroTolerance;
    int32 probesCount = renderContext.List->EnvironmentProbes.Count();
    bool renderProbes = probesCount > 0;
    auto shader = _shader->GetShader();
    auto cb = shader->GetCB(0);

    // Check if no need to render reflection environment
    if (!useReflections || !(renderProbes || useSSR))
        return;
    PROFILE_GPU_CPU("Reflections");

    // Setup data
    const int32 width = renderContext.Buffers->GetWidth();
    const int32 height = renderContext.Buffers->GetHeight();
    Data data;
    GBufferPass::SetInputs(view, data.GBuffer);
    auto& ssrSettings = renderContext.List->Settings.ScreenSpaceReflections;
    data.SSRTexelSize = Float2(1.0f / (float)RenderTools::GetResolution(width, ssrSettings.ResolvePassResolution), 1.0f / (float)RenderTools::GetResolution(height, ssrSettings.ResolvePassResolution));

    // Bind GBuffer inputs
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    const bool depthBufferReadOnly = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView);
    GPUTextureView* depthBufferRTV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
    GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, depthBufferSRV);

    auto tempDesc = GPUTextureDescription::New2D(renderContext.Buffers->GetWidth(), renderContext.Buffers->GetHeight(), PixelFormat::R11G11B10_Float);
    auto reflectionsBuffer = RenderTargetPool::Get(tempDesc);
    RENDER_TARGET_POOL_SET_NAME(reflectionsBuffer, "Reflections");
    context->Clear(*reflectionsBuffer, Color::Black);

    // Reflection Probes pass
    if (renderProbes)
    {
        PROFILE_GPU_CPU("Env Probes");

        context->SetRenderTarget(depthBufferRTV, *reflectionsBuffer);

        // Sort probes by the radius
        Sorting::QuickSort(renderContext.List->EnvironmentProbes.Get(), renderContext.List->EnvironmentProbes.Count(), &SortProbes);

        // Render all env probes
        auto& sphereMesh = _sphereModel->LODs.Get()[0].Meshes.Get()[0];
        auto& boxMesh = _boxModel->LODs.Get()[0].Meshes.Get()[0];
        for (int32 i = 0; i < probesCount; i++)
        {
            const RenderEnvironmentProbeData& probe = renderContext.List->EnvironmentProbes.Get()[i];

            // Calculate world*view*projection matrix
            Matrix world, wvp;
            bool isViewInside;
            Float2 minMaxDepth;
            if (probe.BoxProjection)
            {
                OrientedBoundingBox bounds(probe.Radius, Transform(probe.Position, probe.Orientation, probe.Scale));
                minMaxDepth = RenderTools::GetDepthBounds(view, bounds);
                RenderTools::ComputeBoxModelDrawMatrix(renderContext.View, bounds, world, isViewInside);
            }
            else
            {
                minMaxDepth = RenderTools::GetDepthBounds(view, BoundingSphere(probe.Position, probe.Radius));
                RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, probe.Position, probe.Radius, world, isViewInside);
            }
            Matrix::Multiply(world, view.ViewProjection(), wvp);

            // Setup depth bounds (if device supports it)
            if (_depthBounds)
                context->SetDepthBounds(minMaxDepth.X, minMaxDepth.Y);

            // Pack probe properties buffer
            probe.SetShaderData(data.PData);
            Matrix::Transpose(wvp, data.WVP);

            // Render reflections
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);
            context->BindSR(4, probe.Texture);
            context->SetState(isViewInside ? _psProbeInside : _psProbe);
            if (probe.BoxProjection)
                boxMesh.Render(context);
            else
                sphereMesh.Render(context);
        }

        // Cleanup
        context->UnBindSR(4);
        context->ResetRenderTarget();
        if (_depthBounds)
            context->SetDepthBounds(0, 1);
    }

    // Screen Space Reflections pass
    GPUTexture* ssrBuffer = nullptr;
    if (useSSR)
    {
        ssrBuffer = ScreenSpaceReflectionsPass::Instance()->Render(renderContext, *reflectionsBuffer, lightBuffer);

        // Restore
        context->BindSR(0, renderContext.Buffers->GBuffer0);
        context->BindSR(1, renderContext.Buffers->GBuffer1);
        context->BindSR(2, renderContext.Buffers->GBuffer2);
        context->BindSR(3, depthBufferSRV);
        context->SetViewportAndScissors(renderContext.Task->GetViewport());
    }

    if (renderContext.View.Mode == ViewMode::Reflections)
    {
        // If SSR is in use, then draw it with alpha blending into reflections buffer
        if (ssrBuffer)
        {
            if (!_psDrawSSR)
            {
                _psDrawSSR = GPUDevice::Instance->CreatePipelineState();
                auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
                psDesc.BlendMode = BlendingMode::AlphaBlend;
                psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
                psDesc.PS = GPUDevice::Instance->QuadShader->GetPS("PS_CopyLinear");
                _psDrawSSR->Init(psDesc);
            }
            context->SetRenderTarget(*reflectionsBuffer);
            context->BindSR(0, ssrBuffer);
            context->SetState(_psDrawSSR);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();
        }

        // Override light buffer with the reflections buffer
        context->SetRenderTarget(lightBuffer);
        context->Draw(reflectionsBuffer);
    }
    else
    {
        // Combine reflections and light buffer (additive mode)
        if (_depthBounds)
        {
            context->SetRenderTarget(depthBufferRTV, lightBuffer);
            context->SetDepthBounds(0, RenderTools::DepthBoundMaxBackground);
        }
        else
            context->SetRenderTarget(lightBuffer);
        context->BindCB(0, cb);
        if (probesCount == 0 || !renderProbes)
            context->UpdateCB(cb, &data);
        context->BindSR(5, reflectionsBuffer);
        context->BindSR(6, _preIntegratedGF->GetTexture());
        context->BindSR(7, ssrBuffer);
        context->SetState(_psCombinePass);
        context->DrawFullscreenTriangle();
        if (_depthBounds)
            context->SetDepthBounds(0, 1);
    }

    RenderTargetPool::Release(ssrBuffer);
    RenderTargetPool::Release(reflectionsBuffer);
}
