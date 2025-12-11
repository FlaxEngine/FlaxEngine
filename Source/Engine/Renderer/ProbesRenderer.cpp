// Copyright (c) Wojciech Figat. All rights reserved.

#include "ProbesRenderer.h"
#include "Renderer.h"
#include "RenderList.h"
#include "ReflectionsPass.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Level/Actors/PointLight.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"
#include "Engine/Level/Actors/SkyLight.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Level/LargeWorlds.h"
#include "Engine/ContentExporters/AssetExporters.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Threading/ThreadPoolTask.h"

// Amount of frames to wait for data from probe update job
#define PROBES_RENDERER_LATENCY_FRAMES 1

struct ProbeEntry
{
    enum class Types
    {
        Invalid = 0,
        EnvProbe = 1,
        SkyLight = 2,
    };

    Types Type = Types::Invalid;
    float Timeout = 0.0f;
    ScriptingObjectReference<Actor> Actor;

    bool UseTextureData() const;
    int32 GetResolution() const;
    PixelFormat GetFormat() const;
};

// Custom task called after downloading probe texture data to save it.
class DownloadProbeTask : public ThreadPoolTask
{
private:
    GPUTexture* _texture;
    TextureData _data;
    ProbeEntry _entry;

public:
    DownloadProbeTask(GPUTexture* target, const ProbeEntry& entry)
        : _texture(target)
        , _entry(entry)
    {
    }

    FORCE_INLINE TextureData& GetData()
    {
        return _data;
    }

    bool Run() override
    {
        Actor* actor = _entry.Actor.Get();
        if (_entry.Type == ProbeEntry::Types::EnvProbe)
        {
            if (actor)
                ((EnvironmentProbe*)actor)->SetProbeData(_data);
        }
        else if (_entry.Type == ProbeEntry::Types::SkyLight)
        {
            if (actor)
                ((SkyLight*)actor)->SetProbeData(_data);
        }
        else
        {
            return true;
        }

        ProbesRenderer::OnFinishBake(actor);
        return false;
    }
};

GPU_CB_STRUCT(Data {
    Float2 Dummy0;
    int32 CubeFace;
    float SourceMipIndex;
    });

class ProbesRendererService : public EngineService
{
private:
    bool _initDone = false;
    bool _initFailed = false;

    TimeSpan _lastProbeUpdate = TimeSpan(0);
    Array<ProbeEntry> _probesToBake;

    ProbeEntry _current;
    int32 _workStep;
    float _customCullingNear;

    AssetReference<Shader> _shader;
    GPUPipelineState* _psFilterFace = nullptr;
    SceneRenderTask* _task = nullptr;
    GPUTexture* _output = nullptr;
    GPUTexture* _probe = nullptr;
    GPUTexture* _tmpFace = nullptr;
    uint64 _updateFrameNumber = 0;

public:
    ProbesRendererService()
        : EngineService(TEXT("Probes Renderer"), 500)
    {
    }

    bool LazyInit();
    bool InitShader();
    void Update() override;
    void Dispose() override;
    void Bake(const ProbeEntry& e);

private:
    void OnRender(RenderTask* task, GPUContext* context);
    void OnSetupRender(RenderContext& renderContext);
#if COMPILE_WITH_DEV_ENV
    bool _initShader = false;
    void OnShaderReloading(Asset* obj)
    {
        _initShader = true;
        SAFE_DELETE_GPU_RESOURCE(_psFilterFace);
    }
#endif
};

ProbesRendererService ProbesRendererServiceInstance;

TimeSpan ProbesRenderer::UpdateDelay(0, 0, 0, 0, 100);
TimeSpan ProbesRenderer::ReleaseTimeout(0, 0, 0, 30);
int32 ProbesRenderer::MaxWorkPerFrame = 1;
Delegate<Actor*> ProbesRenderer::OnRegisterBake;
Delegate<Actor*> ProbesRenderer::OnFinishBake;

void ProbesRenderer::Bake(EnvironmentProbe* probe, float timeout)
{
    if (!probe || probe->IsUsingCustomProbe())
        return;
    ProbeEntry e;
    e.Type = ProbeEntry::Types::EnvProbe;
    e.Actor = probe;
    e.Timeout = timeout;
    ProbesRendererServiceInstance.Bake(e);
}

void ProbesRenderer::Bake(SkyLight* probe, float timeout)
{
    if (!probe)
        return;
    ProbeEntry e;
    e.Type = ProbeEntry::Types::SkyLight;
    e.Actor = probe;
    e.Timeout = timeout;
    ProbesRendererServiceInstance.Bake(e);
}

