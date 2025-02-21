// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ParticleEmitter.h"
#include "ParticleSystem.h"
#include "ParticleEffect.h"
#include "Particles.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/ShaderAssetUpgrader.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Shaders/Cache/ShaderCacheManager.h"
#include "Engine/Level/Level.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "ParticleEmitterFunction.h"
#include "Engine/ShadersCompilation/Config.h"
#include "Engine/Particles/Graph/GPU/ParticleEmitterGraph.GPU.h"
#if BUILD_DEBUG
#include "Engine/Engine/Globals.h"
#include "Engine/Scripting/BinaryModule.h"
#endif
#endif
#if COMPILE_WITH_GPU_PARTICLES
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"
#endif
#if COMPILE_WITH_PARTICLE_GPU_GRAPH && COMPILE_WITH_SHADER_COMPILER
#include "Engine/Utilities/Encryption.h"
#endif

REGISTER_BINARY_ASSET_WITH_UPGRADER(ParticleEmitter, "FlaxEngine.ParticleEmitter", ShaderAssetUpgrader, false);

ParticleEmitter::ParticleEmitter(const SpawnParams& params, const AssetInfo* info)
    : ShaderAssetTypeBase<BinaryAsset>(params, info)
    , GraphExecutorCPU(Graph)
{
}

ParticleEffect* ParticleEmitter::Spawn(Actor* parent, const Transform& transform, float duration, bool autoDestroy)
{
    CHECK_RETURN(!WaitForLoaded(), nullptr);
    auto system = Content::CreateVirtualAsset<ParticleSystem>();
    CHECK_RETURN(system, nullptr);
    system->Init(this, duration < MAX_float ? duration : 3600.0f);

    auto effect = New<ParticleEffect>();
    effect->SetTransform(transform);
    effect->ParticleSystem = system;

    Level::SpawnActor(effect, parent);

    if (autoDestroy && duration < MAX_float)
        effect->DeleteObject(duration, true);

    return effect;
}

#if COMPILE_WITH_PARTICLE_GPU_GRAPH && COMPILE_WITH_SHADER_COMPILER

namespace
{
    void OnGeneratorError(ShaderGraph<>::Node* node, ShaderGraphBox* box, const StringView& text)
    {
        LOG(Error, "GPU Particles graph error: {0} (Node:{1}:{2}, Box:{3})", text, node ? node->Type : -1, node ? node->ID : -1, box ? box->ID : -1);
    }
}

#endif

