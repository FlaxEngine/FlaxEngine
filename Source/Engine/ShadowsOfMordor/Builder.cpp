// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Level.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/RenderTargetPool.h"

namespace ShadowsOfMordor
{
    bool IsRunningRadiancePass = false;
    bool EnableLightmapsUsage = true;

    float BuildProgressStepProgress[9] =
    {
        0.01f,
        // Initialize
        0.017f,
        // CacheEntries,
        0.002f,
        // GenerateLightmapCharts,
        0.002f,
        // PackLightmapCharts,
        0.028f,
        // UpdateLightmapsCollection,
        0.004f,
        // UpdateEntries,
        0.018f,
        // GenerateHemispheresCache,
        0.90f,
        // RenderHemispheres,
        0.01f,
        // Cleanup,
    }; // Sum == 1

    PixelFormat HemispheresFormatToPixelFormat[2] =
    {
        PixelFormat::R32G32B32A32_Float,
        PixelFormat::R16G16B16A16_Float,
    };

    float GetProgressBeforeStep(BuildProgressStep step)
    {
        float sum = 0;
        for (int32 i = 0; i < static_cast<int32>(step); i++)
            sum += BuildProgressStepProgress[i];
        return sum;
    }

    float GetProgressWithStep(BuildProgressStep step)
    {
        float sum = 0;
        for (int32 i = 0; i <= static_cast<int32>(step); i++)
            sum += BuildProgressStepProgress[i];
        return sum;
    }
}

class ShadowsOfMordorBuilderService : public EngineService
{
public:

    ShadowsOfMordorBuilderService()
        : EngineService(TEXT("ShadowsOfMordor Builder"), 80)
    {
    }

    void Dispose() override;
};

ShadowsOfMordorBuilderService ShadowsOfMordorBuilderServiceInstance;

ShadowsOfMordor::Builder::Builder()
    : _wasBuildCalled(false)
    , _isActive(false)
    , _wasBuildCancelled(false)
{
}

void ShadowsOfMordor::Builder::Build()
{
    // To bake static lighting we have to support compute shaders
    ASSERT_LOW_LAYER(GPUDevice::Instance && GPUDevice::Instance->Limits.HasCompute);

    _locker.Lock();

    if (!_wasBuildCalled)
    {
        _wasBuildCalled = true;
        _wasBuildCancelled = 0;

        // Ensure any scene has been loaded
        ASSERT(Level::IsAnySceneLoaded());

        // Register background work
        Function<int32()> f;
        f.Bind<Builder, &Builder::doWork>(this);
        ThreadSpawner::Start(f, TEXT("GI Baking"));
    }

    _locker.Unlock();
}

void ShadowsOfMordor::Builder::CancelBuild()
{
    _locker.Lock();

    if (_wasBuildCalled)
    {
        Platform::AtomicStore(&_wasBuildCancelled, 1);
    }

    _locker.Unlock();
}

void ShadowsOfMordor::Builder::Dispose()
{
    _locker.Lock();
    const bool waitForEnd = _wasBuildCalled;
    CancelBuild();
    _locker.Unlock();

    if (waitForEnd)
    {
        // Lightmaps builder must respond always withing 100ms after cancel work signal!
        Platform::Sleep(100);
    }

    releaseResources();
}

#if HEMISPHERES_BAKE_STATE_SAVE

#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Scripting/Scripting.h"
#include "FlaxEngine.Gen.h"

namespace ShadowsOfMordor
{
    const Char* StateCacheFileName = TEXT("ShadowsOfMordor_Cache.bin");
}

#endif

void ShadowsOfMordor::Builder::CheckIfRestoreState()
{
#if HEMISPHERES_BAKE_STATE_SAVE
    // Check if there is a state to restore
    const auto path = Globals::ProjectCacheFolder / StateCacheFileName;
    if (!FileSystem::FileExists(path))
        return;

    // Ask user if restore state
    if (!CommandLine::Options.Headless.IsTrue() && MessageBox::Show(TEXT("The last Lightmaps Baking job had crashed. Do you want to restore the state and continue baking?"), TEXT("Restore lightmaps baking?"), MessageBoxButtons::YesNo, MessageBoxIcon::Question) != DialogResult::Yes)
    {
        deleteState();
        return;
    }

    // Skip compilation on startup so editor will just load binaries
    CommandLine::Options.SkipCompile = true;
#endif
}

