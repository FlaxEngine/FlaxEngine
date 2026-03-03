// Copyright (c) Wojciech Figat. All rights reserved.

#include "AtmospherePreCompute.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Platform/Window.h"
#include "RendererPass.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/RenderTargetPool.h"

// Amount of frames to wait for data from atmosphere precompute job
#define ATMOSPHERE_PRECOMPUTE_LATENCY_FRAMES 1

// Amount of frames to hold shader resources in memory before auto-release
#define ATMOSPHERE_PRECOMPUTE_AUTO_RELEASE_FRAMES 70

const float DensityHeight = 0.5f;
const int32 MaxScatteringOrder = 4;

const int32 TransmittanceTexWidth = 256;
const int32 TransmittanceTexHeight = 64;

const int32 IrradianceTexWidth = 64;
const int32 IrradianceTexHeight = 16;

const int32 InscatterMuNum = 128;
const int32 InscatterMuSNum = 32;
const int32 InscatterNuNum = 8;
const int32 InscatterAltitudeSampleNum = 4;

const int32 InscatterWidth = InscatterMuSNum * InscatterNuNum;
const int32 InscatterHeight = InscatterMuNum;
const int32 InscatterDepth = InscatterAltitudeSampleNum;

const static float RadiusScale = 1;
const static float RadiusGround = 6360 * RadiusScale;
const static float RadiusAtmosphere = 6420 * RadiusScale;

GPU_CB_STRUCT(Data {
    float First;
    float AtmosphereR;
    int AtmosphereLayer;
    float Dummy0;
    Float4 dhdh;
    });

namespace AtmospherePreComputeImpl
{
    bool _isUpdatePending = false;
    bool _isReadyForCompute = false;
    bool _hasDataCached = false;
    uint64 _dataCachedFrameNumber;

    AssetReference<Shader> _shader;
    GPUPipelineState* _psTransmittance = nullptr;
    GPUPipelineState* _psIrradiance1 = nullptr;
    GPUPipelineState* _psIrradianceN = nullptr;
    GPUPipelineState* _psCopyIrradianceAdd = nullptr;
    GPUPipelineState* _psInscatter1_A = nullptr;
    GPUPipelineState* _psInscatter1_B = nullptr;
    GPUPipelineState* _psCopyInscatter1 = nullptr;
    GPUPipelineState* _psCopyInscatterNAdd = nullptr;
    GPUPipelineState* _psInscatterS = nullptr;
    GPUPipelineState* _psInscatterN = nullptr;
    SceneRenderTask* _task = nullptr;

    GPUTexture* AtmosphereTransmittance = nullptr;
    GPUTexture* AtmosphereIrradiance = nullptr;
    GPUTexture* AtmosphereInscatter = nullptr;

    uint64 _updateFrameNumber;
    bool _wasCancelled;

    FORCE_INLINE bool isUpdateSynced()
    {
        return _updateFrameNumber > 0 && _updateFrameNumber + ATMOSPHERE_PRECOMPUTE_LATENCY_FRAMES <= Engine::FrameCount;
    }

    void Render(RenderTask* task, GPUContext* context);
}

using namespace AtmospherePreComputeImpl;

class AtmospherePreComputeService : public EngineService
{
public:

    AtmospherePreComputeService()
        : EngineService(TEXT("Atmosphere Pre Compute"), 50)
    {
    }

    void Update() override;
    void Dispose() override;
};

AtmospherePreComputeService AtmospherePreComputeServiceInstance;

bool AtmospherePreCompute::GetCache(AtmosphereCache* cache)
{
    if (_hasDataCached)
    {
        if (cache)
        {
            cache->Transmittance = AtmosphereTransmittance;
            cache->Irradiance = AtmosphereIrradiance;
            cache->Inscatter = AtmosphereInscatter;
        }
    }
    else if (_task == nullptr || !_task->Enabled)
    {
        _isUpdatePending = true;
    }
    return _hasDataCached;
}

