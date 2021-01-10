// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Level/Actors/BoxBrush.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Graphics/RenderTargetPool.h"

#define STEPS_SLEEP_TIME 20
#define RUN_STEP(handler) handler(); if (checkBuildCancelled()) return true; Platform::Sleep(STEPS_SLEEP_TIME)

bool ShadowsOfMordor::Builder::doWorkInner(DateTime buildStart)
{
#if HEMISPHERES_BAKE_STATE_SAVE
    _lastStateSaveTime = DateTime::Now();
    _firstStateSave = true;

    // Try to load the state that was cached during hemispheres rendering (restore rendering in case of GPU driver crash)
    if (loadState())
    {
        reportProgress(BuildProgressStep::RenderHemispheres, 0.0f);
        const int32 firstScene = _workerActiveSceneIndex;
        {
            // Wait for lightmaps to be fully loaded
            for (_workerActiveSceneIndex = 0; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
            {
                if (_scenes[_workerActiveSceneIndex]->WaitForLightmaps())
                {
                    LOG(Error, "Failed to load lightmap textures.");
                    _wasBuildCalled = false;
                    _isActive = false;
                    return true;
                }

                if (checkBuildCancelled())
                    return true;
            }

            // Continue the hemispheres rendering for the last scene from the cached position
            {
                _workerActiveSceneIndex = firstScene;
                if (runStage(RenderHemispheres, false))
                    return true;

                // Fill black holes with blurred data to prevent artifacts on the edges
                _workerStagePosition0 = 0;
                if (runStage(PostprocessLightmaps))
                    return true;

                // Wait for GPU commands to sync
                if (waitForJobDataSync())
                    return true;

                // Update lightmaps textures
                _scenes[_workerActiveSceneIndex]->UpdateLightmaps();
            }
            for (_workerActiveSceneIndex = firstScene + 1; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
            {
                // Skip scenes without any lightmaps
                if (_scenes[_workerActiveSceneIndex]->Lightmaps.IsEmpty())
                    continue;

                // Clear hemispheres target
                _workerStagePosition0 = 0;
                if (runStage(ClearLightmapData))
                    return true;

                // Render all registered Hemispheres rendering
                _workerStagePosition0 = 0;
                if (runStage(RenderHemispheres))
                    return true;

                // Fill black holes with blurred data to prevent artifacts on the edges
                _workerStagePosition0 = 0;
                if (runStage(PostprocessLightmaps))
                    return true;

                // Wait for GPU commands to sync
                if (waitForJobDataSync())
                    return true;

                // Update lightmaps textures
                _scenes[_workerActiveSceneIndex]->UpdateLightmaps();
            }
        }
        for (int32 bounce = _giBounceRunningIndex + 1; bounce < _bounceCount; bounce++)
        {
            _giBounceRunningIndex = bounce;

            // Wait for lightmaps to be fully loaded
            for (_workerActiveSceneIndex = 0; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
            {
                if (_scenes[_workerActiveSceneIndex]->WaitForLightmaps())
                {
                    LOG(Error, "Failed to load lightmap textures.");
                    _wasBuildCalled = false;
                    _isActive = false;
                    return true;
                }
                if (checkBuildCancelled())
                    return true;
            }

            // Render bounce for every scene separately
            for (_workerActiveSceneIndex = firstScene; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
            {
                // Skip scenes without any lightmaps
                if (_scenes[_workerActiveSceneIndex]->Lightmaps.IsEmpty())
                    continue;

                // Clear hemispheres target
                _workerStagePosition0 = 0;
                if (runStage(ClearLightmapData))
                    return true;

                // Render all registered Hemispheres rendering
                _workerStagePosition0 = 0;
                if (runStage(RenderHemispheres))
                    return true;

                // Fill black holes with blurred data to prevent artifacts on the edges
                _workerStagePosition0 = 0;
                if (runStage(PostprocessLightmaps))
                    return true;

                // Wait for GPU commands to sync
                if (waitForJobDataSync())
                    return true;

                // Update lightmaps textures
                _scenes[_workerActiveSceneIndex]->UpdateLightmaps();
            }
        }
        reportProgress(BuildProgressStep::RenderHemispheres, 1.0f);
        return true;
    }
#endif

    // Compute the final weight for integration
    {
        float weightSum = 0.0f;
        for (uint32 y = 0; y < HEMISPHERES_RESOLUTION; y++)
        {
            const float v = (float(y) / float(HEMISPHERES_RESOLUTION)) * 2.0f - 1.0f;
            for (uint32 x = 0; x < HEMISPHERES_RESOLUTION; x++)
            {
                const float u = (float(x) / float(HEMISPHERES_RESOLUTION)) * 2.0f - 1.0f;
                const float t = 1.0f + u * u + v * v;
                const float weight = 4.0f / (Math::Sqrt(t) * t);
                weightSum += weight;
            }
        }
        weightSum *= 6;
        _hemisphereTexelsTotalWeight = (4.0f * PI) / weightSum;
    }

    // Initialize the lightmaps and pack entries to the charts
    for (_workerActiveSceneIndex = 0; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
    {
        RUN_STEP(cacheEntries);
        RUN_STEP(generateCharts);
        RUN_STEP(packCharts);
        RUN_STEP(updateLightmaps);
        RUN_STEP(updateEntries);
    }

    // TODO: if settings require wait for asset dependencies to all materials and models be loaded (maybe only for higher quality profiles)

    // Generate hemispheres cache and prepare for baking
    for (_workerActiveSceneIndex = 0; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
    {
        // Wait for lightmaps to be fully loaded
        if (_scenes[_workerActiveSceneIndex]->WaitForLightmaps())
        {
            LOG(Error, "Failed to load lightmap textures.");
            _wasBuildCalled = false;
            _isActive = false;
            return true;
        }

        ASSERT(_cachePositions == nullptr && _cacheNormals == nullptr);
        const int32 atlasSize = (int32)_scenes[_workerActiveSceneIndex]->GetSettings().AtlasSize;
        auto tempDesc = GPUTextureDescription::New2D(atlasSize, atlasSize, HemispheresFormatToPixelFormat[CACHE_POSITIONS_FORMAT]);
        _cachePositions = RenderTargetPool::Get(tempDesc);
        tempDesc.Format = HemispheresFormatToPixelFormat[CACHE_NORMALS_FORMAT];
        _cacheNormals = RenderTargetPool::Get(tempDesc);
        if (_cachePositions == nullptr || _cacheNormals == nullptr)
            return true;

        generateHemispheres();

        RenderTargetPool::Release(_cachePositions);
        _cachePositions = nullptr;
        RenderTargetPool::Release(_cacheNormals);
        _cacheNormals = nullptr;

        if (checkBuildCancelled())
            return true;
        Platform::Sleep(STEPS_SLEEP_TIME);
    }

    // Prepare before actual baking
    int32 hemispheresCount = 0;
    int32 mergedHemispheresCount = 0;
    int32 bounceCount = 0;
    int32 lightmapsCount = 0;
    int32 entriesCount = 0;
    for (int32 sceneIndex = 0; sceneIndex < _scenes.Count(); sceneIndex++)
    {
        auto& scene = *_scenes[sceneIndex];
        hemispheresCount += scene.HemispheresCount;
        mergedHemispheresCount += scene.MergedHemispheresCount;
        lightmapsCount += scene.Lightmaps.Count();
        entriesCount += scene.Entries.Count();
        bounceCount = Math::Max(bounceCount, scene.GetSettings().BounceCount);

        // Cleanup unused data to reduce memory usage
        scene.Entries.Resize(0);
        scene.Charts.Resize(0);
        for (auto& lightmap : scene.Lightmaps)
            lightmap.Entries.Resize(0);
    }
    _bounceCount = bounceCount;
    LOG(Info, "Rendering {0} hemispheres in {1} bounce(s) (merged: {2})", hemispheresCount, bounceCount, mergedHemispheresCount);
    if (bounceCount <= 0 || hemispheresCount <= 0)
    {
        LOG(Warning, "No data to render");
        return true;
    }

    // For each bounce
    for (int32 bounce = 0; bounce < _bounceCount; bounce++)
    {
        _giBounceRunningIndex = bounce;

        // Wait for lightmaps to be fully loaded
        for (_workerActiveSceneIndex = 0; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
        {
            if (_scenes[_workerActiveSceneIndex]->WaitForLightmaps())
            {
                LOG(Error, "Failed to load lightmap textures.");
                _wasBuildCalled = false;
                _isActive = false;
                return true;
            }
            if (checkBuildCancelled())
                return true;
        }

        // Render bounce for every scene separately
        for (_workerActiveSceneIndex = 0; _workerActiveSceneIndex < _scenes.Count(); _workerActiveSceneIndex++)
        {
            // Skip scenes without any lightmaps
            if (_scenes[_workerActiveSceneIndex]->Lightmaps.IsEmpty())
                continue;

            // Clear hemispheres target
            _workerStagePosition0 = 0;
            if (runStage(ClearLightmapData))
                return true;

            // Render all registered Hemispheres rendering
            _workerStagePosition0 = 0;
            if (runStage(RenderHemispheres))
                return true;

            // Fill black holes with blurred data to prevent artifacts on the edges
            _workerStagePosition0 = 0;
            if (runStage(PostprocessLightmaps))
                return true;

            // Wait for GPU commands to sync
            if (waitForJobDataSync())
                return true;

            // Update lightmaps textures
            _scenes[_workerActiveSceneIndex]->UpdateLightmaps();
        }
    }

    reportProgress(BuildProgressStep::RenderHemispheres, 1.0f);

#if DEBUG_EXPORT_HEMISPHERES_PREVIEW
	for (int32 sceneIndex = 0; sceneIndex < _scenes.Count(); sceneIndex++)
		downloadDebugHemisphereAtlases(_scenes[sceneIndex]);
#endif

    // References:
    // "Optimization of numerical calculations execution time in multiprocessor systems" - Wojciech Figat
    // https://knarkowicz.wordpress.com/2014/07/20/lightmapping-in-anomaly-2-mobile/
    // http://the-witness.net/news/2010/09/hemicube-rendering-and-integration/
    // http://the-witness.net/news/2010/03/graphics-tech-texture-parameterization/
    // http://the-witness.net/news/2010/03/graphics-tech-lighting-comparison/

    // Some ideas:
    // - render hemispheres to atlas or sth and batch integration and downscalling for multiply texels
    // - use conservative rasterization for dx12 instead of blur or MSAA for all platforms
    // - use hemisphere depth buffer to compute AO

    // End
    const int32 hemispheresRenderedCount = hemispheresCount * bounceCount;
    DateTime buildEnd = DateTime::NowUTC();
    LOG(Info, "Building lightmap finished! Time: {0}s, Lightmaps: {1}, Entries: {2}, Hemicubes rendered: {3}",
        static_cast<int32>((buildEnd - buildStart).GetTotalSeconds()),
        lightmapsCount,
        entriesCount,
        hemispheresRenderedCount);

    return false;
}

int32 ShadowsOfMordor::Builder::doWork()
{
    // Start
    bool buildFailed = true;
    DateTime buildStart = DateTime::NowUTC();
    _lastStep = BuildProgressStep::CacheEntries;
    _lastStepStart = buildStart;
    _hemispheresPerJob = HEMISPHERES_PER_JOB_MIN;
    _hemispheresPerJobUpdateTime = DateTime::Now();
    LOG(Info, "Start building lightmaps...");
    _isActive = true;
    OnBuildStarted();
    reportProgress(BuildProgressStep::Initialize, 0.1f);

    // Check resources and state
    if (checkBuildCancelled() || initResources())
    {
        _wasBuildCalled = false;

        // Fire event
        _isActive = false;
        OnBuildFinished(buildFailed);

        // Back
        return 0;
    }

    // Wait for the scene rendering service to be ready
    reportProgress(BuildProgressStep::Initialize, 0.5f);
    if (!Renderer::IsReady())
    {
        const int32 stepSize = 5;
        const int32 maxWaitTime = 30000;
        int32 stepsCount = static_cast<int32>(maxWaitTime / stepSize);
        while (!Renderer::IsReady() && stepsCount-- > 0)
            Platform::Sleep(stepSize);
        if (!Renderer::IsReady())
        {
            LOG(Error, "Failed to initialize Renderer service.");
            _wasBuildCalled = false;
            _isActive = false;
            OnBuildFinished(buildFailed);
            return 0;
        }
    }

    // Init scenes cache
    reportProgress(BuildProgressStep::Initialize, 0.7f);
    {
        Array<Scene*> scenes;
        Level::GetScenes(scenes);
        if (scenes.Count() == 0)
        {
            LOG(Warning, "No scenes to bake lightmaps.");
            _wasBuildCalled = false;
            _isActive = false;
            OnBuildFinished(false);
            return 0;
        }
        _scenes.Resize(scenes.Count());
        for (int32 sceneIndex = 0; sceneIndex < scenes.Count(); sceneIndex++)
        {
            _scenes[sceneIndex] = New<SceneBuildCache>();
            if (_scenes[sceneIndex]->Init(this, sceneIndex, scenes[sceneIndex]))
            {
                LOG(Error, "Failed to initialize Scene Build Cache data.");
                _wasBuildCalled = false;
                _isActive = false;
                OnBuildFinished(buildFailed);
                return 0;
            }
        }
    }

    // Run
    IsBakingLightmaps = true;
    buildFailed = doWorkInner(buildStart);
    if (buildFailed && !checkBuildCancelled())
    {
        OnBuildFinished(buildFailed);
        return 0;
    }

    // Cleanup cached data
    reportProgress(BuildProgressStep::Cleanup, 0.0f);

    _locker.Lock();

    // Clear
    _wasBuildCalled = false;
    IsBakingLightmaps = false;
    if (!Globals::FatalErrorOccurred)
        deleteState();

    // Release scenes data
    reportProgress(BuildProgressStep::Cleanup, 0.5f);
    for (int32 sceneIndex = 0; sceneIndex < _scenes.Count(); sceneIndex++)
        _scenes[sceneIndex]->Release();
    _scenes.ClearDelete();
    _scenes.Resize(0);

    _locker.Unlock();

    // Cleanup
    releaseResources();

    // Fire events
    reportProgress(BuildProgressStep::Cleanup, 1.0f);
    _isActive = false;
    OnBuildFinished(buildFailed);

    return 0;
}
