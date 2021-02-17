// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROBES_BAKING

#include "ProbesRenderer.h"
#include "Renderer.h"
#include "ReflectionsPass.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Level/Actors/PointLight.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"
#include "Engine/Level/Actors/SkyLight.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/ContentExporters/AssetExporters.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Engine/Time.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Engine/Engine.h"

/// <summary>
/// Custom task called after downloading probe texture data to save it.
/// </summary>
/// <seealso cref="ThreadPoolTask" />
class DownloadProbeTask : public ThreadPoolTask
{
private:

    GPUTexture* _texture;
    TextureData _data;
    ProbesRenderer::Entry _entry;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="DownloadProbeTask"/> class.
    /// </summary>
    /// <param name="target">The target.</param>
    /// <param name="entry">The entry.</param>
    DownloadProbeTask(GPUTexture* target, const ProbesRenderer::Entry& entry)
        : _texture(target)
        , _entry(entry)
    {
    }

public:

    /// <summary>
    /// Gets the texture data container.
    /// </summary>
    FORCE_INLINE TextureData& GetData()
    {
        return _data;
    }

protected:

    // [ThreadPoolTask]
    bool Run() override
    {
        // Switch type
        if (_entry.Type == ProbesRenderer::EntryType::EnvProbe)
        {
            if (_entry.Actor)
                ((EnvironmentProbe*)_entry.Actor.Get())->SetProbeData(_data);
        }
        else if (_entry.Type == ProbesRenderer::EntryType::SkyLight)
        {
            if (_entry.Actor)
                ((SkyLight*)_entry.Actor.Get())->SetProbeData(_data);
        }
        else
        {
            return true;
        }

        // Fire event
        ProbesRenderer::OnFinishBake(_entry);

        return false;
    }
};

PACK_STRUCT(struct Data
    {
    Vector2 Dummy0;
    int32 CubeFace;
    int32 SourceMipIndex;
    Vector4 Sample01;
    Vector4 Sample23;
    Vector4 CoefficientMask0;
    Vector4 CoefficientMask1;
    Vector3 Dummy1;
    float CoefficientMask2;
    });

namespace ProbesRendererImpl
{
    TimeSpan _lastProbeUpdate(0);
    Array<ProbesRenderer::Entry> _probesToBake;

    ProbesRenderer::Entry _current;

    bool _isReady = false;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psFilterFace = nullptr;
    GPUPipelineState* _psCopyFace = nullptr;
    GPUPipelineState* _psCalcDiffuseIrradiance = nullptr;
    GPUPipelineState* _psAccDiffuseIrradiance = nullptr;
    GPUPipelineState* _psAccumulateCubeFaces = nullptr;
    GPUPipelineState* _psCopyFrameLHB = nullptr;
    SceneRenderTask* _task = nullptr;
    GPUTexture* _output = nullptr;
    GPUTexture* _probe = nullptr;
    GPUTexture* _tmpFace = nullptr;
    GPUTexture* _skySHIrradianceMap = nullptr;
    uint64 _updateFrameNumber = 0;

    FORCE_INLINE bool isUpdateSynced()
    {
        return _updateFrameNumber > 0 && _updateFrameNumber + PROBES_RENDERER_LATENCY_FRAMES <= Engine::FrameCount;
    }
}

using namespace ProbesRendererImpl;

class ProbesRendererService : public EngineService
{
public:

    ProbesRendererService()
        : EngineService(TEXT("Probes Renderer"), 70)
    {
    }

    void Update() override;
    void Dispose() override;
};

ProbesRendererService ProbesRendererServiceInstance;

TimeSpan ProbesRenderer::ProbesUpdatedBreak(0, 0, 0, 0, 500);
TimeSpan ProbesRenderer::ProbesReleaseDataTime(0, 0, 0, 60);
Delegate<const ProbesRenderer::Entry&> ProbesRenderer::OnRegisterBake;
Delegate<const ProbesRenderer::Entry&> ProbesRenderer::OnFinishBake;

void ProbesRenderer::Bake(EnvironmentProbe* probe, float timeout)
{
    ASSERT(probe && dynamic_cast<EnvironmentProbe*>(probe));

    // Check if already registered for bake
    for (int32 i = 0; i < _probesToBake.Count(); i++)
    {
        auto& p = _probesToBake[i];
        if (p.Type == EntryType::EnvProbe && p.Actor == probe)
        {
            p.Timeout = timeout;
            return;
        }
    }

    // Register probe
    Entry e;
    e.Type = EntryType::EnvProbe;
    e.Actor = probe;
    e.Timeout = timeout;
    _probesToBake.Add(e);

    // Fire event
    OnRegisterBake(e);
}

