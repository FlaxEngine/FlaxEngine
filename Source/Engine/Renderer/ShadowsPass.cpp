// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ShadowsPass.h"
#include "GBufferPass.h"
#include "VolumetricFogPass.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif

#define NormalOffsetScaleTweak 100.0f
#define SpotLight_NearPlane 10.0f
#define PointLight_NearPlane 10.0f

PACK_STRUCT(struct Data{
    GBufferData GBuffer;
    LightData Light;
    LightShadowData LightShadow;
    Matrix WVP;
    Matrix ViewProjectionMatrix;
    Vector2 Dummy0;
    float ContactShadowsDistance;
    float ContactShadowsLength;
    });

ShadowsPass::ShadowsPass()
    : _shader(nullptr)
    , _shadowMapsSizeCSM(0)
    , _shadowMapsSizeCube(0)
    , _shadowMapCSM(nullptr)
    , _shadowMapCube(nullptr)
    , _currentShadowMapsQuality((Quality)((int32)Quality::Ultra + 1))
    , _sphereModel(nullptr)
    , maxShadowsQuality(0)
{
}

uint64 ShadowsPass::GetShadowMapsMemoryUsage() const
{
    uint64 result = 0;

    if (_shadowMapCSM)
        result += _shadowMapCSM->GetMemoryUsage();
    if (_shadowMapCube)
        result += _shadowMapCube->GetMemoryUsage();

    return result;
}

String ShadowsPass::ToString() const
{
    return TEXT("ShadowsPass");
}

bool ShadowsPass::Init()
{
    // Create pipeline states
    _psShadowDir.CreatePipelineStates();
    _psShadowPoint.CreatePipelineStates();
    _psShadowSpot.CreatePipelineStates();

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Shadows"));
    _sphereModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/SphereLowPoly"));
    if (_shader == nullptr || _sphereModel == nullptr)
    {
        return true;
    }

    // Create shadow maps
    _shadowMapCSM = GPUDevice::Instance->CreateTexture(TEXT("Shadow Map CSM"));
    _shadowMapCube = GPUDevice::Instance->CreateTexture(TEXT("Shadow Map Cube"));

#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ShadowsPass, &ShadowsPass::OnShaderReloading>(this);
#endif

    // If GPU doesn't support linear sampling for the shadow map then fallback to the single sample on lowest quality
    const auto formatTexture = PixelFormatExtensions::FindShaderResourceFormat(SHADOW_MAPS_FORMAT, false);
    const auto formatFeaturesDepth = GPUDevice::Instance->GetFormatFeatures(SHADOW_MAPS_FORMAT);
    const auto formatFeaturesTexture = GPUDevice::Instance->GetFormatFeatures(formatTexture);
    _supportsShadows = FORMAT_FEATURES_ARE_SUPPORTED(formatFeaturesDepth.Support, FormatSupport::DepthStencil | FormatSupport::Texture2D)
            && FORMAT_FEATURES_ARE_SUPPORTED(formatFeaturesTexture.Support, FormatSupport::ShaderSample | FormatSupport::ShaderSampleComparison);
    if (!_supportsShadows)
    {
        LOG(Warning, "GPU doesn't support shadows rendering");
        LOG(Warning, "Format: {0} features support: {1}", (int32)SHADOW_MAPS_FORMAT, (uint32)formatFeaturesDepth.Support);
        LOG(Warning, "Format: {0} features support: {1}", (int32)formatTexture, (uint32)formatFeaturesTexture.Support);
    }

    return false;
}

bool ShadowsPass::setupResources()
{
    // Wait for the assets
    if (!_sphereModel->CanBeRendered() || !_shader->IsLoaded())
        return true;
    auto shader = _shader->GetShader();

    // Validate shader constant buffers sizes
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psShadowPoint.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.CullMode = CullMode::TwoSided;
        psDesc.VS = shader->GetVS("VS_Model");
        if (_psShadowPoint.Create(psDesc, shader, "PS_PointLight"))
            return true;
    }
    if (!_psShadowDir.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        if (_psShadowDir.Create(psDesc, shader, "PS_DirLight"))
            return true;
    }
    if (!_psShadowSpot.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.CullMode = CullMode::TwoSided;
        psDesc.VS = shader->GetVS("VS_Model");
        if (_psShadowSpot.Create(psDesc, shader, "PS_SpotLight"))
            return true;
    }

    return false;
}

