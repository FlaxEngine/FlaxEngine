// Copyright (c) Wojciech Figat. All rights reserved.

#include "Sky.h"
#include "DirectionalLight.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Content/Content.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/AtmospherePreCompute.h"
#include "Engine/Renderer/GBufferPass.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif

GPU_CB_STRUCT(Data {
    Matrix WVP;
    Float3 ViewOffset;
    float Padding;
    ShaderGBufferData GBuffer;
    ShaderAtmosphericFogData Fog;
    });

Sky::Sky(const SpawnParams& params)
    : Actor(params)
    , _shader(nullptr)
    , _psSky(nullptr)
    , _psFog(nullptr)
{
    _drawNoCulling = 1;
    _drawCategory = SceneRendering::PreRender;

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Sky"));
    if (_shader == nullptr)
    {
        LOG(Fatal, "Cannot load sky shader.");
        return;
    }
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<Sky, &Sky::OnShaderReloading>(this);
#endif
}

Sky::~Sky()
{
    SAFE_DELETE_GPU_RESOURCE(_psSky);
    SAFE_DELETE_GPU_RESOURCE(_psFog);
}

void Sky::InitConfig(ShaderAtmosphericFogData& config) const
{
    config.AtmosphericFogDensityScale = 1.0f;
    config.AtmosphericFogSunDiscScale = SunDiscScale;
    config.AtmosphericFogDistanceScale = 1;
    config.AtmosphericFogGroundOffset = 0;

    config.AtmosphericFogAltitudeScale = 1;
    config.AtmosphericFogStartDistance = 0;
    config.AtmosphericFogPower = 1;
    config.AtmosphericFogDistanceOffset = 0;

    config.AtmosphericFogSunPower = SunPower;
    config.AtmosphericFogDensityOffset = 0.0f;
#if USE_EDITOR
    if (IsRunningRadiancePass)
    config.AtmosphericFogSunPower *= IndirectLightingIntensity;
#endif

    if (SunLight)
    {
        config.AtmosphericFogSunDirection = -SunLight->GetDirection();
        config.AtmosphericFogSunColor = SunLight->Color.ToFloat3() * SunLight->Color.A;
    }
    else
    {
        config.AtmosphericFogSunDirection = Float3::UnitY;
        config.AtmosphericFogSunColor = Float3::One;
    }
}

void Sky::Draw(RenderContext& renderContext)
{
    if (HasContentLoaded() && EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Sky))
    {
        // Ensure to have pipeline state cache created
        if (_psSky == nullptr || _psFog == nullptr)
        {
            const auto shader = _shader->GetShader();

            // Create pipeline states
            if (_psSky == nullptr)
            {
                _psSky = GPUDevice::Instance->CreatePipelineState();

                GPUPipelineState::Description psDesc = GPUPipelineState::Description::Default;
                psDesc.VS = shader->GetVS("VS");
                psDesc.PS = shader->GetPS("PS_Sky");
                psDesc.CullMode = CullMode::Inverted;
                psDesc.DepthWriteEnable = false;
                psDesc.DepthClipEnable = false;
                psDesc.DepthFunc = ComparisonFunc::LessEqual;

                if (_psSky->Init(psDesc))
                {
                    LOG(Warning, "Cannot create graphics pipeline state object for '{0}'.", ToString());
                }
            }
            if (_psFog == nullptr)
            {
                _psFog = GPUDevice::Instance->CreatePipelineState();

                GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
                psDesc.PS = shader->GetPS("PS_Fog");
                psDesc.DepthWriteEnable = false;
                psDesc.DepthClipEnable = false;
                psDesc.BlendMode = BlendingMode::Additive;

                if (_psFog->Init(psDesc))
                {
                    LOG(Warning, "Cannot create graphics pipeline state object for '{0}'.", ToString());
                }
            }
        }

        // Register for the sky and fog pass
        renderContext.List->Sky = this;
        //if(renderContext.View.Flags & ViewFlags::Fog) != 0)
        //renderContext.List->AtmosphericFog = this; // TODO: finish atmosphere fog
    }
}

void Sky::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Sky);

    SERIALIZE_MEMBER(Sun, SunLight);
    SERIALIZE(SunDiscScale);
    SERIALIZE(SunPower);
    SERIALIZE(IndirectLightingIntensity);
}

void Sky::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Sun, SunLight);
    DESERIALIZE(SunDiscScale);
    DESERIALIZE(SunPower);
    DESERIALIZE(IndirectLightingIntensity);
}

bool Sky::HasContentLoaded() const
{
    return _shader && _shader->IsLoaded() && AtmospherePreCompute::GetCache(nullptr);
}

bool Sky::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

void Sky::DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output)
{
    // Get precomputed cache and bind it to the pipeline
    AtmosphereCache cache;
    if (!AtmospherePreCompute::GetCache(&cache))
        return;
    context->BindSR(4, cache.Transmittance);
    context->BindSR(5, cache.Irradiance);
    context->BindSR(6, cache.Inscatter->ViewVolume());

    // Bind GBuffer inputs
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);

    // Setup constants data
    Data data;
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    data.ViewOffset = renderContext.View.Origin + GetPosition();
    InitConfig(data.Fog);
    data.Fog.AtmosphericFogSunPower *= SunLight ? SunLight->Brightness : 1.0f;
    bool useSpecularLight = EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::SpecularLight);
    if (!useSpecularLight)
    {
        data.Fog.AtmosphericFogSunDiscScale = 0;
    }

    // Bind pipeline
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->SetState(_psFog);
    context->SetRenderTarget(output);
    context->DrawFullscreenTriangle();
}

bool Sky::IsDynamicSky() const
{
    return !IsStatic() || (SunLight && !SunLight->IsStatic());
}

float Sky::GetIndirectLightingIntensity() const
{
    return IndirectLightingIntensity;
}

void Sky::ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world)
{
    // Get precomputed cache and bind it to the pipeline
    AtmosphereCache cache;
    if (!AtmospherePreCompute::GetCache(&cache))
        return;
    context->BindSR(4, cache.Transmittance);
    context->BindSR(5, cache.Irradiance);
    context->BindSR(6, cache.Inscatter->ViewVolume());

    // Setup constants data
    Matrix m;
    Data data;
    Matrix::Multiply(world, renderContext.View.Frustum.GetMatrix(), m);
    Matrix::Transpose(m, data.WVP);
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    data.ViewOffset = renderContext.View.Origin + GetPosition();
    InitConfig(data.Fog);
    //data.Fog.AtmosphericFogSunPower *= SunLight ? SunLight->Brightness : 1.0f;
    bool useSpecularLight = EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::SpecularLight);
    if (!useSpecularLight)
    {
        // Hide sun disc if specular light is disabled
        data.Fog.AtmosphericFogSunDiscScale = 0;
    }

    // Bind pipeline
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->SetState(_psSky);
}

void Sky::EndPlay()
{
    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psSky);
    SAFE_DELETE_GPU_RESOURCE(_psFog);

    // Base
    Actor::EndPlay();
}

void Sky::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void Sky::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void Sky::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
