// Copyright (c) Wojciech Figat. All rights reserved.

#include "ExponentialHeightFog.h"
#include "DirectionalLight.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Renderer/GBufferPass.h"
#include "Engine/Level/Scene/SceneRendering.h"

ExponentialHeightFog::ExponentialHeightFog(const SpawnParams& params)
    : Actor(params)
{
    _drawNoCulling = 1;
    _drawCategory = SceneRendering::PreRender;

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Fog"));
    if (_shader == nullptr)
    {
        LOG(Fatal, "Cannot load fog shader.");
    }
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ExponentialHeightFog, &ExponentialHeightFog::OnShaderReloading>(this);
#endif
}

void ExponentialHeightFog::Draw(RenderContext& renderContext)
{
    // Render only when shader is valid and fog can be rendered
    // Do not render exponential fog in orthographic views
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Fog)
        && EnumHasAnyFlags(renderContext.View.Pass, DrawPass::GBuffer)
        && _shader
        && _shader->IsLoaded()
        && renderContext.View.IsPerspectiveProjection())
    {
        if (_psFog.States[0] == nullptr)
            _psFog.CreatePipelineStates();
        if (!_psFog.States[0]->IsValid())
        {
            GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
            psDesc.DepthWriteEnable = false;
            psDesc.BlendMode.BlendEnable = true;
            psDesc.BlendMode.SrcBlend = BlendingMode::Blend::One;
            psDesc.BlendMode.DestBlend = BlendingMode::Blend::SrcAlpha;
            psDesc.BlendMode.BlendOp = BlendingMode::Operation::Add;
            psDesc.BlendMode.SrcBlendAlpha = BlendingMode::Blend::One;
            psDesc.BlendMode.DestBlendAlpha = BlendingMode::Blend::Zero;
            psDesc.BlendMode.BlendOpAlpha = BlendingMode::Operation::Add;
            psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
            if (_psFog.Create(psDesc, _shader->GetShader(), "PS_Fog"))
            {
                LOG(Warning, "Cannot create graphics pipeline state object for '{0}'.", ToString());
                return;
            }
        }

        // Register for Fog Pass
        renderContext.List->Fog = this;
    }
}

void ExponentialHeightFog::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(ExponentialHeightFog);

    SERIALIZE(FogDensity);
    SERIALIZE(FogHeightFalloff);
    SERIALIZE(FogInscatteringColor);
    SERIALIZE(FogMaxOpacity);
    SERIALIZE(StartDistance);
    SERIALIZE(FogCutoffDistance);

    SERIALIZE(DirectionalInscatteringLight);
    SERIALIZE(DirectionalInscatteringExponent);
    SERIALIZE(DirectionalInscatteringStartDistance);
    SERIALIZE(DirectionalInscatteringColor);

    SERIALIZE(VolumetricFogEnable);
    SERIALIZE(VolumetricFogScatteringDistribution);
    SERIALIZE(VolumetricFogAlbedo);
    SERIALIZE(VolumetricFogEmissive);
    SERIALIZE(VolumetricFogExtinctionScale);
    SERIALIZE(VolumetricFogDistance);
}

void ExponentialHeightFog::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(FogDensity);
    DESERIALIZE(FogHeightFalloff);
    DESERIALIZE(FogInscatteringColor);
    DESERIALIZE(FogMaxOpacity);
    DESERIALIZE(StartDistance);
    DESERIALIZE(FogCutoffDistance);

    DESERIALIZE(DirectionalInscatteringLight);
    DESERIALIZE(DirectionalInscatteringExponent);
    DESERIALIZE(DirectionalInscatteringStartDistance);
    DESERIALIZE(DirectionalInscatteringColor);

    DESERIALIZE(VolumetricFogEnable);
    DESERIALIZE(VolumetricFogScatteringDistribution);
    DESERIALIZE(VolumetricFogAlbedo);
    DESERIALIZE(VolumetricFogEmissive);
    DESERIALIZE(VolumetricFogExtinctionScale);
    DESERIALIZE(VolumetricFogDistance);
}

bool ExponentialHeightFog::HasContentLoaded() const
{
    return _shader && _shader->IsLoaded();
}