void ShadowsPass::updateShadowMapSize()
{
    // Temporary data
    int32 newSizeCSM = 0;
    int32 newSizeCube = 0;

    // Select new size
    _currentShadowMapsQuality = Graphics::ShadowMapsQuality;
    if (_supportsShadows)
    {
        switch (_currentShadowMapsQuality)
        {
        case Quality::Ultra:
            newSizeCSM = 2048;
            newSizeCube = 1024;
            break;
        case Quality::High:
            newSizeCSM = 1024;
            newSizeCube = 1024;
            break;
        case Quality::Medium:
            newSizeCSM = 1024;
            newSizeCube = 512;
            break;
        case Quality::Low:
            newSizeCSM = 512;
            newSizeCube = 256;
            break;
        }
    }

    // Check if size will change
    if (newSizeCSM > 0 && newSizeCSM != _shadowMapsSizeCSM)
    {
        if (_shadowMapCSM->Init(GPUTextureDescription::New2D(newSizeCSM, newSizeCSM, SHADOW_MAPS_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil, 1, MAX_CSM_CASCADES)))
        {
            LOG(Fatal, "Cannot setup shadow map '{0}' Size: {1}, format: {2}.", TEXT("CSM"), newSizeCSM, (int32)SHADOW_MAPS_FORMAT);
            return;
        }
        _shadowMapsSizeCSM = newSizeCSM;
    }
    if (newSizeCube > 0 && newSizeCube != _shadowMapsSizeCube)
    {
        if (_shadowMapCube->Init(GPUTextureDescription::NewCube(newSizeCube, SHADOW_MAPS_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil)))
        {
            LOG(Fatal, "Cannot setup shadow map '{0}' Size: {1}, format: {2}.", TEXT("Cube"), newSizeCube, (int32)SHADOW_MAPS_FORMAT);
            return;
        }
        _shadowMapsSizeCube = newSizeCube;
    }
}

void ShadowsPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psShadowDir.Delete();
    _psShadowPoint.Delete();
    _psShadowSpot.Delete();
    _shader = nullptr;
    _sphereModel = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_shadowMapCSM);
    SAFE_DELETE_GPU_RESOURCE(_shadowMapCube);
}

bool ShadowsPass::CanRenderShadow(RenderContext& renderContext, const RendererPointLightData& light)
{
    const Vector3 lightPosition = light.Position;
    const float dstLightToView = Vector3::Distance(lightPosition, renderContext.View.Position);

    // Fade shadow on distance
    const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

    return fade > ZeroTolerance && _supportsShadows;
}

bool ShadowsPass::CanRenderShadow(RenderContext& renderContext, const RendererSpotLightData& light)
{
    const Vector3 lightPosition = light.Position;
    const float dstLightToView = Vector3::Distance(lightPosition, renderContext.View.Position);

    // Fade shadow on distance
    const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

    return fade > ZeroTolerance && _supportsShadows;
}

bool ShadowsPass::CanRenderShadow(RenderContext& renderContext, const RendererDirectionalLightData& light)
{
    return _supportsShadows;
}

void ShadowsPass::Prepare(RenderContext& renderContext, GPUContext* context)
{
    ASSERT(IsReady());
    auto& view = renderContext.View;
    const auto shader = _shader->GetShader();

    const auto shadowMapsQuality = Graphics::ShadowMapsQuality;
    if (shadowMapsQuality != _currentShadowMapsQuality)
        updateShadowMapSize();
    auto shadowsQuality = Graphics::ShadowsQuality;
    maxShadowsQuality = Math::Min<int32>(static_cast<int32>(shadowsQuality), static_cast<int32>(view.MaxShadowsQuality));

    // Use the current render view to sync model LODs with the shadow maps rendering stage
    _shadowContext.LodProxyView = &renderContext.View;

    // Prepare properties
    auto& shadowView = _shadowContext.View;
    shadowView.Flags = view.Flags;
    shadowView.StaticFlagsMask = view.StaticFlagsMask;
    shadowView.IsOfflinePass = view.IsOfflinePass;
    shadowView.ModelLODBias = view.ModelLODBias + view.ShadowModelLODBias;
    shadowView.ModelLODDistanceFactor = view.ModelLODDistanceFactor * view.ShadowModelLODDistanceFactor;
    shadowView.Pass = DrawPass::Depth;
    _shadowContext.List = &_shadowCache;
}

