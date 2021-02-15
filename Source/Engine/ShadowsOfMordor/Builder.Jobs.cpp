// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/Actors/BoxBrush.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Terrain/Terrain.h"
#include "Engine/Terrain/TerrainPatch.h"
#include "Engine/Terrain/TerrainManager.h"
#include "Engine/Foliage/Foliage.h"
#include "Engine/Profiler/Profiler.h"

namespace ShadowsOfMordor
{
    PACK_STRUCT(struct ShaderData {
        Rectangle LightmapArea;
        Matrix WorldMatrix;
        Matrix ToTangentSpace;
        float FinalWeight;
        uint32 TexelAddress;
        uint32 AtlasSize;
        float TerrainChunkSizeLOD0;
        Vector4 HeightmapUVScaleBias;
        Vector3 WorldInvScale;
        float Dummy1;
        });
}

void ShadowsOfMordor::Builder::onJobRender(GPUContext* context)
{
    auto scene = _scenes[_workerActiveSceneIndex];
    int32 atlasSize = (int32)scene->GetSettings().AtlasSize;

    switch (_stage)
    {
    case CleanLightmaps:
    {
        PROFILE_GPU_CPU_NAMED("CleanLightmaps");

        uint32 cleanerSize = 0;
        for (int32 i = 0; i < scene->Lightmaps.Count(); i++)
        {
            auto lightmap = scene->Scene->LightmapsData.GetLightmap(_workerStagePosition0);
            GPUTexture* textures[NUM_SH_TARGETS];
            lightmap->GetTextures(textures);
            for (int32 textureIndex = 0; textureIndex < NUM_SH_TARGETS; textureIndex++)
                cleanerSize = Math::Max(textures[textureIndex]->SlicePitch(), cleanerSize);
        }
        auto cleaner = Allocator::Allocate(cleanerSize);
        Platform::MemoryClear(cleaner, cleanerSize);

        for (; _workerStagePosition0 < scene->Lightmaps.Count(); _workerStagePosition0++)
        {
            auto lightmap = scene->Scene->LightmapsData.GetLightmap(_workerStagePosition0);
            GPUTexture* textures[NUM_SH_TARGETS];
            lightmap->GetTextures(textures);
            for (int32 textureIndex = 0; textureIndex < NUM_SH_TARGETS; textureIndex++)
            {
                auto texture = textures[textureIndex];
                for (int32 mipIndex = 0; mipIndex < texture->MipLevels(); mipIndex++)
                {
                    uint32 rowPitch, slicePitch;
                    texture->ComputePitch(mipIndex, rowPitch, slicePitch);
                    context->UpdateTexture(textures[textureIndex], 0, 0, cleaner, rowPitch, slicePitch);
                }
            }
        }

        Allocator::Free(cleaner);
        _wasStageDone = true;
        break;
    }
    case RenderCache:
    {
        PROFILE_GPU_CPU_NAMED("RenderCache");

        scene->EntriesLocker.Lock();

        int32 entriesToRenderLeft = CACHE_ENTRIES_PER_JOB;
        auto& lightmapEntry = scene->Lightmaps[_workerStagePosition0];
        ShaderData shaderData;
        GPUTextureView* rts[2] =
        {
            _cachePositions->View(),
            _cacheNormals->View(),
        };
        context->SetRenderTarget(nullptr, ToSpan(rts, ARRAY_COUNT(rts)));
        float atlasSizeFloat = (float)atlasSize;
        context->SetViewportAndScissors(atlasSizeFloat, atlasSizeFloat);

        // Clear targets if there is no progress for that lightmap (no entries rendered at all)
        if (_workerStagePosition1 == 0)
        {
            context->Clear(_cachePositions->View(), Color::Black);
            context->Clear(_cacheNormals->View(), Color::Black);
        }

        for (; _workerStagePosition1 < lightmapEntry.Entries.Count(); _workerStagePosition1++)
        {
            if (entriesToRenderLeft == 0)
                break;
            entriesToRenderLeft--;

            // Render entry
            auto& entry = scene->Entries[lightmapEntry.Entries[_workerStagePosition1]];
            auto cb = _shader->GetShader()->GetCB(0);
            switch (entry.Type)
            {
            case GeometryType::StaticModel:
            {
                auto staticModel = entry.AsStaticModel.Actor;
                auto& lod = staticModel->Model->LODs[0];

                Matrix worldMatrix;
                staticModel->GetWorld(&worldMatrix);
                Matrix::Transpose(worldMatrix, shaderData.WorldMatrix);
                shaderData.LightmapArea = staticModel->Lightmap.UVsArea;

                context->UpdateCB(cb, &shaderData);
                context->BindCB(0, cb);
                context->SetState(_psRenderCacheModel);
                for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                {
                    auto& mesh = lod.Meshes[meshIndex];
                    auto& materialSlot = staticModel->Entries[mesh.GetMaterialSlotIndex()];

                    if (materialSlot.Visible && mesh.HasLightmapUVs())
                    {
                        mesh.Render(context);
                    }
                }

                break;
            }
            case GeometryType::Terrain:
            {
                auto terrain = entry.AsTerrain.Actor;
                auto patch = terrain->GetPatch(entry.AsTerrain.PatchIndex);
                auto chunk = &patch->Chunks[entry.AsTerrain.ChunkIndex];
                auto chunkSize = terrain->GetChunkSize();
                const auto heightmap = patch->Heightmap.Get()->GetTexture();

                Matrix world;
                chunk->GetWorld(&world);
                Matrix::Transpose(world, shaderData.WorldMatrix);
                shaderData.LightmapArea = chunk->Lightmap.UVsArea;
                shaderData.TerrainChunkSizeLOD0 = TERRAIN_UNITS_PER_VERTEX * chunkSize;
                chunk->GetHeightmapUVScaleBias(&shaderData.HeightmapUVScaleBias);

                // Extract per axis scales from LocalToWorld transform
                const float scaleX = Vector3(world.M11, world.M12, world.M13).Length();
                const float scaleY = Vector3(world.M21, world.M22, world.M23).Length();
                const float scaleZ = Vector3(world.M31, world.M32, world.M33).Length();
                shaderData.WorldInvScale = Vector3(
                    scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
                    scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
                    scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);

                DrawCall drawCall;
                if (TerrainManager::GetChunkGeometry(drawCall, chunkSize, 0))
                    return;

                context->UpdateCB(cb, &shaderData);
                context->BindCB(0, cb);
                context->BindSR(0, heightmap);
                context->SetState(_psRenderCacheTerrain);
                context->BindIB(drawCall.Geometry.IndexBuffer);
                context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 1));
                context->DrawIndexed(drawCall.Draw.IndicesCount, 0, drawCall.Draw.StartIndex);

                break;
            }
            case GeometryType::Foliage:
            {
                auto foliage = entry.AsFoliage.Actor;
                auto& instance = foliage->Instances[entry.AsFoliage.InstanceIndex];
                auto& type = foliage->FoliageTypes[entry.AsFoliage.TypeIndex];

                Matrix::Transpose(instance.World, shaderData.WorldMatrix);
                shaderData.LightmapArea = instance.Lightmap.UVsArea;

                context->UpdateCB(cb, &shaderData);
                context->BindCB(0, cb);
                context->SetState(_psRenderCacheModel);
                type.Model->LODs[0].Meshes[entry.AsFoliage.MeshIndex].Render(context);

                break;
            }
            }
            // TODO: on directx 12 use conservative rasterization
            // TODO: we could also MSAA -> even better results
        }

        // Check if stage has been done
        if (_workerStagePosition1 >= lightmapEntry.Entries.Count())
            _wasStageDone = true;

        scene->EntriesLocker.Unlock();
        break;
    }
    case PostprocessCache:
    {
        PROFILE_GPU_CPU_NAMED("PostprocessCache");

        // In ideal case we should use analytical anti-aliasing and conservative rasterization
        // But for now let's use simple trick to blur positions and normals cache to reduce amount of black artifacts on uv edges

        auto tempDesc = GPUTextureDescription::New2D(atlasSize, atlasSize, HemispheresFormatToPixelFormat[CACHE_POSITIONS_FORMAT]);
        auto resultPositions = RenderTargetPool::Get(tempDesc);
        tempDesc.Format = HemispheresFormatToPixelFormat[CACHE_NORMALS_FORMAT];
        auto resultNormals = RenderTargetPool::Get(tempDesc);
        if (resultPositions == nullptr || resultNormals == nullptr)
        {
            RenderTargetPool::Release(resultPositions);
            RenderTargetPool::Release(resultNormals);
            LOG(Error, "Cannot get temporary targets for ShadowsOfMordor::Builder::PostprocessCache");
            _wasStageDone = true;
            break;
        }

        auto srcPositions = _cachePositions;
        auto srcNormals = _cacheNormals;

        GPUTextureView* rts[2] =
        {
            resultPositions->View(),
            resultNormals->View(),
        };
        context->SetRenderTarget(nullptr, ToSpan(rts, ARRAY_COUNT(rts)));
        float atlasSizeFloat = (float)atlasSize;
        context->SetViewportAndScissors(atlasSizeFloat, atlasSizeFloat);

        context->BindSR(0, srcNormals);
        context->BindSR(1, srcPositions);

        ShaderData shaderData;
        shaderData.AtlasSize = atlasSize;
        auto cb = _shader->GetShader()->GetCB(0);
        context->UpdateCB(cb, &shaderData);
        context->BindCB(0, cb);

        context->SetState(_psBlurCache);
        context->DrawFullscreenTriangle();

        _cachePositions = resultPositions;
        _cacheNormals = resultNormals;

        RenderTargetPool::Release(srcPositions);
        RenderTargetPool::Release(srcNormals);

        _wasStageDone = true;
        break;
    }
    case ClearLightmapData:
    {
        PROFILE_GPU_CPU_NAMED("ClearLightmapData");

        // Before hemispheres rendering we have to clear target lightmap data
        // Later we use blur shader to interpolate empty texels (so empty texels should be pure black)

        ASSERT(scene->Lightmaps.Count() > _workerStagePosition0);
        auto& lightmapEntry = scene->Lightmaps[_workerStagePosition0];

        // All black everything!
        context->ClearUA(lightmapEntry.LightmapData, Vector4::Zero);

        _wasStageDone = true;
        break;
    }
    case RenderHemispheres:
    {
        auto now = DateTime::Now();
        auto& lightmapEntry = scene->Lightmaps[_workerStagePosition0];

#if HEMISPHERES_BAKE_STATE_SAVE
        if (lightmapEntry.LightmapDataInit.HasItems())
        {
            context->UpdateBuffer(lightmapEntry.LightmapData, lightmapEntry.LightmapDataInit.Get(), lightmapEntry.LightmapDataInit.Count());
            lightmapEntry.LightmapDataInit.Resize(0);
        }

        // Every few minutes save the baking state to restore it in case of GPU driver crash
        if (now - _lastStateSaveTime >= TimeSpan::FromSeconds(HEMISPHERES_BAKE_STATE_SAVE_DELAY))
        {
            saveState();
            break;
        }
#endif

        PROFILE_GPU_CPU_NAMED("RenderHemispheres");

        // Dynamically adjust hemispheres to render per-job to minimize the bake speed but without GPU hangs
        if (now - _hemispheresPerJobUpdateTime >= TimeSpan::FromSeconds(1.0))
        {
            _hemispheresPerJobUpdateTime = now;
            const int32 fps = Engine::GetFramesPerSecond();
            int32 hemispheresPerJob = _hemispheresPerJob;
            if (fps > HEMISPHERES_RENDERING_TARGET_FPS * 5)
                hemispheresPerJob *= 4;
            else if (fps > HEMISPHERES_RENDERING_TARGET_FPS * 3)
                hemispheresPerJob *= 2;
            else if (fps > (int32)(HEMISPHERES_RENDERING_TARGET_FPS * 1.5f))
                hemispheresPerJob = Math::RoundToInt((float)hemispheresPerJob * 1.1f);
            else if (fps < (int32)(HEMISPHERES_RENDERING_TARGET_FPS * 0.8f))
                hemispheresPerJob = Math::RoundToInt((float)hemispheresPerJob * 0.9f);
            hemispheresPerJob = Math::Clamp(hemispheresPerJob, HEMISPHERES_PER_JOB_MIN, HEMISPHERES_PER_JOB_MAX);
            if (hemispheresPerJob != _hemispheresPerJob)
            {
                LOG(Info, "Changing GI baking hemispheres count per job from {0} to {1}", _hemispheresPerJob, hemispheresPerJob);
                _hemispheresPerJob = hemispheresPerJob;
            }
        }

        // Prepare
        int32 hemispheresToRenderLeft = _hemispheresPerJob;
        int32 hemispheresToRenderBeforeSyncLeft = hemispheresToRenderLeft > 10 ? HEMISPHERES_PER_GPU_FLUSH : HEMISPHERES_PER_JOB_MAX;
        Matrix view, projection;
        Matrix::PerspectiveFov(HEMISPHERES_FOV * DegreesToRadians, 1.0f, HEMISPHERES_NEAR_PLANE, HEMISPHERES_FAR_PLANE, projection);
        ShaderData shaderData;
#if COMPILE_WITH_PROFILER
        auto gpuProfilerEnabled = ProfilerGPU::Enabled;
        ProfilerGPU::Enabled = false;
#endif

        // Render hemispheres
        for (; _workerStagePosition1 < lightmapEntry.Hemispheres.Count(); _workerStagePosition1++)
        {
            if (hemispheresToRenderLeft == 0)
                break;
            hemispheresToRenderLeft--;
            auto& hemisphere = lightmapEntry.Hemispheres[_workerStagePosition1];

            // Create tangent frame
            Vector3 tangent;
            Vector3 c1 = Vector3::Cross(hemisphere.Normal, Vector3(0.0, 0.0, 1.0));
            Vector3 c2 = Vector3::Cross(hemisphere.Normal, Vector3(0.0, 1.0, 0.0));
            tangent = c1.Length() > c2.Length() ? c1 : c2;
            tangent = Vector3::Normalize(tangent);
            const Vector3 binormal = Vector3::Cross(tangent, hemisphere.Normal);

            // Setup view
            const Vector3 pos = hemisphere.Position + hemisphere.Normal * 0.001f;
            Matrix::LookAt(pos, pos + hemisphere.Normal, tangent, view);
            _task->View.SetUp(view, projection);
            _task->View.Position = pos;
            _task->View.Direction = hemisphere.Normal;

            // Render hemisphere
            // TODO: maybe render geometry backfaces in postLightPass to set the pure black? - to remove light leaking
            IsRunningRadiancePass = true;
            EnableLightmapsUsage = _giBounceRunningIndex != 0;
            //
            Renderer::Render(_task);
            context->ClearState();
            //
            IsRunningRadiancePass = false;
            EnableLightmapsUsage = true;
            auto radianceMap = _output->View();

#if DEBUG_EXPORT_HEMISPHERES_PREVIEW
            addDebugHemisphere(context, radianceMap);
#endif

            // Setup shader data
            Matrix worldToTangent;
            worldToTangent.SetRow1(Vector4(tangent, 0.0f));
            worldToTangent.SetRow2(Vector4(binormal, 0.0f));
            worldToTangent.SetRow3(Vector4(hemisphere.Normal, 0.0f));
            worldToTangent.SetRow4(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
            worldToTangent.Invert();
            //
            Matrix viewToWorld; // viewToWorld is inverted view, since view is worldToView
            Matrix::Invert(view, viewToWorld);
            viewToWorld.SetRow4(Vector4(0.0f, 0.0f, 0.0f, 1.0f)); // reset translation row
            Matrix viewToTangent;
            Matrix::Multiply(viewToWorld, worldToTangent, viewToTangent);
            Matrix::Transpose(viewToTangent, shaderData.ToTangentSpace);
            shaderData.FinalWeight = _hemisphereTexelsTotalWeight;
            shaderData.AtlasSize = atlasSize;
            shaderData.TexelAddress = (hemisphere.TexelY * atlasSize + hemisphere.TexelX) * NUM_SH_TARGETS;

            // Calculate per pixel irradiance using compute shaders
            auto cb = _shader->GetShader()->GetCB(0);
            context->UpdateCB(cb, &shaderData);
            context->BindCB(0, cb);
            context->BindUA(0, _irradianceReduction->View());
            context->BindSR(0, radianceMap);
            context->Dispatch(_shader->GetShader()->GetCS("CS_Integrate"), 1, HEMISPHERES_RESOLUTION, 1);

            // Downscale H-basis to 1x1 and copy results to lightmap data buffer
            context->BindUA(0, lightmapEntry.LightmapData->View());
            context->FlushState();
            context->BindSR(0, _irradianceReduction->View());
            // TODO: cache shader handle
            context->Dispatch(_shader->GetShader()->GetCS("CS_Reduction"), 1, NUM_SH_TARGETS, 1);

            // Unbind slots now to make rendering backend live easier
            context->UnBindSR(0);
            context->UnBindUA(0);
            context->FlushState();

            // Keep GPU busy
            if (hemispheresToRenderBeforeSyncLeft-- < 0)
            {
                hemispheresToRenderBeforeSyncLeft = HEMISPHERES_PER_GPU_FLUSH;
                context->Flush();
            }
        }
#if COMPILE_WITH_PROFILER
        ProfilerGPU::Enabled = gpuProfilerEnabled;
#endif

        // Report progress
        float hemispheresProgress = static_cast<float>(_workerStagePosition1) / lightmapEntry.Hemispheres.Count();
        float lightmapsProgress = static_cast<float>(_workerStagePosition0 + hemispheresProgress) / scene->Lightmaps.Count();
        float bouncesProgress = static_cast<float>(_giBounceRunningIndex) / _bounceCount;
        reportProgress(BuildProgressStep::RenderHemispheres, lightmapsProgress / _bounceCount + bouncesProgress);

        // Check if work has been finished
        if (hemispheresProgress >= 1.0f)
        {
            // Move to another lightmap
            _workerStagePosition0++;
            _workerStagePosition1 = 0;

            // Check if it's stage end
            if (_workerStagePosition0 == scene->Lightmaps.Count())
            {
                _wasStageDone = true;
            }
        }
        break;
    }
    case PostprocessLightmaps:
    {
        PROFILE_GPU_CPU_NAMED("PostprocessLightmaps");

        // Let's blur generated lightmaps to reduce amount of black artifacts and holes

        // Prepare
        auto& lightmapEntry = scene->Lightmaps[_workerStagePosition0];
        ShaderData shaderData;
        shaderData.AtlasSize = atlasSize;
        auto cb = _shader->GetShader()->GetCB(0);
        context->UpdateCB(cb, &shaderData);
        context->BindCB(0, cb);

        // Blur empty lightmap texel to reduce black artifacts during sampling lightmap on objects
        context->ResetRenderTarget();
        context->BindSR(0, lightmapEntry.LightmapData->View());
        context->BindUA(0, scene->TempLightmapData->View());
        context->Dispatch(_shader->GetShader()->GetCS("CS_BlurEmpty"), atlasSize, atlasSize, 1);

        // Swap temporary buffer used as output with lightmap entry data (these buffers are the same)
        // So we can rewrite data from one buffer to another with custom sampling
        Swap(scene->TempLightmapData, lightmapEntry.LightmapData);

        // Keep blurring the empty lightmap texels (from background)
        const int32 blurPasses = 24;
        for (int32 blurPassIndex = 0; blurPassIndex < blurPasses; blurPassIndex++)
        {
            context->UnBindSR(0);
            context->UnBindUA(0);
            context->FlushState();

            context->BindSR(0, lightmapEntry.LightmapData->View());
            context->BindUA(0, scene->TempLightmapData->View());
            context->Dispatch(_shader->GetShader()->GetCS("CS_Dilate"), atlasSize, atlasSize, 1);

            Swap(scene->TempLightmapData, lightmapEntry.LightmapData);
        }
        context->UnBindSR(0);
        context->BindUA(0, lightmapEntry.LightmapData->View());

        // Remove the BACKGROUND_TEXELS_MARK from the unused texels (see shader for more info)
        context->Dispatch(_shader->GetShader()->GetCS("CS_Finalize"), atlasSize, atlasSize, 1);

        // Move to another lightmap
        _workerStagePosition0++;

        // Check if it's stage end
        if (_workerStagePosition0 >= scene->Lightmaps.Count())
        {
            _wasStageDone = true;
        }
        break;
    }
    }

    // Cleanup after rendering
    context->ClearState();

    // Mark job as done
    Platform::AtomicStore(&_wasJobDone, 1);
    _lastJobFrame = Engine::FrameCount;

    // Check if stage has been done
    if (_wasStageDone)
    {
        // Disable task
        _task->Enabled = false;
    }
}