bool ExponentialHeightFog::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

void ExponentialHeightFog::GetVolumetricFogOptions(VolumetricFogOptions& result) const
{
    const float height = (float)GetPosition().Y;
    const float density = FogDensity / 1000.0f;
    const float heightFalloff = FogHeightFalloff / 1000.0f;

    result.Enable = VolumetricFogEnable;
    result.ScatteringDistribution = VolumetricFogScatteringDistribution;
    result.Albedo = VolumetricFogAlbedo * FogInscatteringColor;
    result.Emissive = VolumetricFogEmissive * (1.0f / 100.0f);
    result.ExtinctionScale = VolumetricFogExtinctionScale;
    result.Distance = VolumetricFogDistance;
    result.FogParameters = Float4(density, height, heightFalloff, 0.0f);
}

void ExponentialHeightFog::GetExponentialHeightFogData(const RenderView& view, ShaderExponentialHeightFogData& result) const
{
    const float height = (float)GetPosition().Y;
    const float density = FogDensity / 1000.0f;
    const float heightFalloff = FogHeightFalloff / 1000.0f;
    const float viewHeight = view.Position.Y;
    const bool useDirectionalLightInscattering = DirectionalInscatteringLight != nullptr;

    result.FogInscatteringColor = FogInscatteringColor.ToFloat3();
    result.FogMinOpacity = 1.0f - FogMaxOpacity;
    result.FogDensity = density;
    result.FogHeight = height;
    result.FogHeightFalloff = heightFalloff;
    result.FogAtViewPosition = density * Math::Pow(2.0f, Math::Clamp(-heightFalloff * (viewHeight - height), -125.f, 126.f));
    result.StartDistance = StartDistance;
    result.FogMinOpacity = 1.0f - FogMaxOpacity;
    result.FogCutoffDistance = FogCutoffDistance >= 0 ? FogCutoffDistance : view.Far + FogCutoffDistance;
    if (useDirectionalLightInscattering)
    {
        result.InscatteringLightDirection = -DirectionalInscatteringLight->GetDirection();
        result.DirectionalInscatteringColor = DirectionalInscatteringColor.ToFloat3();
        result.DirectionalInscatteringExponent = Math::Clamp(DirectionalInscatteringExponent, 0.000001f, 1000.0f);
        result.DirectionalInscatteringStartDistance = Math::Min(DirectionalInscatteringStartDistance, view.Far - 1.0f);
    }
    else
    {
        result.InscatteringLightDirection = Float3::Zero;
        result.DirectionalInscatteringColor = Float3::Zero;
        result.DirectionalInscatteringExponent = 4.0f;
        result.DirectionalInscatteringStartDistance = 0.0f;
    }
    result.ApplyDirectionalInscattering = useDirectionalLightInscattering ? 1.0f : 0.0f;
    result.VolumetricFogMaxDistance = VolumetricFogDistance;
}

GPU_CB_STRUCT(Data {
    ShaderGBufferData GBuffer;
    ShaderExponentialHeightFogData ExponentialHeightFog;
    });

void ExponentialHeightFog::DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output)
{
    auto integratedLightScattering = renderContext.Buffers->VolumetricFog;
    bool useVolumetricFog = integratedLightScattering != nullptr;

    // Setup shader inputs
    Data data;
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    GetExponentialHeightFogData(renderContext.View, data.ExponentialHeightFog);
    auto cb = _shader->GetShader()->GetCB(0);
    ASSERT(cb->GetSize() == sizeof(Data));
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, renderContext.Buffers->DepthBuffer);
    context->BindSR(1, integratedLightScattering ? integratedLightScattering->ViewVolume() : nullptr);

    // TODO: instead of rendering fullscreen triangle, draw quad transformed at the fog start distance (also it could use early depth discard)

    // Draw fog
    const int32 psIndex = (useVolumetricFog ? 1 : 0);
    context->SetState(_psFog.Get(psIndex));
    context->SetRenderTarget(output);
    context->DrawFullscreenTriangle();
}

void ExponentialHeightFog::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void ExponentialHeightFog::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void ExponentialHeightFog::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