bool InitAtmospherePreCompute()
{
    if (_isReadyForCompute)
        return false;
    PROFILE_CPU();

    // Load shader
    if (_shader == nullptr)
    {
        LOG(Warning, "Failed to load AtmospherePreCompute shader!");
        return true;
    }
    if (_shader->WaitForLoaded())
    {
        LOG(Warning, "Loading AtmospherePreCompute shader timeout!");
        return true;
    }
    auto shader = _shader->GetShader();
    ASSERT(shader->GetCB(0) != nullptr);
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
    auto device = GPUDevice::Instance;

    // Create pipeline stages
    _psTransmittance = device->CreatePipelineState();
    _psIrradiance1 = device->CreatePipelineState();
    _psIrradianceN = device->CreatePipelineState();
    _psCopyIrradianceAdd = device->CreatePipelineState();
    _psInscatter1_A = device->CreatePipelineState();
    _psInscatter1_B = device->CreatePipelineState();
    _psCopyInscatter1 = device->CreatePipelineState();
    _psInscatterS = device->CreatePipelineState();
    _psInscatterN = device->CreatePipelineState();
    _psCopyInscatterNAdd = device->CreatePipelineState();
    GPUPipelineState::Description psDesc, psDescLayers;
    psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    psDescLayers = psDesc;
    {
        psDesc.PS = shader->GetPS("PS_Transmittance");
        if (_psTransmittance->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_Irradiance1");
        if (_psIrradiance1->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_IrradianceN");
        if (_psIrradianceN->Init(psDesc))
            return true;
    }
    {
        psDescLayers.PS = shader->GetPS("PS_Inscatter1_A");
        if (_psInscatter1_A->Init(psDescLayers))
            return true;
    }
    {
        psDescLayers.PS = shader->GetPS("PS_Inscatter1_B");
        if (_psInscatter1_B->Init(psDescLayers))
            return true;
    }
    {
        psDescLayers.PS = shader->GetPS("PS_CopyInscatter1");
        if (_psCopyInscatter1->Init(psDescLayers))
            return true;
    }
    {
        psDescLayers.PS = shader->GetPS("PS_InscatterS");
        if (_psInscatterS->Init(psDescLayers))
            return true;
    }
    {
        psDescLayers.PS = shader->GetPS("PS_InscatterN");
        if (_psInscatterN->Init(psDescLayers))
            return true;
    }
    psDescLayers.BlendMode = BlendingMode::Add;
    psDesc.BlendMode = BlendingMode::Add;
    {
        psDescLayers.PS = shader->GetPS("PS_CopyInscatterN");
        if (_psCopyInscatterNAdd->Init(psDescLayers))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_CopyIrradiance1");
        if (_psCopyIrradianceAdd->Init(psDesc))
            return true;
    }

    // Init rendering pipeline
    _task = New<SceneRenderTask>();
    _task->Enabled = false;
    _task->IsCustomRendering = true;
    _task->Render.Bind(Render);

    // Init render targets
    if (!AtmosphereTransmittance)
    {
        AtmosphereTransmittance = device->CreateTexture(TEXT("AtmospherePreCompute.Transmittance"));
        auto desc = GPUTextureDescription::New2D(TransmittanceTexWidth, TransmittanceTexHeight, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget);
        if (AtmosphereTransmittance->Init(desc))
            return true;
        AtmosphereIrradiance = device->CreateTexture(TEXT("AtmospherePreCompute.Irradiance"));
        desc = GPUTextureDescription::New2D(IrradianceTexWidth, IrradianceTexHeight, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget);
        if (AtmosphereIrradiance->Init(desc))
            return true;
        AtmosphereInscatter = device->CreateTexture(TEXT("AtmospherePreCompute.Inscatter"));
        desc = GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews);
        if (AtmosphereInscatter->Init(desc))
            return true;
    }

    // Mark as ready
    _isReadyForCompute = true;
    _wasCancelled = false;
    return false;
}

void ReleaseAtmospherePreCompute()
{
    if (!_isReadyForCompute)
        return;
    if (_updateFrameNumber != 0)
    {
        _wasCancelled = true;
    }
    _updateFrameNumber = 0;

    // Release data
    SAFE_DELETE_GPU_RESOURCE(_psTransmittance);
    SAFE_DELETE_GPU_RESOURCE(_psIrradiance1);
    SAFE_DELETE_GPU_RESOURCE(_psIrradianceN);
    SAFE_DELETE_GPU_RESOURCE(_psCopyIrradianceAdd);
    SAFE_DELETE_GPU_RESOURCE(_psInscatter1_A);
    SAFE_DELETE_GPU_RESOURCE(_psInscatter1_B);
    SAFE_DELETE_GPU_RESOURCE(_psCopyInscatter1);
    SAFE_DELETE_GPU_RESOURCE(_psInscatterS);
    SAFE_DELETE_GPU_RESOURCE(_psInscatterN);
    SAFE_DELETE_GPU_RESOURCE(_psCopyInscatterNAdd);
    _shader = nullptr;
    SAFE_DELETE(_task);

    _isReadyForCompute = false;
}

void AtmospherePreComputeService::Update()
{
    // Check if render job is done
    if (isUpdateSynced())
    {
        // Clear flags
        _updateFrameNumber = 0;
        _isUpdatePending = false;
    }
    else if (_isUpdatePending && (_task == nullptr || !_task->Enabled))
    {
        PROFILE_CPU();
        PROFILE_MEM(Graphics);

        // Wait for the shader
        if (!_shader)
            _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/AtmospherePreCompute"));
        if (_shader && !_shader->IsLoaded())
            return;
        if (InitAtmospherePreCompute())
        {
            LOG(Fatal, "Cannot setup Atmosphere Pre Compute!");
        }

        // Mark task to update
        _task->Enabled = true;
        _updateFrameNumber = 0;
    }
    else if (_hasDataCached && _isReadyForCompute && !_isUpdatePending && Engine::FrameCount - _dataCachedFrameNumber > ATMOSPHERE_PRECOMPUTE_AUTO_RELEASE_FRAMES)
    {
        // Automatically free resources after last update (keep Transmittance/Irradiance/Inscatter textures valid for use)
        ReleaseAtmospherePreCompute();
    }
}

void AtmospherePreComputeService::Dispose()
{
    ReleaseAtmospherePreCompute();
    _hasDataCached = false;
    SAFE_DELETE_GPU_RESOURCE(AtmosphereTransmittance);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereIrradiance);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereInscatter);
}

