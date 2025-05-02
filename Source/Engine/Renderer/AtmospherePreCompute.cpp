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
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/AssetReference.h"

// Amount of frames to wait for data from atmosphere precompute job
#define ATMOSPHERE_PRECOMPUTE_LATENCY_FRAMES 1

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

class DownloadJob : public ThreadPoolTask
{
private:

    GPUTexture* _transmittance;
    GPUTexture* _irradiance;
    GPUTexture* _inscatter;

public:

    DownloadJob(GPUTexture* transmittance, GPUTexture* irradiance, GPUTexture* inscatter);

protected:

    // [ThreadPoolTask]
    bool Run() override;
};

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

    AssetReference<Shader> _shader;
    GPUPipelineState* _psTransmittance = nullptr;
    GPUPipelineState* _psIrradiance1 = nullptr;
    GPUPipelineState* _psIrradianceN = nullptr;
    GPUPipelineState* _psCopyIrradiance = nullptr;
    GPUPipelineState* _psCopyIrradianceAdd = nullptr;
    GPUPipelineState* _psInscatter1_A = nullptr;
    GPUPipelineState* _psInscatter1_B = nullptr;
    GPUPipelineState* _psCopyInscatter1 = nullptr;
    GPUPipelineState* _psCopyInscatterNAdd = nullptr;
    GPUPipelineState* _psInscatterS = nullptr;
    GPUPipelineState* _psInscatterN = nullptr;
    SceneRenderTask* _task = nullptr;

    //
    GPUTexture* AtmosphereTransmittance = nullptr;
    GPUTexture* AtmosphereIrradiance = nullptr;
    GPUTexture* AtmosphereInscatter = nullptr;
    //
    GPUTexture* AtmosphereDeltaE = nullptr;
    GPUTexture* AtmosphereDeltaSR = nullptr;
    GPUTexture* AtmosphereDeltaSM = nullptr;
    GPUTexture* AtmosphereDeltaJ = nullptr;

    uint64 _updateFrameNumber;
    bool _wasCancelled;

    FORCE_INLINE bool isUpdateSynced()
    {
        return _updateFrameNumber > 0 && _updateFrameNumber + ATMOSPHERE_PRECOMPUTE_LATENCY_FRAMES <= Engine::FrameCount;
    }

    void onRender(RenderTask* task, GPUContext* context);
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

bool init()
{
    if (_isReadyForCompute)
        return false;

    LOG(Info, "Starting Atmosphere Pre Compute service");

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
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    _psTransmittance = GPUDevice::Instance->CreatePipelineState();
    _psIrradiance1 = GPUDevice::Instance->CreatePipelineState();
    _psIrradianceN = GPUDevice::Instance->CreatePipelineState();
    _psCopyIrradiance = GPUDevice::Instance->CreatePipelineState();
    _psCopyIrradianceAdd = GPUDevice::Instance->CreatePipelineState();
    _psInscatter1_A = GPUDevice::Instance->CreatePipelineState();
    _psInscatter1_B = GPUDevice::Instance->CreatePipelineState();
    _psCopyInscatter1 = GPUDevice::Instance->CreatePipelineState();
    _psInscatterS = GPUDevice::Instance->CreatePipelineState();
    _psInscatterN = GPUDevice::Instance->CreatePipelineState();
    _psCopyInscatterNAdd = GPUDevice::Instance->CreatePipelineState();
    GPUPipelineState::Description psDesc, psDescLayers;
    psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    psDescLayers = psDesc;
    //psDescLayers.GS = shader->GetGS(0);
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
        psDesc.PS = shader->GetPS("PS_CopyIrradiance1");
        if (_psCopyIrradiance->Init(psDesc))
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
    _task->Render.Bind(onRender);

    // Init render targets
    AtmosphereTransmittance = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.Transmittance"));
    if (AtmosphereTransmittance->Init(GPUTextureDescription::New2D(TransmittanceTexWidth, TransmittanceTexHeight, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)))
        return true;
    AtmosphereIrradiance = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.Irradiance"));
    if (AtmosphereIrradiance->Init(GPUTextureDescription::New2D(IrradianceTexWidth, IrradianceTexHeight, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)))
        return true;
    AtmosphereDeltaE = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.DeltaE"));
    if (AtmosphereDeltaE->Init(GPUTextureDescription::New2D(IrradianceTexWidth, IrradianceTexHeight, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)))
        return true;
    AtmosphereInscatter = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.Inscatter"));
    if (AtmosphereInscatter->Init(GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews)))
        return true;
    AtmosphereDeltaSR = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.DeltaSR"));
    if (AtmosphereDeltaSR->Init(GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews)))
        return true;
    AtmosphereDeltaSM = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.DeltaSM"));
    if (AtmosphereDeltaSM->Init(GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews)))
        return true;
    AtmosphereDeltaJ = GPUDevice::Instance->CreateTexture(TEXT("AtmospherePreCompute.DeltaJ"));
    if (AtmosphereDeltaJ->Init(GPUTextureDescription::New3D(InscatterWidth, InscatterHeight, InscatterDepth, PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerSliceViews)))
        return true;

    // Mark as ready
    _isReadyForCompute = true;
    _wasCancelled = false;
    return false;
}