void ProbesRenderer::Bake(SkyLight* probe, float timeout)
{
    ASSERT(probe && dynamic_cast<SkyLight*>(probe));

    // Check if already registered for bake
    for (int32 i = 0; i < _probesToBake.Count(); i++)
    {
        auto& p = _probesToBake[i];
        if (p.Type == EntryType::SkyLight && p.Actor == probe)
        {
            p.Timeout = timeout;
            return;
        }
    }

    // Register probe
    Entry e;
    e.Type = EntryType::SkyLight;
    e.Actor = probe;
    e.Timeout = timeout;
    _probesToBake.Add(e);

    // Fire event
    OnRegisterBake(e);
}

int32 ProbesRenderer::GetBakeQueueSize()
{
    return _probesToBake.Count();
}

bool ProbesRenderer::HasReadyResources()
{
    return _isReady && _shader->IsLoaded();
}

bool ProbesRenderer::Init()
{
    if (_isReady)
        return false;

    // Load shader
    if (_shader == nullptr)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/ProbesFilter"));
        if (_shader == nullptr)
            return true;
    }
    if (!_shader->IsLoaded())
        return false;
    const auto shader = _shader->GetShader();

    LOG(Info, "Starting Probes Renderer service");

    // Validate shader constant buffers sizes
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        LOG(Fatal, "Shader {0} has incorrect constant buffer {1} size: {2} bytes. Expected: {3} bytes", _shader.ToString(), 0, shader->GetCB(0)->GetSize(), sizeof(Data));
        return true;
    }

    // Create pipeline stages
    _psFilterFace = GPUDevice::Instance->CreatePipelineState();
    _psCopyFace = GPUDevice::Instance->CreatePipelineState();
    _psCalcDiffuseIrradiance = GPUDevice::Instance->CreatePipelineState();
    _psAccDiffuseIrradiance = GPUDevice::Instance->CreatePipelineState();
    _psAccumulateCubeFaces = GPUDevice::Instance->CreatePipelineState();
    _psCopyFrameLHB = GPUDevice::Instance->CreatePipelineState();
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    {
        psDesc.PS = shader->GetPS("PS_FilterFace");
        if (_psFilterFace->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_CopyFace");
        if (_psCopyFace->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_CalcDiffuseIrradiance");
        if (_psCalcDiffuseIrradiance->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_AccDiffuseIrradiance");
        if (_psAccDiffuseIrradiance->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_AccumulateCubeFaces");
        if (_psAccumulateCubeFaces->Init(psDesc))
            return true;
    }
    {
        psDesc.PS = shader->GetPS("PS_CopyFrameLHB");
        if (_psCopyFrameLHB->Init(psDesc))
            return true;
    }

    // Init rendering pipeline
    _output = GPUTexture::New();
    if (_output->Init(GPUTextureDescription::New2D(ENV_PROBES_RESOLUTION, ENV_PROBES_RESOLUTION, ENV_PROBES_FORMAT)))
        return true;
    _task = New<SceneRenderTask>();
    auto task = _task;
    task->Enabled = false;
    task->Output = _output;
    auto& view = task->View;
    view.Flags =
            ViewFlags::AO |
            ViewFlags::GI |
            ViewFlags::DirectionalLights |
            ViewFlags::PointLights |
            ViewFlags::SpotLights |
            ViewFlags::SkyLights |
            ViewFlags::Decals |
            ViewFlags::Shadows |
            ViewFlags::Fog;
    view.Mode = ViewMode::NoPostFx;
    view.IsOfflinePass = true;
    view.StaticFlagsMask = StaticFlags::ReflectionProbe;
    view.MaxShadowsQuality = Quality::Low;
    task->IsCameraCut = true;
    task->Resize(ENV_PROBES_RESOLUTION, ENV_PROBES_RESOLUTION);
    task->Render.Bind(onRender);

    // Init render targets
    _probe = GPUDevice::Instance->CreateTexture(TEXT("ProbesUpdate.Probe"));
    if (_probe->Init(GPUTextureDescription::NewCube(ENV_PROBES_RESOLUTION, ENV_PROBES_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews, 0)))
        return true;
    _tmpFace = GPUDevice::Instance->CreateTexture(TEXT("ProbesUpdate.TmpFae"));
    if (_tmpFace->Init(GPUTextureDescription::New2D(ENV_PROBES_RESOLUTION, ENV_PROBES_RESOLUTION, 0, ENV_PROBES_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews)))
        return true;

    // Mark as ready
    _isReady = true;
    return false;
}

void ProbesRenderer::Release()
{
    if (!_isReady)
        return;
    ASSERT(_updateFrameNumber == 0);

    LOG(Info, "Disposing Probes Renderer service");

    // Release GPU data
    if (_output)
        _output->ReleaseGPU();

    // Release data
    SAFE_DELETE_GPU_RESOURCE(_psFilterFace);
    SAFE_DELETE_GPU_RESOURCE(_psCopyFace);
    SAFE_DELETE_GPU_RESOURCE(_psCalcDiffuseIrradiance);
    SAFE_DELETE_GPU_RESOURCE(_psAccDiffuseIrradiance);
    SAFE_DELETE_GPU_RESOURCE(_psAccumulateCubeFaces);
    SAFE_DELETE_GPU_RESOURCE(_psCopyFrameLHB);
    _shader = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_output);
    SAFE_DELETE(_task);
    SAFE_DELETE_GPU_RESOURCE(_probe);
    SAFE_DELETE_GPU_RESOURCE(_tmpFace);
    SAFE_DELETE_GPU_RESOURCE(_skySHIrradianceMap);

    _isReady = false;
}

void ProbesRendererService::Update()
{
    // Calculate time delta since last update
    auto timeNow = Time::Update.UnscaledTime;
    auto timeSinceUpdate = timeNow - _lastProbeUpdate;

    // Check if render job is done
    if (isUpdateSynced())
    {
        // Create async job to gather probe data from the GPU
        GPUTexture* texture = nullptr;
        switch (_current.Type)
        {
        case ProbesRenderer::EntryType::SkyLight:
        case ProbesRenderer::EntryType::EnvProbe:
            texture = _probe;
            break;
        }
        ASSERT(texture);
        auto taskB = New<DownloadProbeTask>(texture, _current);
        auto taskA = texture->DownloadDataAsync(taskB->GetData());
        if (taskA == nullptr)
        {
            LOG(Fatal, "Failed to create async tsk to download env probe texture data fro mthe GPU.");
        }
        taskA->ContinueWith(taskB);
        taskA->Start();

        // Clear flag
        _updateFrameNumber = 0;
        _current.Type = ProbesRenderer::EntryType::Invalid;
    }
    else if (_current.Type == ProbesRenderer::EntryType::Invalid)
    {
        int32 firstValidEntryIndex = -1;
        auto dt = (float)Time::Update.UnscaledDeltaTime.GetTotalSeconds();
        for (int32 i = 0; i < _probesToBake.Count(); i++)
        {
            _probesToBake[i].Timeout -= dt;
            if (_probesToBake[i].Timeout <= 0)
            {
                firstValidEntryIndex = i;
                break;
            }
        }

        // Check if need to update probe
        if (firstValidEntryIndex >= 0 && timeSinceUpdate > ProbesRenderer::ProbesUpdatedBreak)
        {
            // Init service
            if (ProbesRenderer::Init())
            {
                LOG(Fatal, "Cannot setup Probes Renderer!");
            }
            if (ProbesRenderer::HasReadyResources() == false)
                return;

            // Mark probe to update
            _current = _probesToBake[firstValidEntryIndex];
            _probesToBake.RemoveAtKeepOrder(firstValidEntryIndex);
            _task->Enabled = true;
            _updateFrameNumber = 0;

            // Store time of the last probe update
            _lastProbeUpdate = timeNow;
        }
            // Check if need to release data
        else if (_isReady && timeSinceUpdate > ProbesRenderer::ProbesReleaseDataTime)
        {
            // Release service
            ProbesRenderer::Release();
        }
    }
}

void ProbesRendererService::Dispose()
{
    ProbesRenderer::Release();
}

bool fixFarPlaneTreeExecute(Actor* actor, const Vector3& position, float& farPlane)
{
    if (auto* pointLight = dynamic_cast<PointLight*>(actor))
    {
        const float dst = Vector3::Distance(pointLight->GetPosition(), position) + pointLight->GetScaledRadius();
        if (dst > farPlane)
        {
            farPlane = dst;
        }
    }

    return true;
}

void ProbesRenderer::onRender(RenderTask* task, GPUContext* context)
{
    ASSERT(_current.Type != EntryType::Invalid && _updateFrameNumber == 0);
    switch (_current.Type)
    {
    case EntryType::EnvProbe:
    case EntryType::SkyLight:
    {
        if (_current.Actor == nullptr)
        {
            // Probe has been unlinked (or deleted)
            return;
        }
        break;
    }
    default:
        // Canceled
        return;
    }

    bool setLowerHemisphereToBlack = false;
    auto shader = _shader->GetShader();

    // Init
    float customCullingNear = -1;
    if (_current.Type == EntryType::EnvProbe)
    {
        auto envProbe = (EnvironmentProbe*)_current.Actor.Get();
        LOG(Info, "Updating Env probe '{0}'...", envProbe->ToString());
        Vector3 position = envProbe->GetPosition();
        float radius = envProbe->GetScaledRadius();
        float nearPlane = Math::Max(0.1f, envProbe->CaptureNearPlane);

        // Fix far plane distance
        // TODO: investigate performance of this action, maybe we could skip it?
        float farPlane = Math::Max(radius, nearPlane + 100.0f);
        Function<bool(Actor*, const Vector3&, float&)> f(&fixFarPlaneTreeExecute);
        SceneQuery::TreeExecute<const Vector3&, float&>(f, position, farPlane);

        // Setup view
        _task->View.SetUpCube(nearPlane, farPlane, position);
    }
    else if (_current.Type == EntryType::SkyLight)
    {
        auto skyLight = (SkyLight*)_current.Actor.Get();
        LOG(Info, "Updating sky light '{0}'...", skyLight->ToString());
        Vector3 position = skyLight->GetPosition();
        float nearPlane = 10.0f;
        float farPlane = Math::Max(nearPlane + 1000.0f, skyLight->SkyDistanceThreshold * 2.0f);
        customCullingNear = skyLight->SkyDistanceThreshold;

        // TODO: use setLowerHemisphereToBlack feature for SkyLight?

        // Setup view
        _task->View.SetUpCube(nearPlane, farPlane, position);
    }

    // Disable actor during baking (it cannot influence own results)
    const bool isActorActive = _current.Actor->GetIsActive();
    _current.Actor->SetIsActive(false);

    // Render scene for all faces
    for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        // Set view
        _task->View.SetFace(faceIndex);

        // Handle custom frustum for the culling (used to skip objects near the camera)
        if (customCullingNear > 0)
        {
            Matrix p;
            Matrix::PerspectiveFov(PI_OVER_2, 1.0f, customCullingNear, _task->View.Far, p);
            _task->View.CullingFrustum.SetMatrix(_task->View.View, p);
        }

        // Render frame
        Renderer::Render(_task);
        context->ClearState();

        // Copy frame to cube face
        context->SetRenderTarget(_probe->View(faceIndex));
        auto probeFrame = _output->View();
        if (setLowerHemisphereToBlack && faceIndex != 2)
        {
            MISSING_CODE("set lower hemisphere of the probe to black");
            /*
            ProbesFilter_Tmp.Set(_core.Rendering.Pool.RT2_FloatRGB.Handle);
            ProbesFilter_CoefficientMask2.Set(f != 3 ? 1.0f : 0.0f);
            ProbesFilter_Shader.Apply(5);
            _device.DrawFullscreenTriangle();
            */
        }
        else
        {
            context->Draw(probeFrame);
        }
        context->ResetRenderTarget();
    }

    // Enable actor back
    _current.Actor->SetIsActive(isActorActive);

    // Filter all lower mip levels
    {
        Data data;
        int32 mipLevels = _probe->MipLevels();
        auto cb = shader->GetCB(0);
        for (int32 mipIndex = 1; mipIndex < mipLevels; mipIndex++)
        {
            // Cache data
            int32 srcMipIndex = mipIndex - 1;
            int32 mipSize = 1 << (mipLevels - mipIndex - 1);
            data.SourceMipIndex = srcMipIndex;

            // Set viewport
            float mipSizeFloat = (float)mipSize;
            context->SetViewportAndScissors(mipSizeFloat, mipSizeFloat);

            // Filter all faces
            for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                context->ResetSR();
                context->ResetRenderTarget();

                // Filter face
                data.CubeFace = faceIndex;
                context->UpdateCB(cb, &data);
                context->BindCB(0, cb);
                context->BindSR(0, _probe->ViewArray());
                context->SetRenderTarget(_tmpFace->View(0, mipIndex));
                context->SetState(_psFilterFace);
                context->DrawFullscreenTriangle();

                context->ResetSR();
                context->ResetRenderTarget();

                // Copy face back to the cubemap
                copyTmpToFace(context, mipIndex, faceIndex);
            }
        }
    }

    // Cleanup
    context->ClearState();

    // Mark as rendered
    _updateFrameNumber = Engine::FrameCount;
    _task->Enabled = false;
}

void ProbesRenderer::copyTmpToFace(GPUContext* context, int32 mipIndex, int32 faceIndex)
{
    // Set destination render target
    context->SetRenderTarget(_probe->View(faceIndex, mipIndex));

    // Setup shader constants
    context->BindSR(1, _tmpFace->View(0, mipIndex));

    // Copy pixels
    context->SetState(_psCopyFace);
    context->DrawFullscreenTriangle();
}

#endif