void GetLayerValue(int32 layer, float& atmosphereR, Float4& dhdh)
{
    float r = layer / Math::Max<float>(InscatterAltitudeSampleNum - 1.0f, 1.0f);
    r = r * r;
    r = Math::Sqrt(RadiusGround * RadiusGround + r * (RadiusAtmosphere * RadiusAtmosphere - RadiusGround * RadiusGround)) + (layer == 0 ? 0.01f : (layer == InscatterAltitudeSampleNum - 1 ? -0.001f : 0.0f));
    const float dMin = RadiusAtmosphere - r;
    const float dMax = Math::Sqrt(r * r - RadiusGround * RadiusGround) + Math::Sqrt(RadiusAtmosphere * RadiusAtmosphere - RadiusGround * RadiusGround);
    const float dMinP = r - RadiusGround;
    const float dMaxP = Math::Sqrt(r * r - RadiusGround * RadiusGround);
    atmosphereR = r;
    dhdh = Float4(dMin, dMax, dMinP, dMaxP);
}

void AtmospherePreComputeImpl::Render(RenderTask* task, GPUContext* context)
{
    // If job has been cancelled (eg. on window close)
    if (_wasCancelled)
    {
        LOG(Warning, "AtmospherePreCompute job cancelled");
        return;
    }
    ASSERT(_isUpdatePending && _updateFrameNumber == 0);
    PROFILE_GPU_CPU("AtmospherePreCompute");

    // Allocate temporary render targets
    auto desc = GPUTextureDescription::New2D(IrradianceTexWidth, IrradianceTexHeight, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget);
    auto AtmosphereDeltaE = RenderTargetPool::Get(desc);
    desc = GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews);
    auto AtmosphereDeltaSR = RenderTargetPool::Get(desc);
    desc = GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews);
    auto AtmosphereDeltaSM = RenderTargetPool::Get(desc);
    desc = GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews);
    auto AtmosphereDeltaJ = RenderTargetPool::Get(desc);
    if (!AtmosphereDeltaE || !AtmosphereDeltaSR || !AtmosphereDeltaSM || !AtmosphereDeltaJ)
        return;
    RENDER_TARGET_POOL_SET_NAME(AtmosphereDeltaE, "AtmospherePreCompute.DeltaE");
    RENDER_TARGET_POOL_SET_NAME(AtmosphereDeltaSR, "AtmospherePreCompute.DeltaSR");
    RENDER_TARGET_POOL_SET_NAME(AtmosphereDeltaSM, "AtmospherePreCompute.DeltaSM");
    RENDER_TARGET_POOL_SET_NAME(AtmosphereDeltaJ, "AtmospherePreCompute.DeltaJ");

    const auto shader = _shader->GetShader();
    const auto cb = shader->GetCB(0);
    Data data;

    // Compute transmittance texture T (line 1 in algorithm 4.1)
    context->SetRenderTarget(*AtmosphereTransmittance);
    context->SetViewportAndScissors((float)TransmittanceTexWidth, (float)TransmittanceTexHeight);
    context->SetState(_psTransmittance);
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Compute irradiance texture deltaE (line 2 in algorithm 4.1)
    context->SetRenderTarget(*AtmosphereDeltaE);
    context->SetViewportAndScissors((float)IrradianceTexWidth, (float)IrradianceTexHeight);
    context->BindSR(0, AtmosphereTransmittance);
    context->SetState(_psIrradiance1);
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Compute single scattering texture deltaS (line 3 in algorithm 4.1)
    // Rayleigh and Mie separated in deltaSR + deltaSM
    context->SetViewportAndScissors((float)InscatterWidth, (float)InscatterHeight);
    context->SetState(_psInscatter1_A);
    for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
    {
        GetLayerValue(layer, data.AtmosphereR, data.dhdh);
        data.AtmosphereLayer = layer;
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);

        context->SetRenderTarget(AtmosphereDeltaSR->View(layer));
        context->DrawFullscreenTriangle();
    }
    context->SetState(_psInscatter1_B);
    for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
    {
        GetLayerValue(layer, data.AtmosphereR, data.dhdh);
        data.AtmosphereLayer = layer;
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);

        context->SetRenderTarget(AtmosphereDeltaSM->View(layer));
        context->DrawFullscreenTriangle();
    }
    context->ResetRenderTarget();

    context->Clear(*AtmosphereIrradiance, Color::Transparent);

    // Copy deltaS into inscatter texture S (line 5 in algorithm 4.1)
    context->SetViewportAndScissors((float)InscatterWidth, (float)InscatterHeight);
    context->SetState(_psCopyInscatter1);
    context->BindSR(4, AtmosphereDeltaSR->ViewVolume());
    context->BindSR(5, AtmosphereDeltaSM->ViewVolume());
    for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
    {
        GetLayerValue(layer, data.AtmosphereR, data.dhdh);
        data.AtmosphereLayer = layer;
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);

        context->SetRenderTarget(AtmosphereInscatter->View(layer));
        context->DrawFullscreenTriangle();
    }
    context->ResetRenderTarget();

    // Loop for each scattering order (line 6 in algorithm 4.1)
    for (int32 order = 2; order <= MaxScatteringOrder; order++)
    {
        // Compute deltaJ (line 7 in algorithm 4.1)
        context->UnBindSR(6);
        context->SetViewportAndScissors((float)InscatterWidth, (float)InscatterHeight);
        context->SetState(_psInscatterS);
        data.First = order == 2 ? 1.0f : 0.0f;
        context->BindSR(0, AtmosphereTransmittance);
        context->BindSR(3, AtmosphereDeltaE);
        context->BindSR(4, AtmosphereDeltaSR->ViewVolume());
        context->BindSR(5, AtmosphereDeltaSM->ViewVolume());
        for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
        {
            GetLayerValue(layer, data.AtmosphereR, data.dhdh);
            data.AtmosphereLayer = layer;
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);

            context->SetRenderTarget(AtmosphereDeltaJ->View(layer));
            context->DrawFullscreenTriangle();
        }

        // Compute deltaE (line 8 in algorithm 4.1)
        context->UnBindSR(3);
        context->SetRenderTarget(AtmosphereDeltaE->View());
        context->SetViewportAndScissors((float)IrradianceTexWidth, (float)IrradianceTexHeight);
        context->BindSR(0, AtmosphereTransmittance);
        context->BindSR(4, AtmosphereDeltaSR->ViewVolume());
        context->BindSR(5, AtmosphereDeltaSM->ViewVolume());
        context->SetState(_psIrradianceN);
        context->DrawFullscreenTriangle();
        context->ResetRenderTarget();

        // Compute deltaS (line 9 in algorithm 4.1)
        context->UnBindSR(4);
        context->SetViewportAndScissors((float)InscatterWidth, (float)InscatterHeight);
        context->SetState(_psInscatterN);
        context->BindSR(0, AtmosphereTransmittance);
        context->BindSR(6, AtmosphereDeltaJ->ViewVolume());
        for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
        {
            GetLayerValue(layer, data.AtmosphereR, data.dhdh);
            data.AtmosphereLayer = layer;
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);

            context->SetRenderTarget(AtmosphereDeltaSR->View(layer));
            context->DrawFullscreenTriangle();
        }

        // Add deltaE into irradiance texture E (line 10 in algorithm 4.1)
        context->SetRenderTarget(*AtmosphereIrradiance);
        context->SetViewportAndScissors((float)IrradianceTexWidth, (float)IrradianceTexHeight);
        context->BindSR(3, AtmosphereDeltaE);
        context->SetState(_psCopyIrradianceAdd);
        context->DrawFullscreenTriangle();
        context->ResetRenderTarget();

        // Add deltaS into inscatter texture S (line 11 in algorithm 4.1)
        context->SetViewportAndScissors((float)InscatterWidth, (float)InscatterHeight);
        context->SetState(_psCopyInscatterNAdd);
        context->BindSR(4, AtmosphereDeltaSR->ViewVolume());
        for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
        {
            GetLayerValue(layer, data.AtmosphereR, data.dhdh);
            data.AtmosphereLayer = layer;
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);

            context->SetRenderTarget(AtmosphereInscatter->View(layer));
            context->DrawFullscreenTriangle();
        }
    }

    // Cleanup
    context->ResetRenderTarget();
    context->ResetSR();
    RenderTargetPool::Release(AtmosphereDeltaE);
    RenderTargetPool::Release(AtmosphereDeltaSR);
    RenderTargetPool::Release(AtmosphereDeltaSM);
    RenderTargetPool::Release(AtmosphereDeltaJ);

    // Mark as rendered
    _hasDataCached = true;
    _isUpdatePending = false;
    _dataCachedFrameNumber = _updateFrameNumber = Engine::FrameCount;
    _task->Enabled = false;
}