bool ShadowsOfMordor::Builder::checkBuildCancelled()
{
    const bool wasCancelled = Platform::AtomicRead(&_wasBuildCancelled) != 0;
    if (wasCancelled)
    {
        LOG(Warning, "Lightmap building was cancelled");
    }
    return wasCancelled;
}

bool ShadowsOfMordor::Builder::runStage(BuildingStage stage, bool resetPosition)
{
    bool wasCancelled;
    _wasStageDone = false;
    if (resetPosition)
        _workerStagePosition1 = 0;
    _stage = stage;
    _lastJobFrame = 0;

    // Start the job
    RenderTask::TasksLocker.Lock();
    _task->Enabled = true;
    RenderTask::TasksLocker.Unlock();

    // Split work into more jobs to reduce overhead
    while (true)
    {
        // Wait for the end or cancellation event
        while (true)
        {
            Platform::Sleep(1);

            wasCancelled = checkBuildCancelled();
            const bool wasJobDone = Platform::AtomicRead(&_wasJobDone) != 0;
            if (wasJobDone)
                break;
        }

        // Check for stage end
        if (_wasStageDone || wasCancelled)
            break;
    }

    // Ensure to disable task
    RenderTask::TasksLocker.Lock();
    _task->Enabled = false;
    RenderTask::TasksLocker.Unlock();

    return wasCancelled;
}