bool ShadowsOfMordor::Builder::RestoreState()
{
#if HEMISPHERES_BAKE_STATE_SAVE
    // Check if there is a state to restore
    const auto path = Globals::ProjectCacheFolder / StateCacheFileName;
    if (!FileSystem::FileExists(path))
        return false;

    // Open file
    LOG(Info, "Restoring the lightmaps baking state...");
    auto stream = FileReadStream::Open(path);
    int32 version;
    stream->ReadInt32(&version);
    if (version != 1)
    {
        LOG(Error, "Invalid version.");
        Delete(stream);
        deleteState();
        return false;
    }
    int32 scenesCount;
    stream->ReadInt32(&scenesCount);

    // Open scenes used during baking
    for (int32 i = 0; i < scenesCount; i++)
    {
        Guid id;
        stream->Read(&id);
        Level::LoadScene(id);
    }

    Delete(stream);

    return true;
#else
    return false;
#endif
}

void ShadowsOfMordor::Builder::saveState()
{
#if HEMISPHERES_BAKE_STATE_SAVE
    const auto path = Globals::ProjectCacheFolder / StateCacheFileName;
    const auto pathTmp = path + TEXT(".tmp");
    auto stream = FileWriteStream::Open(pathTmp);

    LOG(Info, "Saving the lightmaps baking state (scene: {0}, lightmap: {1}, hemisphere: {2})", _workerActiveSceneIndex, _workerStagePosition0, _workerStagePosition1);

    // Save all scenes on first state saving (actors have modified lightmap entries mapping to the textures and scene lightmaps list has been edited)
    if (_firstStateSave)
    {
        _firstStateSave = false;
        Level::SaveAllScenes();
    }

    // Format version
    stream->WriteInt32(1);

    // Scenes ids
    stream->WriteInt32(_scenes.Count());
    for (int32 i = 0; i < _scenes.Count(); i++)
        stream->Write(&_scenes[i]->Scene->GetID());

    // State
    stream->WriteInt32(_giBounceRunningIndex);
    stream->WriteInt32(_bounceCount);
    stream->WriteInt32(_workerActiveSceneIndex);
    stream->WriteInt32(_workerStagePosition0);
    stream->WriteInt32(_workerStagePosition1);
    stream->WriteFloat(_hemisphereTexelsTotalWeight);

    // Scenes data
    for (int32 sceneIndex = 0; sceneIndex < _scenes.Count(); sceneIndex++)
    {
        auto& scene = _scenes[sceneIndex];
        stream->WriteInt32(scene->LightmapsCount);
        stream->WriteInt32(scene->HemispheresCount);
        stream->WriteInt32(scene->MergedHemispheresCount);

        if (scene->LightmapsCount == 0)
            continue;
        auto lightmapDataStaging = scene->Lightmaps[0].LightmapData->ToStagingReadback();
        for (int32 lightmapIndex = 0; lightmapIndex < scene->LightmapsCount; lightmapIndex++)
        {
            auto& lightmap = scene->Lightmaps[lightmapIndex];

            // Hemispheres
            stream->WriteInt32(lightmap.Hemispheres.Count());
            stream->WriteBytes(lightmap.Hemispheres.Get(), lightmap.Hemispheres.Count() * sizeof(HemisphereData));

            // Lightmap Data
            // TODO: instead of doing hackish flush/sleep just copy data to some temporary buffer one frame before saving the state
            ASSERT(GPUDevice::Instance->IsRendering());
            auto context = GPUDevice::Instance->GetMainContext();
            const int32 lightmapDataSize = lightmapDataStaging->GetSize();
            context->CopyBuffer(lightmapDataStaging, lightmap.LightmapData, lightmapDataSize);
            context->Flush();
            Platform::Sleep(10);
            void* mapped = lightmapDataStaging->Map(GPUResourceMapMode::Read);
            stream->WriteInt32(lightmapDataSize);
            stream->WriteBytes(mapped, lightmapDataSize);
            lightmapDataStaging->Unmap();
        }
        SAFE_DELETE_GPU_RESOURCE(lightmapDataStaging);
    }

    Delete(stream);

    // Update the cache file
    if (FileSystem::FileExists(path))
        FileSystem::DeleteFile(path);
    FileSystem::MoveFile(path, pathTmp);
    FileSystem::DeleteFile(pathTmp);

    _lastStateSaveTime = DateTime::Now();
#endif
}