Asset::LoadResult ParticleEmitter::load()
{
    ConcurrentSystemLocker::WriteScope systemScope(Particles::SystemLocker);

    // Load the graph
    const auto surfaceChunk = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
    if (!surfaceChunk)
    {
        // Initialize as empty graph with only root node (no modules)
        Graph.CreateDefault();
        return LoadResult::Ok;
    }
    if (LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
    {
        LOG(Warning, "Cannot load \'{0}\' data from chunk {1}.", ToString(), SHADER_FILE_CHUNK_VISJECT_SURFACE);
        return LoadResult::CannotLoadStorage;
    }
    MemoryReadStream surfaceChunkStream(surfaceChunk->Get(), surfaceChunk->Size());
    if (Graph.Load(&surfaceChunkStream, USE_EDITOR))
    {
        LOG(Warning, "Cannot load Particle Emitter graph '{0}'.", GetPath());
        return LoadResult::CannotLoadData;
    }

    // Cache data
    {
        auto root = Graph.Root;
        if (root->Values.Count() != 6)
        {
            Graph.Clear();
            Graph.CreateDefault();
            LOG(Warning, "Invalid Particle Emitter graph root node '{0}'.", GetPath());
        }
        Capacity = root->Values[0].AsInt;
        SimulationMode = (ParticlesSimulationMode)root->Values[1].AsInt;
        SimulationSpace = (ParticlesSimulationSpace)root->Values[2].AsInt;
        EnablePooling = root->Values[3].AsBool;
        CustomBounds = (BoundingBox)root->Values[4];
        UseAutoBounds = root->Values[5].AsBool;
        IsUsingLights = Graph.LightModules.HasItems();
    }

    // Select simulation mode
    //SimulationMode = ParticlesSimulationMode::CPU;
    //SimulationMode = ParticlesSimulationMode::GPU;
    if (SimulationMode == ParticlesSimulationMode::Default)
    {
        // Use GPU simulation only for bigger systems
        SimulationMode = Capacity >= 1024 ? ParticlesSimulationMode::GPU : ParticlesSimulationMode::CPU;
    }
    if (SimulationMode == ParticlesSimulationMode::GPU)
    {
        // Downgrade to CPU simulation if no GPU support for that
        if (IsUsingLights || Graph.RibbonRenderingModules.HasItems() || Graph.UsesVolumetricFogRendering)
        {
            // Downgrade to CPU simulation for ribbons (no GPU support for that)
            SimulationMode = ParticlesSimulationMode::CPU;
        }
    }

#if COMPILE_WITH_PARTICLE_GPU_GRAPH && COMPILE_WITH_SHADER_COMPILER
    // Check if generate shader source code
    if (SimulationMode == ParticlesSimulationMode::GPU &&
        (
            _shaderHeader.ParticleEmitter.GraphVersion != PARTICLE_GPU_GRAPH_VERSION
#if USE_EDITOR
            || !HasChunk(SHADER_FILE_CHUNK_SOURCE)
#endif
            || HasDependenciesModified()
#if COMPILE_WITH_DEV_ENV
            // Set to true to enable force GPU particle simulation shaders regeneration (don't commit it)
            || false
#endif
        ))
    {
        // Load GPU graph
        ParticleEmitterGPUGenerator generator;
        generator.Error.Bind(&OnGeneratorError);
        auto graph = New<ParticleEmitterGraphGPU>();
        surfaceChunkStream.SetPosition(0);
        if (graph->Load(&surfaceChunkStream, false))
        {
            Delete(graph);
            LOG(Warning, "Cannot load Particle Emitter GPU graph '{0}'.", GetPath());
            return LoadResult::CannotLoadData;
        }
        generator.AddGraph(graph);

        // Get chunk with material parameters
        auto materialParamsChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
        if (materialParamsChunk == nullptr)
            return LoadResult::MissingDataChunk;
        materialParamsChunk->Data.Release();

        // Generate source code and metadata
        MemoryWriteStream source(16 * 1024);
        int32 customDataSize;
        if (generator.Generate(source, materialParamsChunk->Data, customDataSize))
        {
            LOG(Error, "Cannot generate particle emitter GPU shader source code for \'{0}\'. Please see log for more information.", ToString());
            return LoadResult::Failed;
        }

        // Update asset dependencies
        ClearDependencies();
        for (auto& asset : generator.Assets)
        {
            if (asset->Is<ParticleEmitterFunction>())
                AddDependency(asset.As<BinaryAsset>());
        }

        // Setup shader header
        Platform::MemoryClear(&_shaderHeader, sizeof(_shaderHeader));
        _shaderHeader.ParticleEmitter.GraphVersion = PARTICLE_GPU_GRAPH_VERSION;
        _shaderHeader.ParticleEmitter.CustomDataSize = customDataSize;

#if BUILD_DEBUG && USE_EDITOR
        // Dump generated shader source to the temporary file
        BinaryModule::Locker.Lock();
        source.SaveToFile(Globals::ProjectCacheFolder / TEXT("particle_emitter.txt"));
        BinaryModule::Locker.Unlock();
#endif

        // Encrypt source code
        Encryption::EncryptBytes((byte*)source.GetHandle(), source.GetPosition());

        // Set new source code chunk
        SetChunk(SHADER_FILE_CHUNK_SOURCE, ToSpan(source.GetHandle(), source.GetPosition()));

        // Save to file
#if USE_EDITOR
        if (Save())
        {
            LOG(Error, "Cannot save \'{0}\'", ToString());
            return LoadResult::Failed;
        }
#endif
#if COMPILE_WITH_SHADER_CACHE_MANAGER
        // Invalidate shader cache
        ShaderCacheManager::RemoveCache(GetID());
#endif
    }
#endif

#if USE_EDITOR
    // Find asset dependencies to particle emitter functions (GPU shader generation collects dependencies so don't override them)
    if (SimulationMode != ParticlesSimulationMode::GPU)
    {
        ClearDependencies();
        for (const auto& node : Graph.Nodes)
        {
            if (node.Type == GRAPH_NODE_MAKE_TYPE(14, 300) && node.Assets.Count() > 0)
            {
                const auto function = node.Assets[0].As<ParticleEmitterFunction>();
                if (function)
                {
                    AddDependency(function);
                }
            }
        }
    }
#endif

#if COMPILE_WITH_GPU_PARTICLES
    // Handle the current runtime support for simulation mode
    if (SimulationMode == ParticlesSimulationMode::GPU && (!GPUDevice::Instance->Limits.HasCompute || !GPUDevice::Instance->Limits.HasDrawIndirect))
    {
        // Downgrade to CPU simulation if GPU doesn't support compute shaders
        SimulationMode = ParticlesSimulationMode::CPU;
    }

    // Check if use GPU simulation
    if (SimulationMode == ParticlesSimulationMode::GPU)
    {
        // Load shader cache (it may call compilation or gather cached data)
        ShaderCacheResult shaderCache;
        if (LoadShaderCache(shaderCache))
        {
            LOG(Error, "Cannot load \'{0}\' shader cache.", ToString());
            return LoadResult::Failed;
        }
        MemoryReadStream shaderCacheStream(shaderCache.Data.Get(), shaderCache.Data.Length());

        // Get material parameters (may be not created if material is not using parameters)
        const auto materialParamsChunk = GetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
        const byte* materialParamsData = nullptr;
        uint32 materialParamsDataSize = 0;
        if (materialParamsChunk != nullptr)
        {
            if (LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS)))
            {
                LOG(Warning, "Cannot load \'{0}\' data from chunk {1}.", ToString(), SHADER_FILE_CHUNK_MATERIAL_PARAMS);
                return LoadResult::CannotLoadStorage;
            }
            materialParamsData = materialParamsChunk->Get();
            materialParamsDataSize = materialParamsChunk->Size();
        }
        MemoryReadStream materialParamsStream(materialParamsData, materialParamsDataSize);

        // Setup GPU execution pipeline
        if (GPU.Init(this, shaderCacheStream, &materialParamsStream, _shaderHeader.ParticleEmitter.CustomDataSize))
        {
            LOG(Error, "Cannot init \'{0}\' GPU execution runtime.", ToString());
            return LoadResult::Failed;
        }
#if COMPILE_WITH_SHADER_COMPILER
        RegisterForShaderReloads(this, shaderCache);
#endif
    }