void ShadowsPass::RenderShadow(RenderContext& renderContext, RendererPointLightData& light, GPUTextureView* shadowMask)
{
    const float sphereModelScale = 3.0f;

    PROFILE_GPU_CPU("Shadow");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& view = renderContext.View;
    auto shader = _shader->GetShader();
    Data sperLight;
    float lightRadius = light.Radius;
    Vector3 lightPosition = light.Position;
    Vector3 lightDirection = light.Direction;
    float dstLightToView = Vector3::Distance(lightPosition, view.Position);

    // TODO: here we can use lower shadows quality based on light distance to view (LOD switching) and per light setting for max quality
    int32 shadowQuality = maxShadowsQuality;

    // Fade shadow on distance
    const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

    // Set up GPU context and render view
    const auto shadowMapsSizeCube = (float)_shadowMapsSizeCube;
    context->SetViewportAndScissors(shadowMapsSizeCube, shadowMapsSizeCube);
    _shadowContext.View.SetUpCube(PointLight_NearPlane, lightRadius, lightPosition);
    _shadowContext.View.PrepareCache(_shadowContext, shadowMapsSizeCube, shadowMapsSizeCube, Vector2::Zero);

    // Render depth to all 6 faces of the cube map
    for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        // Set up view
        _shadowCache.Clear();
        _shadowContext.View.SetFace(faceIndex);
        Matrix::Transpose(_shadowContext.View.ViewProjection(), sperLight.LightShadow.ShadowVP[faceIndex]);

        // Set render target
        auto rt = _shadowMapCube->View(faceIndex);
        context->ResetSR();
        context->SetRenderTarget(rt, static_cast<GPUTextureView*>(nullptr));
        context->ClearDepth(rt);

        // Render actors to the shadow map
        renderContext.Task->OnCollectDrawCalls(_shadowContext);
        _shadowCache.SortDrawCalls(_shadowContext, false, DrawCallsListType::Depth);
        _shadowCache.ExecuteDrawCalls(_shadowContext, DrawCallsListType::Depth);
    }

    // Restore GPU context
    context->ResetSR();
    context->ResetRenderTarget();
    const Viewport viewport = renderContext.Task->GetViewport();
    context->SetViewportAndScissors(viewport);
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);
    context->BindSR(4, renderContext.Buffers->GBuffer3);

    // Setup shader data
    GBufferPass::SetInputs(view, sperLight.GBuffer);
    light.SetupLightData(&sperLight.Light, view, true);
    sperLight.LightShadow.ShadowMapSize = shadowMapsSizeCube;
    sperLight.LightShadow.Sharpness = light.ShadowsSharpness;
    sperLight.LightShadow.Fade = Math::Saturate(light.ShadowsStrength * fade);
    sperLight.LightShadow.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / shadowMapsSizeCube);
    sperLight.LightShadow.Bias = light.ShadowsDepthBias;
    sperLight.LightShadow.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    sperLight.LightShadow.NumCascades = 1;
    sperLight.LightShadow.CascadeSplits = Vector4::Zero;
    Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
    sperLight.ContactShadowsDistance = light.ShadowsDistance;
    sperLight.ContactShadowsLength = view.Flags & ViewFlags::ContactShadows ? light.ContactShadowsLength : 0.0f;

    // Calculate world view projection matrix for the light sphere
    Matrix world, wvp, matrix;
    Matrix::Scaling(lightRadius * sphereModelScale, wvp);
    Matrix::Translation(lightPosition, matrix);
    Matrix::Multiply(wvp, matrix, world);
    Matrix::Multiply(world, view.ViewProjection(), wvp);
    Matrix::Transpose(wvp, sperLight.WVP);

    // Render shadow in screen space
    context->UpdateCB(shader->GetCB(0), &sperLight);
    context->BindCB(0, shader->GetCB(0));
    context->BindCB(1, shader->GetCB(1));
    context->BindSR(5, _shadowMapCube->ViewArray());
    context->SetRenderTarget(shadowMask);
    context->SetState(_psShadowPoint.Get(shadowQuality + (sperLight.ContactShadowsLength > ZeroTolerance ? 4 : 0)));
    _sphereModel->Render(context);

    // Cleanup
    context->ResetRenderTarget();
    context->UnBindSR(5);

    // Render volumetric light with shadow
    VolumetricFogPass::Instance()->RenderLight(renderContext, context, light, _shadowMapCube->ViewArray(), sperLight.LightShadow);
}