bool ShadowsOfMordor::Builder::loadState()
{
#if HEMISPHERES_BAKE_STATE_SAVE
    const auto path = Globals::ProjectCacheFolder / StateCacheFileName;
    if (!FileSystem::FileExists(path))
        return false;

    // Open file
    LOG(Info, "Loading the lightmaps baking state...");
    auto stream = FileReadStream::Open(path);
    int32 version;
    stream->ReadInt32(&version);
    if (version != 1)
    {
        LOG(Error, "Invalid version.");
        Delete(stream);
        deleteState();
        return false;
    }
    int32 scenesCount;
    stream->ReadInt32(&scenesCount);

    // Verify if scenes used during baking are loaded
    if (Level::Scenes.Count() != scenesCount || scenesCount != _scenes.Count())
    {
        LOG(Error, "Invalid scenes.");
        Delete(stream);
        deleteState();
        return false;
    }
    for (int32 i = 0; i < scenesCount; i++)
    {
        Guid id;
        stream->Read(&id);
        if (Level::Scenes[i]->GetID() != id || _scenes[i]->SceneIndex != i)
        {
            LOG(Error, "Invalid scenes.");
            Delete(stream);
            deleteState();
            return false;
        }
    }

    // State
    stream->ReadInt32(&_giBounceRunningIndex);
    stream->ReadInt32(&_bounceCount);
    stream->ReadInt32(&_workerActiveSceneIndex);
    stream->ReadInt32(&_workerStagePosition0);
    stream->ReadInt32(&_workerStagePosition1);
    stream->ReadFloat(&_hemisphereTexelsTotalWeight);

    // Scenes data
    for (int32 sceneIndex = 0; sceneIndex < _scenes.Count(); sceneIndex++)
    {
        auto& scene = _scenes[sceneIndex];
        stream->ReadInt32(&scene->LightmapsCount);
        stream->ReadInt32(&scene->HemispheresCount);
        stream->ReadInt32(&scene->MergedHemispheresCount);

        scene->Lightmaps.Resize(scene->LightmapsCount);
        if (scene->LightmapsCount == 0)
            continue;
        for (int32 lightmapIndex = 0; lightmapIndex < scene->LightmapsCount; lightmapIndex++)
        {
            auto& lightmap = scene->Lightmaps[lightmapIndex];
            lightmap.Init(&scene->GetSettings());

            // Hemispheres
            int32 hemispheresCount;
            stream->ReadInt32(&hemispheresCount);
            lightmap.Hemispheres.Resize(hemispheresCount);
            stream->ReadBytes(lightmap.Hemispheres.Get(), lightmap.Hemispheres.Count() * sizeof(HemisphereData));

            // Lightmap Data
            int32 lightmapDataSize;
            stream->ReadInt32(&lightmapDataSize);
            const auto lightmapData = lightmap.LightmapData;
            if (lightmapDataSize != lightmapData->GetSize())
            {
                LOG(Error, "Invalid lightmap data size.");
                Delete(stream);
                deleteState();
                return false;
            }
            lightmap.LightmapDataInit.Resize(lightmapDataSize);
            stream->ReadBytes(lightmap.LightmapDataInit.Get(), lightmapDataSize);
        }
    }

    Delete(stream);

    _firstStateSave = false;
    _lastStateSaveTime = DateTime::Now();
    return true;
#else
    return false;
#endif
}

void ShadowsOfMordor::Builder::deleteState()
{
#if HEMISPHERES_BAKE_STATE_SAVE
    const auto path = Globals::ProjectCacheFolder / StateCacheFileName;
    if (FileSystem::FileExists(path))
        FileSystem::DeleteFile(path);
#endif
}

void ShadowsOfMordor::Builder::reportProgress(BuildProgressStep step, float stepProgress)
{
    const auto currentStepTotalProgress = BuildProgressStepProgress[(int32)step];

    // Apply scenes progress
    //stepProgress = (_workerActiveSceneIndex + stepProgress) / _scenes.Count();

    // Get progress in 'before' steps
    reportProgress(step, stepProgress, GetProgressBeforeStep(step) + stepProgress * currentStepTotalProgress);
}

void ShadowsOfMordor::Builder::reportProgress(BuildProgressStep step, float stepProgress, float totalProgress)
{
    // Send step changes info
    if (_lastStep != step)
    {
        const auto now = DateTime::Now();
        LOG(Info, "Lightmaps baking step {0} time: {1}s", ShadowsOfMordor::ToString(_lastStep), Math::RoundToInt((float)(now - _lastStepStart).GetTotalSeconds()));

        _lastStep = step;
        _lastStepStart = now;
    }

    // Send event
    OnBuildProgress(step, stepProgress, totalProgress);
}