bool ProbeEntry::UseTextureData() const
{
    if (Type == Types::EnvProbe && Actor)
    {
        switch (Actor.As<EnvironmentProbe>()->UpdateMode)
        {
        case EnvironmentProbe::ProbeUpdateMode::Realtime:
            return false;
        }
    }
    return true;
}

int32 ProbeEntry::GetResolution() const
{
    auto resolution = ProbeCubemapResolution::UseGraphicsSettings;
    if (Type == Types::EnvProbe && Actor)
        resolution = ((EnvironmentProbe*)Actor.Get())->CubemapResolution;
    else if (Type == Types::SkyLight)
        resolution = ProbeCubemapResolution::_128;
    if (resolution == ProbeCubemapResolution::UseGraphicsSettings)
        resolution = GraphicsSettings::Get()->DefaultProbeResolution;
    if (resolution == ProbeCubemapResolution::UseGraphicsSettings)
        resolution = ProbeCubemapResolution::_128;
    return (int32)resolution;
}

PixelFormat ProbeEntry::GetFormat() const
{
    return GraphicsSettings::Get()->UseHDRProbes ? PixelFormat::R11G11B10_Float : PixelFormat::R8G8B8A8_UNorm;
}

bool ProbesRendererService::LazyInit()
{
    if (_initDone || _initFailed)
        return false;

    // Load shader
    if (_shader == nullptr)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/ProbesFilter"));
        _initFailed = _shader == nullptr;
        if (_initFailed)
            return false;
#if COMPILE_WITH_DEV_ENV
        _shader->OnReloading.Bind<ProbesRendererService, &ProbesRendererService::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
        return true;
    _initFailed |= InitShader();

    // Init rendering pipeline
    _output = GPUDevice::Instance->CreateTexture(TEXT("ProbesRenderer.Output"));
    const int32 probeResolution = _current.GetResolution();
    const PixelFormat probeFormat = _current.GetFormat();
    _initFailed |= _output->Init(GPUTextureDescription::New2D(probeResolution, probeResolution, probeFormat));
    _task = New<SceneRenderTask>();
    auto task = _task;
    task->Order = -100; // Run before main view rendering (realtime probes will get smaller latency)
    task->Enabled = false;
    task->IsCustomRendering = true;
    task->ActorsSource = ActorsSources::ScenesAndCustomActors;
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
        ViewFlags::Sky |
        ViewFlags::Fog;
    view.Mode = ViewMode::NoPostFx;
    view.IsOfflinePass = true;
    view.IsSingleFrame = true;
    view.StaticFlagsMask = view.StaticFlagsCompare = StaticFlags::ReflectionProbe;
    task->IsCameraCut = true;
    task->Resize(probeResolution, probeResolution);
    task->Render.Bind<ProbesRendererService, &ProbesRendererService::OnRender>(this);
    task->SetupRender.Bind<ProbesRendererService, &ProbesRendererService::OnSetupRender>(this);

    // Init render targets
    _probe = GPUDevice::Instance->CreateTexture(TEXT("ProbesRenderer.Probe"));
    _initFailed |= _probe->Init(GPUTextureDescription::NewCube(probeResolution, probeFormat, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews, 0));
    _tmpFace = GPUDevice::Instance->CreateTexture(TEXT("ProbesRenderer.TmpFace"));
    _initFailed |= _tmpFace->Init(GPUTextureDescription::New2D(probeResolution, probeResolution, 0, probeFormat, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews));

    // Mark as ready
    _initDone = true;
    return false;
}

bool ProbesRendererService::InitShader()
{
    const auto shader = _shader->GetShader();
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
    _psFilterFace = GPUDevice::Instance->CreatePipelineState();
    auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    psDesc.PS = shader->GetPS("PS_FilterFace");
    return _psFilterFace->Init(psDesc);
}