void release()
{
    if (!_isReadyForCompute)
        return;
    if (_updateFrameNumber != 0)
    {
        _wasCancelled = true;
    }
    _updateFrameNumber = 0;

    LOG(Info, "Disposing Atmosphere Pre Compute service");

    // Release data
    SAFE_DELETE_GPU_RESOURCE(_psTransmittance);
    SAFE_DELETE_GPU_RESOURCE(_psIrradiance1);
    SAFE_DELETE_GPU_RESOURCE(_psIrradianceN);
    SAFE_DELETE_GPU_RESOURCE(_psCopyIrradiance);
    SAFE_DELETE_GPU_RESOURCE(_psCopyIrradianceAdd);
    SAFE_DELETE_GPU_RESOURCE(_psInscatter1_A);
    SAFE_DELETE_GPU_RESOURCE(_psInscatter1_B);
    SAFE_DELETE_GPU_RESOURCE(_psCopyInscatter1);
    SAFE_DELETE_GPU_RESOURCE(_psInscatterS);
    SAFE_DELETE_GPU_RESOURCE(_psInscatterN);
    SAFE_DELETE_GPU_RESOURCE(_psCopyInscatterNAdd);
    _shader = nullptr;
    SAFE_DELETE(_task);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereTransmittance);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereIrradiance);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereInscatter);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereDeltaE);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereDeltaSR);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereDeltaSM);
    SAFE_DELETE_GPU_RESOURCE(AtmosphereDeltaJ);

    _isReadyForCompute = false;
}

void AtmospherePreComputeService::Update()
{
    // Calculate time delta since last update
    auto currentTime = Time::Update.UnscaledTime;

    // Check if render job is done
    if (isUpdateSynced())
    {
        // Create async job to gather data from the GPU
        //ThreadPool::AddJob(New<DownloadJob>(AtmosphereTransmittance, AtmosphereIrradiance, AtmosphereInscatter));

        // TODO: download data from generated cache textures
        // TODO: serialize cached textures

        // Clear flags
        _updateFrameNumber = 0;
        _isUpdatePending = false;

        // Release service
        //release();
        // TODO: release service data
    }
    else if (_isUpdatePending && (_task == nullptr || !_task->Enabled))
    {
        // TODO: init but without a stalls, just wait for resources loaded and then start rendering

        // Init service
        if (!_shader)
            _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/AtmospherePreCompute"));
        if (_shader && !_shader->IsLoaded())
            return;
        if (init())
        {
            LOG(Fatal, "Cannot setup Atmosphere Pre Compute!");
        }

        // Mark task to update
        _task->Enabled = true;
        _updateFrameNumber = 0;
    }
}