bool ShadowsOfMordor::Builder::initResources()
{
    // TODO: remove this release and just create missing resources
    releaseResources();

    _output = GPUTexture::New();
    if (_output->Init(GPUTextureDescription::New2D(HEMISPHERES_RESOLUTION, HEMISPHERES_RESOLUTION, PixelFormat::R11G11B10_Float)))
        return true;
    _task = New<BuilderRenderTask>();
    _task->Enabled = false;
    _task->Output = _output;
    auto& view = _task->View;
    view.Mode = ViewMode::NoPostFx;
    view.Flags =
            ViewFlags::GI |
            ViewFlags::DirectionalLights |
            ViewFlags::PointLights |
            ViewFlags::SpotLights |
            ViewFlags::Shadows |
            ViewFlags::Decals |
            ViewFlags::SkyLights |
            ViewFlags::Reflections;
    view.IsOfflinePass = true;
    view.Near = HEMISPHERES_NEAR_PLANE;
    view.Far = HEMISPHERES_FAR_PLANE;
    view.StaticFlagsMask = StaticFlags::Lightmap;
    view.MaxShadowsQuality = Quality::Low;
    _task->Resize(HEMISPHERES_RESOLUTION, HEMISPHERES_RESOLUTION);

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/BakeLightmap"));
    if (_shader == nullptr)
        return true;
    if (_shader->WaitForLoaded())
        return true;

    _psRenderCacheModel = GPUDevice::Instance->CreatePipelineState();
    GPUPipelineState::Description desc = GPUPipelineState::Description::DefaultNoDepth;
    desc.CullMode = CullMode::TwoSided;
    desc.VS = _shader->GetShader()->GetVS("VS_RenderCacheModel");
    desc.PS = _shader->GetShader()->GetPS("PS_RenderCache");
    if (_psRenderCacheModel->Init(desc))
        return true;

    _psRenderCacheTerrain = GPUDevice::Instance->CreatePipelineState();
    desc.VS = _shader->GetShader()->GetVS("VS_RenderCacheTerrain");
    if (_psRenderCacheTerrain->Init(desc))
        return true;

    _psBlurCache = GPUDevice::Instance->CreatePipelineState();
    desc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    desc.PS = _shader->GetShader()->GetPS("PS_BlurCache");
    if (_psBlurCache->Init(desc))
        return true;

    _irradianceReduction = GPUDevice::Instance->CreateBuffer(TEXT("IrradianceReduction"));
    if (_irradianceReduction->Init(GPUBufferDescription::Typed(HEMISPHERES_RESOLUTION * NUM_SH_TARGETS, HemispheresFormatToPixelFormat[HEMISPHERES_IRRADIANCE_FORMAT], true)))
        return true;

#if DEBUG_EXPORT_HEMISPHERES_PREVIEW
    releaseDebugHemisphereAtlases();
#endif

    return false;
}

void ShadowsOfMordor::Builder::releaseResources()
{
#if DEBUG_EXPORT_HEMISPHERES_PREVIEW
    releaseDebugHemisphereAtlases();
#endif

    SAFE_DELETE_GPU_RESOURCE(_psRenderCacheModel);
    SAFE_DELETE_GPU_RESOURCE(_psRenderCacheTerrain);
    SAFE_DELETE_GPU_RESOURCE(_psBlurCache);
    _shader = nullptr;

    SAFE_DELETE_GPU_RESOURCE(_irradianceReduction);

    RenderTargetPool::Release(_cachePositions);
    _cachePositions = nullptr;
    RenderTargetPool::Release(_cacheNormals);
    _cacheNormals = nullptr;

    if (_output)
        _output->ReleaseGPU();

    SAFE_DELETE(_task);
    SAFE_DELETE_GPU_RESOURCE(_output);
}

bool ShadowsOfMordor::Builder::waitForJobDataSync()
{
    bool wasCancelled = false;
    const int32 framesToSyncCount = 3;

    while (!wasCancelled)
    {
        Platform::Sleep(1);

        wasCancelled = checkBuildCancelled();
        if (_lastJobFrame + framesToSyncCount <= Engine::FrameCount)
            break;
    }

    return wasCancelled;
}

void ShadowsOfMordorBuilderService::Dispose()
{
    ShadowsOfMordor::Builder::Instance()->Dispose();
}