#else
	// Downgrade to CPU simulation if no support in build
	SimulationMode = ParticlesSimulationMode::CPU;
#endif

    return LoadResult::Ok;
}

void ParticleEmitter::unload(bool isReloading)
{
    ConcurrentSystemLocker::WriteScope systemScope(Particles::SystemLocker);
#if COMPILE_WITH_SHADER_COMPILER
    UnregisterForShaderReloads(this);
#endif

    Particles::OnEmitterUnload(this);

    Graph.Clear();
#if COMPILE_WITH_GPU_PARTICLES
    GPU.Dispose();
#endif
}

AssetChunksFlag ParticleEmitter::getChunksToPreload() const
{
    AssetChunksFlag result = ShaderAssetTypeBase<BinaryAsset>::getChunksToPreload();
    result |= GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE);
#if COMPILE_WITH_GPU_PARTICLES
    result |= GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
#endif
    return result;
}

#if USE_EDITOR

void ParticleEmitter::OnDependencyModified(BinaryAsset* asset)
{
    BinaryAsset::OnDependencyModified(asset);

    Reload();
}

#endif

#if USE_EDITOR

void ParticleEmitter::InitCompilationOptions(ShaderCompilationOptions& options)
{
    // Base
    ShaderAssetBase::InitCompilationOptions(options);

#if COMPILE_WITH_SHADER_COMPILER
    // Setup shader macros
    options.Macros.Add({ "THREAD_GROUP_SIZE", "1024" });
#endif
}

#endif

BytesContainer ParticleEmitter::LoadSurface(bool createDefaultIfMissing)
{
    BytesContainer result;
    if (WaitForLoaded() && !LastLoadFailed())
        return result;
    ScopeLock lock(Locker);

    // Check if has that chunk
    if (HasChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
    {
        // Load graph
        if (!LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
        {
            // Get stream with graph data
            const auto data = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
            result.Copy(data->Data);
            return result;
        }
    }

    LOG(Warning, "Particle Emitter \'{0}\' surface data is missing.", GetPath());

    // Check if create default surface
    if (createDefaultIfMissing)
    {
        // Create default layer
        ParticleEmitterGraphCPU graph;
        graph.CreateDefault();

        // Serialize layer to stream
        MemoryWriteStream stream(512);
        graph.Save(&stream, false);

        // Set output data
        result.Copy(stream.GetHandle(), stream.GetPosition());
    }

    return result;
}

#if USE_EDITOR

bool ParticleEmitter::SaveSurface(BytesContainer& data)
{
    // Wait for asset to be loaded or don't if last load failed (eg. by shader source compilation error)
    if (LastLoadFailed())
    {
        LOG(Warning, "Saving asset that failed to load.");
    }
    else if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }
    ConcurrentSystemLocker::WriteScope systemScope(Particles::SystemLocker);
    ScopeLock lock(Locker);

    // Release all chunks
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        ReleaseChunk(i);

    // Clear particle emitter info
    Platform::MemoryClear(&_shaderHeader, sizeof(_shaderHeader));
    _shaderHeader.ParticleEmitter.GraphVersion = PARTICLE_GPU_GRAPH_VERSION;
    _shaderHeader.ParticleEmitter.CustomDataSize = 0;

    // Set Visject Surface data
    auto visjectSurfaceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
    ASSERT(visjectSurfaceChunk != nullptr);
    visjectSurfaceChunk->Data.Copy(data);

    if (Save())
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

#if COMPILE_WITH_SHADER_CACHE_MANAGER
    // Invalidate shader cache
    ShaderCacheManager::RemoveCache(GetID());
#endif

    return false;
}

#endif