void ShadowsPass::RenderShadow(RenderContext& renderContext, RendererSpotLightData& light, GPUTextureView* shadowMask)
{
    const float sphereModelScale = 3.0f;

    PROFILE_GPU_CPU("Shadow");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& view = renderContext.View;
    auto shader = _shader->GetShader();
    Data sperLight;
    float lightRadius = light.Radius;
    Vector3 lightPosition = light.Position;
    Vector3 lightDirection = light.Direction;
    float dstLightToView = Vector3::Distance(lightPosition, view.Position);

    // TODO: here we can use lower shadows quality based on light distance to view (LOD switching) and per light setting for max quality
    int32 shadowQuality = maxShadowsQuality;

    // Fade shadow on distance
    const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

    // Set up GPU context and render view
    const auto shadowMapsSizeCube = (float)_shadowMapsSizeCube;
    context->SetViewportAndScissors(shadowMapsSizeCube, shadowMapsSizeCube);
    _shadowContext.View.SetProjector(SpotLight_NearPlane, lightRadius, lightPosition, lightDirection, light.UpVector, light.OuterConeAngle * 2.0f);
    _shadowContext.View.PrepareCache(_shadowContext, shadowMapsSizeCube, shadowMapsSizeCube, Vector2::Zero);

    // Render depth to all 1 face of the cube map
    const int32 cubeFaceIndex = 0;
    {
        // Set up view
        _shadowCache.Clear();
        Matrix::Transpose(_shadowContext.View.ViewProjection(), sperLight.LightShadow.ShadowVP[cubeFaceIndex]);

        // Set render target
        auto rt = _shadowMapCube->View(cubeFaceIndex);
        context->ResetSR();
        context->SetRenderTarget(rt, static_cast<GPUTextureView*>(nullptr));
        context->ClearDepth(rt);

        // Render actors to the shadow map
        renderContext.Task->OnCollectDrawCalls(_shadowContext);
        _shadowCache.SortDrawCalls(_shadowContext, false, DrawCallsListType::Depth);
        _shadowCache.ExecuteDrawCalls(_shadowContext, DrawCallsListType::Depth);
    }

    // Restore GPU context
    context->ResetSR();
    context->ResetRenderTarget();
    const Viewport viewport = renderContext.Task->GetViewport();
    context->SetViewportAndScissors(viewport);
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);
    context->BindSR(4, renderContext.Buffers->GBuffer3);

    // Setup shader data
    GBufferPass::SetInputs(view, sperLight.GBuffer);
    light.SetupLightData(&sperLight.Light, view, true);
    sperLight.LightShadow.ShadowMapSize = shadowMapsSizeCube;
    sperLight.LightShadow.Sharpness = light.ShadowsSharpness;
    sperLight.LightShadow.Fade = Math::Saturate(light.ShadowsStrength * fade);
    sperLight.LightShadow.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / shadowMapsSizeCube);
    sperLight.LightShadow.Bias = light.ShadowsDepthBias;
    sperLight.LightShadow.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    sperLight.LightShadow.NumCascades = 1;
    sperLight.LightShadow.CascadeSplits = Vector4::Zero;
    Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
    sperLight.ContactShadowsDistance = light.ShadowsDistance;
    sperLight.ContactShadowsLength = view.Flags & ViewFlags::ContactShadows ? light.ContactShadowsLength : 0.0f;

    // Calculate world view projection matrix for the light sphere
    Matrix world, wvp, matrix;
    Matrix::Scaling(lightRadius * sphereModelScale, wvp);
    Matrix::Translation(lightPosition, matrix);
    Matrix::Multiply(wvp, matrix, world);
    Matrix::Multiply(world, view.ViewProjection(), wvp);
    Matrix::Transpose(wvp, sperLight.WVP);

    // Render shadow in screen space
    context->UpdateCB(shader->GetCB(0), &sperLight);
    context->BindCB(0, shader->GetCB(0));
    context->BindCB(1, shader->GetCB(1));
    context->BindSR(5, _shadowMapCube->View(cubeFaceIndex));
    context->SetRenderTarget(shadowMask);
    context->SetState(_psShadowSpot.Get(shadowQuality + (sperLight.ContactShadowsLength > ZeroTolerance ? 4 : 0)));
    _sphereModel->Render(context);

    // Cleanup
    context->ResetRenderTarget();
    context->UnBindSR(5);

    // Render volumetric light with shadow
    VolumetricFogPass::Instance()->RenderLight(renderContext, context, light, _shadowMapCube->View(cubeFaceIndex), sperLight.LightShadow);
}