void ProbesRendererService::Update()
{
    PROFILE_MEM(Graphics);

    // Calculate time delta since last update
    auto timeNow = Time::Update.UnscaledTime;
    auto timeSinceUpdate = timeNow - _lastProbeUpdate;
    if (timeSinceUpdate < 0)
    {
        _lastProbeUpdate = timeNow;
        timeSinceUpdate = 0;
    }

    // Check if render job is done
    if (_updateFrameNumber > 0 && _updateFrameNumber + PROBES_RENDERER_LATENCY_FRAMES <= Engine::FrameCount)
    {
        // Create async job to gather probe data from the GPU
        GPUTexture* texture = nullptr;
        switch (_current.Type)
        {
        case ProbeEntry::Types::SkyLight:
        case ProbeEntry::Types::EnvProbe:
            texture = _probe;
            break;
        }
        ASSERT(texture && _current.UseTextureData());
        auto taskB = New<DownloadProbeTask>(texture, _current);
        auto taskA = texture->DownloadDataAsync(taskB->GetData());
        ASSERT(taskA);
        taskA->ContinueWith(taskB);
        taskA->Start();

        // Clear flag
        _updateFrameNumber = 0;
        _workStep = 0;
        _current.Type = ProbeEntry::Types::Invalid;
    }
    else if (_current.Type == ProbeEntry::Types::Invalid && timeSinceUpdate > ProbesRenderer::UpdateDelay)
    {
        int32 firstValidEntryIndex = -1;
        auto dt = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
        for (int32 i = 0; i < _probesToBake.Count(); i++)
        {
            auto& e = _probesToBake[i];
            e.Timeout -= dt;
            if (e.Timeout <= 0)
            {
                firstValidEntryIndex = i;
                break;
            }
        }

        // Check if need to update probe
        if (firstValidEntryIndex >= 0 && timeSinceUpdate > ProbesRenderer::UpdateDelay)
        {
            if (LazyInit())
                return; // Shader is not yet loaded so try the next frame

            // Mark probe to update
            _current = _probesToBake[firstValidEntryIndex];
            _probesToBake.RemoveAtKeepOrder(firstValidEntryIndex);
            _task->Enabled = true;
            _updateFrameNumber = 0;
            _workStep = 0;
            _lastProbeUpdate = timeNow;
        }
        // Check if need to release data
        else if (_initDone && timeSinceUpdate > ProbesRenderer::ReleaseTimeout)
        {
            // Release resources
            Dispose();
        }
    }
}

void ProbesRendererService::Dispose()
{
    if (!_initDone && !_initFailed)
        return;
    ASSERT(_updateFrameNumber == 0);
    if (_output)
        _output->ReleaseGPU();
    SAFE_DELETE_GPU_RESOURCE(_psFilterFace);
    SAFE_DELETE_GPU_RESOURCE(_output);
    SAFE_DELETE_GPU_RESOURCE(_probe);
    SAFE_DELETE_GPU_RESOURCE(_tmpFace);
    SAFE_DELETE(_task);
    _shader = nullptr;
    _initDone = false;
    _initFailed = false;
}

void ProbesRendererService::Bake(const ProbeEntry& e)
{
    // Check if already registered for bake
    for (ProbeEntry& p : _probesToBake)
    {
        if (p.Type == e.Type && p.Actor == e.Actor)
        {
            p.Timeout = e.Timeout;
            return;
        }
    }

    _probesToBake.Add(e);

    // Fire event
    if (e.UseTextureData())
        ProbesRenderer::OnRegisterBake(e.Actor);
}

static bool FixFarPlane(Actor* actor, const Vector3& position, float& farPlane)
{
    if (auto* pointLight = dynamic_cast<PointLight*>(actor))
    {
        const Real dst = Vector3::Distance(pointLight->GetPosition(), position) + pointLight->GetScaledRadius();
        if (dst > farPlane && dst * 0.5f < farPlane)
        {
            farPlane = (float)dst;
        }
    }
    return true;
}