void AtmospherePreComputeService::Dispose()
{
    release();
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

void AtmospherePreComputeImpl::onRender(RenderTask* task, GPUContext* context)
{
    // If job has been cancelled (eg. on window close)
    if (_wasCancelled)
    {
        LOG(Warning, "AtmospherePreCompute job cancelled");
        return;
    }
    ASSERT(_isUpdatePending && _updateFrameNumber == 0);

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

    //// old way to render Inscatter1 to DeltaSR and DeltaSM at once but didn't work well :/ (no time to find out why)
    //for (int32 layer = 0; layer < InscatterAltitudeSampleNum; layer++)
    //{
    //	GetLayerValue(layer, data.AtmosphereR, data.dhdh);
    //	data.AtmosphereLayer = layer;
    //	cb->SetData(&data);
    //	context->Bind(0, cb);

    //	GPUTextureView* deltaRTs[2] = { AtmosphereDeltaSR->HandleSlice(layer), AtmosphereDeltaSM->HandleSlice(layer) };
    //	//GPUTextureView* deltaRTs[2] = { AtmosphereIrradiance->Handle(), AtmosphereDeltaE->Handle() };
    //	context->SetRenderTarget(nullptr, 2, deltaRTs);
    //	//context->SetRenderTarget(AtmosphereIrradiance->View());
    //	//context->SetRenderTarget(AtmosphereDeltaSR->HandleSlice(layer));
    //	//context->SetRenderTarget(AtmosphereDeltaSR->View());
    //	context->DrawFullscreenTriangle();
    //}
    //context->SetRenderTarget();

    // Copy deltaE into irradiance texture E (line 4 in algorithm 4.1)
    /*context->SetRenderTarget(*AtmosphereIrradiance);
    context->SetViewport(IrradianceTexWidth, IrradianceTexHeight);
    context->Bind(3, AtmosphereDeltaE);
    context->SetState(_psCopyIrradiance);
    context->DrawFullscreenTriangle();*/
    // TODO: remove useless CopyIrradiance shader program?

    context->SetViewportAndScissors((float)IrradianceTexWidth, (float)IrradianceTexHeight);
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

    // Mark as rendered
    _hasDataCached = true;
    _isUpdatePending = false;
    _updateFrameNumber = Engine::FrameCount;
    _task->Enabled = false;
}

DownloadJob::DownloadJob(GPUTexture* transmittance, GPUTexture* irradiance, GPUTexture* inscatter)
    : _transmittance(transmittance)
    , _irradiance(irradiance)
    , _inscatter(inscatter)
{
}

bool DownloadJob::Run()
{
    TextureData textureData;
    Array<byte> data;

#if BUILD_DEBUG

    // Transmittance
    /*{
        if (_transmittance->DownloadData(textureData))
        {
            LOG(Fatal, 193, "AtmospherePreCompute::DownloadJob::Transmittance");
            return;
        }

        auto in = textureData.Get(0, 0);
        int rowPich = 3 * TransmittanceTexWidth * sizeof(BYTE);
        int size = rowPich * TransmittanceTexHeight;
        data.EnsureCapacity(size, false);
        byte* p = data.Get();
        for (int y = 0; y < TransmittanceTexHeight; y++)
        {
            for (int x = 0; x < TransmittanceTexWidth; x++)
            {
                Float4 t = ((Half4*)&in->Data[y * in->RowPitch + x * sizeof(Half4)])->ToFloat4();
                *p++ = Math::Saturate(t.Z) * 255;
                *p++ = Math::Saturate(t.Y) * 255;
                *p++ = Math::Saturate(t.X) * 255;
            }
        }

        FileSystem::SaveBitmapToFile(data.Get(),
                                     TransmittanceTexWidth,
                                     TransmittanceTexHeight,
                                     24,
                                     0,
                                     L"C:\\Users\\Wojtek\\Downloads\\transmittance_Flax.bmp");
    }*/

    // Irradiance
    /*{
        textureData.Clear();
        if (_irradiance->DownloadData(textureData))
        {
            LOG(Fatal, 193, "AtmospherePreCompute::DownloadJob::Irradiance");
            return;
        }

        auto in = textureData.Get(0, 0);
        int rowPich = 3 * IrradianceTexWidth * sizeof(BYTE);
        int size = rowPich * IrradianceTexHeight;
        data.EnsureCapacity(size, false);
        byte* p = data.Get();
        for (int y = 0; y < IrradianceTexHeight; y++)
        {
            for (int x = 0; x < IrradianceTexWidth; x++)
            {
                Float4 t = ((Half4*)&in->Data[y * in->RowPitch + x * sizeof(Half4)])->ToFloat4();
                *p++ = Math::Saturate(t.Z) * 255;
                *p++ = Math::Saturate(t.Y) * 255;
                *p++ = Math::Saturate(t.X) * 255;
            }
        }

        FileSystem::SaveBitmapToFile(data.Get(),
                                     IrradianceTexWidth,
                                     IrradianceTexHeight,
                                     24,
                                     0,
                                     L"C:\\Users\\Wojtek\\Downloads\\irradiance_Flax.bmp");
    }*/

    /*// Inscatter
    {
        textureData.Clear();
        if (_inscatter->DownloadData(textureData))
        {
            LOG(Fatal, 193, "AtmospherePreCompute::DownloadJob::Inscatter");
            return;
        }
        auto in = textureData.Get(0, 0);
        int rowPich = 3 * InscatterWidth * sizeof(BYTE);
        int size = rowPich * InscatterHeight;
        data.EnsureCapacity(size, false);

        for (int32 depthSlice = 0; depthSlice < InscatterDepth; depthSlice++)
        {
            byte* p = data.Get();
            byte* s = in->Data.Get() + depthSlice * in->DepthPitch;

            for (int y = 0; y < InscatterHeight; y++)
            {
                for (int x = 0; x < InscatterWidth; x++)
                {
                    Float4 t = ((Half4*)&s[y * in->RowPitch + x * sizeof(Half4)])->ToFloat4();
                    *p++ = Math::Saturate(t.Z) * 255;
                    *p++ = Math::Saturate(t.Y) * 255;
                    *p++ = Math::Saturate(t.X) * 255;
                }
            }

            FileSystem::SaveBitmapToFile(data.Get(),
                                         InscatterWidth,
                                         InscatterHeight,
                                         24,
                                         0,
                                         *String::Format(TEXT("C:\\Users\\Wojtek\\Downloads\\inscatter_{0}_Flax.bmp"), depthSlice));

            break;
        }
    }*/

#else

    MISSING_CODE("download precomputed atmosphere data from the GPU!");

#endif

    return false;
}