void ShadowsPass::RenderShadow(RenderContext& renderContext, RendererDirectionalLightData& light, int32 index, GPUTextureView* shadowMask)
{
    PROFILE_GPU_CPU("Shadow");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& view = renderContext.View;
    auto mainCache = renderContext.List;
    auto shader = _shader->GetShader();
    Data sperLight;
    Vector3 lightDirection = light.Direction;
    float shadowsDistance = Math::Min(view.Far, light.ShadowsDistance);
    int32 csmCount = Math::Clamp(light.CascadeCount, 0, MAX_CSM_CASCADES);
    bool blendCSM = Graphics::AllowCSMBlending;
#if USE_EDITOR
    if (IsRunningRadiancePass)
        blendCSM = false;
#endif

    // Views with orthographic cameras cannot use cascades, we force it to 1 shadow map here
    if (view.Projection.M44 == 1.0f)
        csmCount = 1;

    // Calculate cascade splits
    auto cameraNear = view.Near;
    auto cameraFar = view.Far;
    auto cameraRange = cameraFar - cameraNear;
    float minDistance;
    float maxDistance;
    float cascadeSplits[MAX_CSM_CASCADES];
    {
        // TODO: use HiZ and get view min/max range to fit cascades better
        minDistance = cameraNear;
        maxDistance = cameraNear + shadowsDistance;

        // TODO: expose partition mode?
        enum class PartitionMode
        {
            Manual = 0,
            Logarithmic = 1,
            PSSM = 2,
        };
        PartitionMode partitionMode = PartitionMode::Manual;
        float pssmFactor = 0.5f;
        float splitDistance0 = 0.05f;
        float splitDistance1 = 0.15f;
        float splitDistance2 = 0.50f;
        float splitDistance3 = 1.00f;

        // Compute the split distances based on the partitioning mode
        if (partitionMode == PartitionMode::Manual)
        {
            if (csmCount == 1)
            {
                cascadeSplits[0] = minDistance + splitDistance3 * maxDistance;
            }
            else if (csmCount == 2)
            {
                cascadeSplits[0] = minDistance + splitDistance1 * maxDistance;
                cascadeSplits[1] = minDistance + splitDistance3 * maxDistance;
            }
            else if (csmCount == 3)
            {
                cascadeSplits[0] = minDistance + splitDistance0 * maxDistance;
                cascadeSplits[1] = minDistance + splitDistance2 * maxDistance;
                cascadeSplits[2] = minDistance + splitDistance3 * maxDistance;
            }
            else if (csmCount == 4)
            {
                cascadeSplits[0] = minDistance + splitDistance0 * maxDistance;
                cascadeSplits[1] = minDistance + splitDistance1 * maxDistance;
                cascadeSplits[2] = minDistance + splitDistance2 * maxDistance;
                cascadeSplits[3] = minDistance + splitDistance3 * maxDistance;
            }
        }
        else if (partitionMode == PartitionMode::Logarithmic || partitionMode == PartitionMode::PSSM)
        {
            const float lambda = partitionMode == PartitionMode::PSSM ? pssmFactor : 1.0f;

            const auto range = maxDistance - minDistance;
            const auto ratio = maxDistance / minDistance;
            const auto logRatio = Math::Clamp(1.0f - lambda, 0.0f, 1.0f);
            for (int32 cascadeLevel = 0; cascadeLevel < csmCount; cascadeLevel++)
            {
                // Compute cascade split (between znear and zfar)
                const float distribute = static_cast<float>(cascadeLevel + 1) / csmCount;
                float logZ = static_cast<float>(minDistance * powf(ratio, distribute));
                float uniformZ = minDistance + range * distribute;
                cascadeSplits[cascadeLevel] = Math::Lerp(uniformZ, logZ, logRatio);
            }
        }

        // Convert distance splits to ratios cascade in the range [0, 1]
        for (int32 i = 0; i < MAX_CSM_CASCADES; i++)
        {
            cascadeSplits[i] = (cascadeSplits[i] - cameraNear) / cameraRange;
        }
    }

    // Select best Up vector
    Vector3 upDirection = Vector3::UnitX;
    Vector3 VectorUps[] = { Vector3::UnitY, Vector3::UnitX, Vector3::UnitZ };
    for (int32 i = 0; i < ARRAY_COUNT(VectorUps); i++)
    {
        const Vector3 vectorUp = VectorUps[i];
        if (Math::Abs(Vector3::Dot(lightDirection, vectorUp)) < (1.0f - 0.0001f))
        {
            const Vector3 side = Vector3::Normalize(Vector3::Cross(vectorUp, lightDirection));
            upDirection = Vector3::Normalize(Vector3::Cross(lightDirection, side));
            break;
        }
    }

    // Temporary data
    Vector3 frustumCorners[8];
    Matrix shadowView, shadowProjection, shadowVP;

    // Set up GPU context and render view
    const auto shadowMapsSizeCSM = (float)_shadowMapsSizeCSM;
    context->SetViewportAndScissors(shadowMapsSizeCSM, shadowMapsSizeCSM);
    _shadowContext.View.PrepareCache(_shadowContext, shadowMapsSizeCSM, shadowMapsSizeCSM, Vector2::Zero);

    // Create the different view and projection matrices for each split
    float splitMinRatio = 0;
    float splitMaxRatio = (minDistance - cameraNear) / cameraRange;
    for (int32 cascadeIndex = 0; cascadeIndex < csmCount; cascadeIndex++)
    {
        // Cascade splits
        const auto oldSplitMinRatio = splitMinRatio;
        splitMinRatio = splitMaxRatio;
        splitMaxRatio = cascadeSplits[cascadeIndex];

        // Calculate cascade split frustum corners in view space
        for (int32 j = 0; j < 4; j++)
        {
            // Calculate frustum in WS and VS
            float overlap = 0;
            if (blendCSM)
                overlap = 0.2f * (splitMinRatio - oldSplitMinRatio);

            const auto frustumRangeVS = mainCache->FrustumCornersVs[j + 4] - mainCache->FrustumCornersVs[j];
            frustumCorners[j] = mainCache->FrustumCornersVs[j] + frustumRangeVS * (splitMinRatio - overlap);
            frustumCorners[j + 4] = mainCache->FrustumCornersVs[j] + frustumRangeVS * splitMaxRatio;
        }

        // Perform stabilization (using Projection Snapping)
        Vector3 cascadeMinBoundLS;
        Vector3 cascadeMaxBoundLS;
        Vector3 target;
        {
            // Make sure we are using the same direction when stabilizing
            BoundingSphere boundingVS;
            BoundingSphere::FromPoints(frustumCorners, ARRAY_COUNT(frustumCorners), boundingVS);

            // Compute bounding box center
            Vector3::TransformCoordinate(boundingVS.Center, view.IV, target);

            cascadeMaxBoundLS = Vector3(boundingVS.Radius);
            cascadeMinBoundLS = -cascadeMaxBoundLS;
        }

        const auto nearClip = -2000.0f;
        const auto farClip = cascadeMaxBoundLS.Z - cascadeMinBoundLS.Z;

        // Create shadow view matrix
        Matrix::LookAt(target - lightDirection * cascadeMaxBoundLS.Z, target, upDirection, shadowView);

        // Create viewport for culling with extended near/far planes due to culling issues
        Matrix cullingVP;
        {
            Matrix::OrthoOffCenter(cascadeMinBoundLS.X, cascadeMaxBoundLS.X, cascadeMinBoundLS.Y, cascadeMaxBoundLS.Y, -100000.0f, farClip + 100000.0f, shadowProjection);
            Matrix::Multiply(shadowView, shadowProjection, cullingVP);
        }

        // Create shadow projection matrix
        Matrix::OrthoOffCenter(cascadeMinBoundLS.X, cascadeMaxBoundLS.X, cascadeMinBoundLS.Y, cascadeMaxBoundLS.Y, nearClip, farClip, shadowProjection);

        // Construct shadow matrix (View * Projection)
        Matrix::Multiply(shadowView, shadowProjection, shadowVP);

        // Stabilize the shadow matrix on the projection
        {
            auto shadowPixelPosition = shadowVP.GetTranslation() * (shadowMapsSizeCSM * 0.5f);
            shadowPixelPosition.Z = 0;
            const auto shadowPixelPositionRounded = Vector3(Math::Round(shadowPixelPosition.X), Math::Round(shadowPixelPosition.Y), 0.0f);
            auto shadowPixelOffset = Vector4(shadowPixelPositionRounded - shadowPixelPosition, 0.0f);
            shadowPixelOffset *= 2.0f / shadowMapsSizeCSM;
            shadowProjection.SetRow4(shadowProjection.GetRow4() + shadowPixelOffset);
            Matrix::Multiply(shadowView, shadowProjection, shadowVP);
        }

        // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
        {
            const Matrix T(
                0.5f, 0.0f, 0.0f, 0.0f,
                0.0f, -0.5f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.5f, 0.5f, 0.0f, 1.0f);
            Matrix m;
            Matrix::Multiply(shadowVP, T, m);
            Matrix::Transpose(m, sperLight.LightShadow.ShadowVP[cascadeIndex]);
        }

        // Set up view and cache
        _shadowCache.Clear();
        _shadowContext.View.Position = -lightDirection * shadowsDistance + view.Position;
        _shadowContext.View.Direction = lightDirection;
        _shadowContext.View.SetUp(shadowView, shadowProjection);
        _shadowContext.View.CullingFrustum.SetMatrix(cullingVP);

        // Set render target
        const auto rt = _shadowMapCSM->View(cascadeIndex);
        context->ResetSR();
        context->SetRenderTarget(rt, static_cast<GPUTextureView*>(nullptr));
        context->ClearDepth(rt);

        // Render actors to the shadow map
        renderContext.Task->OnCollectDrawCalls(_shadowContext);
        _shadowCache.SortDrawCalls(_shadowContext, false, DrawCallsListType::Depth);
        _shadowCache.ExecuteDrawCalls(_shadowContext, DrawCallsListType::Depth);
    }

    // Restore GPU context
    context->ResetSR();
    context->ResetRenderTarget();
    const Viewport viewport = renderContext.Task->GetViewport();
    context->SetViewportAndScissors(viewport);
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);
    context->BindSR(4, renderContext.Buffers->GBuffer3);

    // Setup shader data
    GBufferPass::SetInputs(view, sperLight.GBuffer);
    light.SetupLightData(&sperLight.Light, view, true);
    sperLight.LightShadow.ShadowMapSize = shadowMapsSizeCSM;
    sperLight.LightShadow.Sharpness = light.ShadowsSharpness;
    sperLight.LightShadow.Fade = Math::Saturate(light.ShadowsStrength);
    sperLight.LightShadow.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / shadowMapsSizeCSM);
    sperLight.LightShadow.Bias = light.ShadowsDepthBias;
    sperLight.LightShadow.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    sperLight.LightShadow.NumCascades = csmCount;
    sperLight.LightShadow.CascadeSplits = view.Near + Vector4(cascadeSplits) * cameraRange;
    Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
    sperLight.ContactShadowsDistance = light.ShadowsDistance;
    sperLight.ContactShadowsLength = view.Flags & ViewFlags::ContactShadows ? light.ContactShadowsLength : 0.0f;

    // Render shadow in screen space
    context->UpdateCB(shader->GetCB(0), &sperLight);
    context->BindCB(0, shader->GetCB(0));
    context->BindCB(1, shader->GetCB(1));
    context->BindSR(5, _shadowMapCSM->ViewArray());
    context->SetRenderTarget(shadowMask);
    context->SetState(_psShadowDir.Get(maxShadowsQuality + static_cast<int32>(Quality::MAX) * blendCSM + (sperLight.ContactShadowsLength > ZeroTolerance ? 8 : 0)));
    context->DrawFullscreenTriangle();

    // Cleanup
    context->ResetRenderTarget();
    context->UnBindSR(5);

    // Cache params for the volumetric fog or other effects that use dir light shadow sampling
    LastDirLightIndex = index;
    LastDirLightShadowMap = _shadowMapCSM->ViewArray();
    LastDirLight = sperLight.LightShadow;
}