void ProbesRendererService::OnRender(RenderTask* task, GPUContext* context)
{
    switch (_current.Type)
    {
    case ProbeEntry::Types::EnvProbe:
    case ProbeEntry::Types::SkyLight:
    {
        if (_current.Actor == nullptr)
        {
            // Probe has been unlinked (or deleted)
            _task->Enabled = false;
            _updateFrameNumber = 0;
            _current.Type = ProbeEntry::Types::Invalid;
            return;
        }
        break;
    }
    default:
        // Canceled
        return;
    }
    ASSERT(_updateFrameNumber == 0);
    auto shader = _shader->GetShader();
    PROFILE_GPU("Render Probe");

#if COMPILE_WITH_DEV_ENV
    // handle shader hot-reload
    if (_initShader)
    {
        if (_shader->WaitForLoaded())
            return;
        _initShader = false;
        if (InitShader())
            return;
    }
#endif

    // Init
    const int32 probeResolution = _current.GetResolution();
    const PixelFormat probeFormat = _current.GetFormat();
    if (_workStep == 0)
    {
        _customCullingNear = -1;
        if (_current.Type == ProbeEntry::Types::EnvProbe)
        {
            auto envProbe = (EnvironmentProbe*)_current.Actor.Get();
            Vector3 position = envProbe->GetPosition();
            float radius = envProbe->GetScaledRadius();
            float nearPlane = Math::Max(0.1f, envProbe->CaptureNearPlane);

            // Adjust far plane distance
            float farPlane = Math::Max(radius, nearPlane + 100.0f);
            farPlane *= farPlane < 10000 ? 10 : 4;
            Function<bool(Actor*, const Vector3&, float&)> f(&FixFarPlane);
            SceneQuery::TreeExecute<const Vector3&, float&>(f, position, farPlane);

            // Setup view
            LargeWorlds::UpdateOrigin(_task->View.Origin, position);
            _task->View.SetUpCube(nearPlane, farPlane, position - _task->View.Origin);
        }
        else if (_current.Type == ProbeEntry::Types::SkyLight)
        {
            auto skyLight = (SkyLight*)_current.Actor.Get();
            Vector3 position = skyLight->GetPosition();
            float nearPlane = 10.0f;
            float farPlane = Math::Max(nearPlane + 1000.0f, skyLight->SkyDistanceThreshold * 2.0f);
            _customCullingNear = skyLight->SkyDistanceThreshold;

            // Setup view
            LargeWorlds::UpdateOrigin(_task->View.Origin, position);
            _task->View.SetUpCube(nearPlane, farPlane, position - _task->View.Origin);
        }

        // Resize buffers
        bool resizeFailed = _output->Resize(probeResolution, probeResolution, probeFormat);
        resizeFailed |= _probe->Resize(probeResolution, probeResolution, probeFormat);
        resizeFailed |= _tmpFace->Resize(probeResolution, probeResolution, probeFormat);
        resizeFailed |= _task->Resize(probeResolution, probeResolution);
        if (resizeFailed)
            LOG(Error, "Failed to resize probe");
    }

    // Disable actor during baking (it cannot influence own results)
    const bool isActorActive = _current.Actor->GetIsActive();
    _current.Actor->SetIsActive(false);

    // Lower quality when rendering probes in-game to gain performance
    _task->View.MaxShadowsQuality = Engine::IsPlayMode() || probeResolution <= 128 ? Quality::Low : Quality::Ultra;

    // Render scene for all faces
    int32 workLeft = ProbesRenderer::MaxWorkPerFrame;
    const int32 lastFace = Math::Min(_workStep + workLeft, 6);
    for (int32 faceIndex = _workStep; faceIndex < lastFace; faceIndex++)
    {
        _task->CameraCut();
        _task->View.SetFace(faceIndex);

        // Handle custom frustum for the culling (used to skip objects near the camera)
        if (_customCullingNear > 0)
        {
            Matrix p;
            Matrix::PerspectiveFov(PI_OVER_2, 1.0f, _customCullingNear, _task->View.Far, p);
            _task->View.CullingFrustum.SetMatrix(_task->View.View, p);
        }

        // Render frame
        Renderer::Render(_task);
        context->ResetState();

        // Copy frame to cube face
        {
            PROFILE_GPU("Copy Face");
            context->SetRenderTarget(_probe->View(faceIndex));
            context->SetViewportAndScissors((float)probeResolution, (float)probeResolution);
            context->Draw(_output->View());
            context->ResetRenderTarget();
        }

        // Move to the next face
        _workStep++;
        workLeft--;
    }

    // Enable actor back
    _current.Actor->SetIsActive(isActorActive);

    // Filter all lower mip levels
    if (workLeft > 0)
    {
        PROFILE_GPU("Filtering");
        Data data;
        int32 mipLevels = _probe->MipLevels();
        auto cb = shader->GetCB(0);
        for (int32 mipIndex = 1; mipIndex < mipLevels; mipIndex++)
        {
            const int32 mipSize = 1 << (mipLevels - mipIndex - 1);
            data.SourceMipIndex = (float)mipIndex - 1.0f;
            context->SetViewportAndScissors((float)mipSize, (float)mipSize);
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
                context->SetRenderTarget(_probe->View(faceIndex, mipIndex));
                context->Draw(_tmpFace->View(0, mipIndex));
            }
        }

        // End
        workLeft--;
        _workStep++;
    }

    // Cleanup
    context->ResetState();

    if (_workStep < 7)
        return; // Continue rendering next frame

    // Mark as rendered
    _updateFrameNumber = Engine::FrameCount;
    _task->Enabled = false;

    // Real-time probes don't use TextureData (for streaming) but copy generated probe directly to GPU memory
    if (!_current.UseTextureData())
    {
        if (_current.Type == ProbeEntry::Types::EnvProbe && _current.Actor)
        {
            _current.Actor.As<EnvironmentProbe>()->SetProbeData(context, _probe);
        }

        // Clear flag
        _updateFrameNumber = 0;
        _current.Type = ProbeEntry::Types::Invalid;
    }
}

void ProbesRendererService::OnSetupRender(RenderContext& renderContext)
{
    // Disable Volumetric Fog in reflection as it causes seams on cubemap face edges
    renderContext.List->Setup.UseVolumetricFog = false;
}
