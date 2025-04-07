// Copyright (c) Wojciech Figat. All rights reserved.

#include "Material.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Upgraders/ShaderAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Materials/MaterialShader.h"
#include "Engine/Graphics/Shaders/Cache/ShaderCacheManager.h"
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Threading.h"
#if COMPILE_WITH_SHADER_COMPILER
#include "MaterialFunction.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Tools/MaterialGenerator/MaterialGenerator.h"
#include "Engine/ShadersCompilation/Config.h"
#if BUILD_DEBUG
#include "Engine/Engine/Globals.h"
#include "Engine/Scripting/BinaryModule.h"
#endif
#endif

/// <summary>
/// Enable/disable automatic material shader source code generation (if missing)
/// </summary>
#define MATERIAL_AUTO_GENERATE_MISSING_SOURCE (USE_EDITOR)

REGISTER_BINARY_ASSET_WITH_UPGRADER(Material, "FlaxEngine.Material", ShaderAssetUpgrader, false);

Material::Material(const SpawnParams& params, const AssetInfo* info)
    : ShaderAssetTypeBase<MaterialBase>(params, info)
{
}

bool Material::IsMaterialInstance() const
{
    return false;
}

const MaterialInfo& Material::GetInfo() const
{
    if (_materialShader)
        return _materialShader->GetInfo();

    static MaterialInfo EmptyInfo;
    return EmptyInfo;
}

GPUShader* Material::GetShader() const
{
    return _materialShader ? _materialShader->GetShader() : nullptr;
}

bool Material::IsReady() const
{
    return _materialShader && _materialShader->IsReady();
}

DrawPass Material::GetDrawModes() const
{
    if (_materialShader)
        return _materialShader->GetDrawModes();
    return DrawPass::None;
}

bool Material::CanUseLightmap() const
{
    return _materialShader && _materialShader->CanUseLightmap();
}

bool Material::CanUseInstancing(InstancingHandler& handler) const
{
    return _materialShader && _materialShader->CanUseInstancing(handler);
}

void Material::Bind(BindParameters& params)
{
    ASSERT(IsReady());

    MaterialParamsLink* lastLink = params.ParamsLink;
    MaterialParamsLink link;
    link.This = &Params;
    if (lastLink)
    {
        while (lastLink->Down)
            lastLink = lastLink->Down;
        lastLink->Down = &link;
    }
    else
    {
        params.ParamsLink = &link;
    }
    link.Up = lastLink;
    link.Down = nullptr;

    _materialShader->Bind(params);

    if (lastLink)
    {
        lastLink->Down = nullptr;
    }
    else
    {
        params.ParamsLink = nullptr;
    }
}

#if COMPILE_WITH_MATERIAL_GRAPH && COMPILE_WITH_SHADER_COMPILER

namespace
{
    void OnGeneratorError(ShaderGraph<>::Node* node, ShaderGraphBox* box, const StringView& text)
    {
        LOG(Error, "Material error: {0} (Node:{1}:{2}, Box:{3})", text, node ? node->Type : -1, node ? node->ID : -1, box ? box->ID : -1);
    }
}

#endif

Asset::LoadResult Material::load()
{
    ASSERT(_materialShader == nullptr);
    FlaxChunk* materialParamsChunk;

    // Wait for the GPU Device to be ready (eg. case when loading material before GPU init)
#define IS_GPU_NOT_READY() (GPUDevice::Instance == nullptr || GPUDevice::Instance->GetState() != GPUDevice::DeviceState::Ready)
    if (!IsInMainThread() && IS_GPU_NOT_READY())
    {
        int32 timeout = 1000;
        while (IS_GPU_NOT_READY() && timeout-- > 0)
            Platform::Sleep(1);
        if (IS_GPU_NOT_READY())
            return LoadResult::InvalidData;
    }
#undef IS_GPU_NOT_READY

    // If engine was compiled with shaders compiling service:
    // - Material should be changed in need to convert it to the newer version (via Visject Surface)
    //   Shader should be recompiled if shader source code has been modified
    // otherwise:
    // - If material version is not supported then material cannot be loaded
#if COMPILE_WITH_SHADER_COMPILER

    // Check if current engine has different materials version or convert it by force or has no source generated at all
    if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION
#if MATERIAL_AUTO_GENERATE_MISSING_SOURCE
        || !HasChunk(SHADER_FILE_CHUNK_SOURCE)
#endif
        || HasDependenciesModified()
#if COMPILE_WITH_DEV_ENV
        // Set to true to enable force GPU shader regeneration (don't commit it)
        || false
#endif
    )
    {
        // Guard file with the lock during shader generation (prevents FlaxStorage::Tick from messing with the file)
        auto lock = Storage->Lock();

        // Prepare
        const String name = ToString();
        MaterialGenerator generator;
        generator.Error.Bind(&OnGeneratorError);
        if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION)
            LOG(Info, "Converting material \'{0}\', from version {1} to {2}...", name, _shaderHeader.Material.GraphVersion, MATERIAL_GRAPH_VERSION);
        else
            LOG(Info, "Updating material \'{0}\'...", name);

        // Load or create material surface
        MaterialLayer* layer;
        if (HasChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
        {
            // Load graph
            if (LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
            {
                LOG(Warning, "Cannot load \'{0}\' data from chunk {1}.", name, SHADER_FILE_CHUNK_VISJECT_SURFACE);
                return LoadResult::Failed;
            }

            // Get stream with graph data
            auto surfaceChunk = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
            MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());

            // Load layer
            layer = MaterialLayer::Load(GetID(), &stream, _shaderHeader.Material.Info, name);
            if (ContentDeprecated::Clear())
            {
                // If encountered any deprecated data when loading graph then serialize it
                MaterialGraph graph;
                MemoryWriteStream writeStream(1024);
                stream.SetPosition(0);
                if (!graph.Load(&stream, true) && !graph.Save(&writeStream, true))
                {
                    surfaceChunk->Data.Copy(ToSpan(writeStream));
                    ContentDeprecated::Clear();
                }
            }
        }
        else
        {
            // Create default layer
            layer = MaterialLayer::CreateDefault(GetID());

            // Create surface chunk
            auto surfaceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
            if (surfaceChunk == nullptr)
                return LoadResult::MissingDataChunk;

            // Save layer to the chunk data
            MemoryWriteStream stream(512);
            layer->Graph.Save(&stream, false);
            surfaceChunk->Data.Copy(ToSpan(stream));
        }
        generator.AddLayer(layer);

        // Get chunk with material parameters
        materialParamsChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
        if (materialParamsChunk == nullptr)
            return LoadResult::MissingDataChunk;
        materialParamsChunk->Data.Release();

        // Generate material source code and metadata
        MemoryWriteStream newMaterialMeta(1024);
        MemoryWriteStream source(64 * 1024);
        MaterialInfo info = _shaderHeader.Material.Info;
        if (generator.Generate(source, info, materialParamsChunk->Data))
        {
            LOG(Error, "Cannot generate material source code for \'{0}\'. Please see log for more info.", name);
            return LoadResult::Failed;
        }

        // Update asset dependencies
        ClearDependencies();
        for (auto& asset : generator.Assets)
        {
            if (asset->Is<MaterialBase>() ||
                asset->Is<MaterialFunction>())
                AddDependency(asset.As<BinaryAsset>());
        }

#if BUILD_DEBUG && USE_EDITOR
        // Dump generated material source to the temporary file
        BinaryModule::Locker.Lock();
        source.SaveToFile(Globals::ProjectCacheFolder / TEXT("material.txt"));
        BinaryModule::Locker.Unlock();
#endif

        // Encrypt source code
        Encryption::EncryptBytes((byte*)source.GetHandle(), source.GetPosition());

        // Set new source code chunk
        SetChunk(SHADER_FILE_CHUNK_SOURCE, ToSpan(source.GetHandle(), source.GetPosition()));

        // Clear shader cache
        for (int32 chunkIndex = 1; chunkIndex < 14; chunkIndex++)
            ReleaseChunk(chunkIndex);

        // Setup shader header
        Platform::MemoryClear(&_shaderHeader, sizeof(_shaderHeader));
        _shaderHeader.Material.GraphVersion = MATERIAL_GRAPH_VERSION;
        _shaderHeader.Material.Info = info;

        // Save to file
#if USE_EDITOR
        if (SaveShaderAsset())
        {
            LOG(Error, "Cannot save \'{0}\'", name);
            return LoadResult::Failed;
        }
#endif
#if COMPILE_WITH_SHADER_CACHE_MANAGER
        // Invalidate shader cache
        ShaderCacheManager::RemoveCache(GetID());
#endif
    }

#else

	// Ensure that material is in the current version (whole materials pipeline depends on that)
	if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION)
	{
		LOG(Fatal, "Unsupported material version: {0} in material \'{1}\'. Current is {2}.", _shaderHeader.Material.GraphVersion, ToString(), MATERIAL_GRAPH_VERSION);
		return LoadResult::Failed;
	}

#endif

    // Load shader cache (it may call compilation or gather cached data)
    ShaderCacheResult shaderCache;
    if (IsNullRenderer())
    {
        MemoryReadStream shaderCacheStream(nullptr, 0);
        _materialShader = MaterialShader::CreateDummy(shaderCacheStream, _shaderHeader.Material.Info);
    }
    else if (LoadShaderCache(shaderCache))
    {
        LOG(Error, "Cannot load \'{0}\' shader cache.", ToString());
#if 1
        return LoadResult::Failed;
#else
        // Custom path: don't fail asset loading but use dummy material instead (surface and parameters will be available for editing)
        LOG(Error, "Using dummy material shader for \'{0}\'.", ToString());
        MemoryWriteStream dummyShader(64);
        {
            dummyShader.WriteInt32(6);
            dummyShader.WriteInt32(0);
            dummyShader.WriteInt32(0);
            dummyShader.WriteByte(0);
            dummyShader.WriteByte(0);
        }
        MemoryReadStream shaderCacheStream(dummyShader.GetHandle(), dummyShader.GetPosition());
        _materialShader = MaterialShader::CreateDummy(shaderCacheStream, _shaderHeader.Material.Info);
        if (_materialShader == nullptr)
        {
            LOG(Warning, "Cannot load material.");
            return LoadResult::Failed;
        }
#endif
    }
    else
    {
        // Load material (load shader from cache, load params, setup pipeline stuff)
        MemoryReadStream shaderCacheStream(shaderCache.Data.Get(), shaderCache.Data.Length());
#if GPU_ENABLE_RESOURCE_NAMING
        const StringView name(GetPath());
#else
        const StringView name;
#endif
        _materialShader = MaterialShader::Create(name, shaderCacheStream, _shaderHeader.Material.Info);
        if (_materialShader == nullptr)
        {
            LOG(Warning, "Cannot load material.");
            return LoadResult::Failed;
        }
    }

    // Load material parameters
    materialParamsChunk = GetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
    if (materialParamsChunk != nullptr && materialParamsChunk->IsLoaded())
    {
        MemoryReadStream materialParamsStream(materialParamsChunk->Get(), materialParamsChunk->Size());
        if (Params.Load(&materialParamsStream))
        {
            LOG(Warning, "Cannot load material parameters.");
            return LoadResult::Failed;
        }
    }
    else
    {
        // Don't use parameters
        Params.Dispose();
    }
    ParamsChanged();

#if COMPILE_WITH_SHADER_COMPILER
    RegisterForShaderReloads(this, shaderCache);
#endif

    return LoadResult::Ok;
}

void Material::unload(bool isReloading)
{
#if COMPILE_WITH_SHADER_COMPILER
    UnregisterForShaderReloads(this);
#endif

    if (_materialShader)
    {
        _materialShader->Unload();
        Delete(_materialShader);
        _materialShader = nullptr;
    }

    Params.Dispose();
}

AssetChunksFlag Material::getChunksToPreload() const
{
    AssetChunksFlag result = ShaderAssetTypeBase<MaterialBase>::getChunksToPreload();
    result |= GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
    return result;
}

#if USE_EDITOR

void Material::OnDependencyModified(BinaryAsset* asset)
{
    BinaryAsset::OnDependencyModified(asset);

    Reload();
}

#endif

#if USE_EDITOR

void Material::InitCompilationOptions(ShaderCompilationOptions& options)
{
    // Base
    ShaderAssetBase::InitCompilationOptions(options);

#if COMPILE_WITH_SHADER_COMPILER
    // Ensure that this call is valid (material features switches may depend on target compilation platform)
    ASSERT(options.Profile != ShaderProfile::Unknown);

    // Prepare
    auto& info = _shaderHeader.Material.Info;
    const bool isSurfaceOrTerrainOrDeformable = info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Terrain || info.Domain == MaterialDomain::Deformable;
    const bool useCustomData = info.ShadingModel == MaterialShadingModel::Subsurface || info.ShadingModel == MaterialShadingModel::Foliage;
    const bool useForward = ((info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Deformable) && info.BlendMode != MaterialBlendMode::Opaque) || info.Domain == MaterialDomain::Particle;
    const bool useTess =
            info.TessellationMode != TessellationMethod::None &&
            RenderTools::CanSupportTessellation(options.Profile) && isSurfaceOrTerrainOrDeformable;
    const bool useDistortion =
            (info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Deformable || info.Domain == MaterialDomain::Particle) &&
            info.BlendMode != MaterialBlendMode::Opaque &&
            EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UseRefraction) &&
            (info.FeaturesFlags & MaterialFeaturesFlags::DisableDistortion) == MaterialFeaturesFlags::None;

    // @formatter:off
    static const char* Numbers[] =
    {
        "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31","32","33","34","35","36","37","38","39","40","41","42","43","44","45","46","47","48","49","50","51","52","53","54","55","56","57","58","59","60","61","62","63","64","65","66","67","68","69",
    };
    // @formatter:on

    // Setup shader macros
    options.Macros.Add({ "MATERIAL_DOMAIN", Numbers[(int32)info.Domain] });
    options.Macros.Add({ "MATERIAL_BLEND", Numbers[(int32)info.BlendMode] });
    options.Macros.Add({ "MATERIAL_SHADING_MODEL", Numbers[(int32)info.ShadingModel] });
    options.Macros.Add({ "MATERIAL_MASKED", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UseMask) ? 1 : 0] });
    options.Macros.Add({ "DECAL_BLEND_MODE", Numbers[(int32)info.DecalBlendingMode] });
    options.Macros.Add({ "USE_EMISSIVE", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UseEmissive) ? 1 : 0] });
    options.Macros.Add({ "USE_NORMAL", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UseNormal) ? 1 : 0] });
    options.Macros.Add({ "USE_POSITION_OFFSET", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UsePositionOffset) ? 1 : 0] });
    options.Macros.Add({ "USE_VERTEX_COLOR", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UseVertexColor) ? 1 : 0] });
    options.Macros.Add({ "USE_DISPLACEMENT", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsageFlags::UseDisplacement) ? 1 : 0] });
    options.Macros.Add({ "USE_DITHERED_LOD_TRANSITION", Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeaturesFlags::DitheredLODTransition) ? 1 : 0] });
    options.Macros.Add({ "USE_GBUFFER_CUSTOM_DATA", Numbers[useCustomData ? 1 : 0] });
    options.Macros.Add({ "USE_REFLECTIONS", Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeaturesFlags::DisableReflections) ? 0 : 1] });
    if (!(info.FeaturesFlags & MaterialFeaturesFlags::DisableReflections) && EnumHasAnyFlags(info.FeaturesFlags, MaterialFeaturesFlags::ScreenSpaceReflections))
        options.Macros.Add({ "MATERIAL_REFLECTIONS", Numbers[1] });
    options.Macros.Add({ "USE_FOG", Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeaturesFlags::DisableFog) ? 0 : 1] });
    if (useForward)
    {
        options.Macros.Add({ "USE_PIXEL_NORMAL_OFFSET_REFRACTION", Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeaturesFlags::PixelNormalOffsetRefraction) ? 1 : 0] });
        switch (info.TransparentLightingMode)
        {
        case MaterialTransparentLightingMode::Surface:
            break;
        case MaterialTransparentLightingMode::SurfaceNonDirectional:
            options.Macros.Add({ "LIGHTING_NO_DIRECTIONAL", "1" });
            break;
        }
    }

    // TODO: don't compile VS_Depth for deferred/forward materials if material doesn't use position offset or masking

    options.Macros.Add({ "USE_TESSELLATION", Numbers[useTess ? 1 : 0] });
    options.Macros.Add({ "TESSELLATION_IN_CONTROL_POINTS", "3" });
    if (useTess)
    {
        switch (info.TessellationMode)
        {
        case TessellationMethod::Flat:
            options.Macros.Add({ "MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_FLAT" });
            break;
        case TessellationMethod::PointNormal:
            options.Macros.Add({ "MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_PN" });
            break;
        case TessellationMethod::Phong:
            options.Macros.Add({ "MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_PHONG" });
            break;
        }
        options.Macros.Add({ "MAX_TESSELLATION_FACTOR", Numbers[Math::Min<int32>(info.MaxTessellationFactor, ARRAY_COUNT(Numbers) - 1)] });
    }

    // Helper macros (used by the parser)
    options.Macros.Add({ "IS_SURFACE", Numbers[info.Domain == MaterialDomain::Surface ? 1 : 0] });
    options.Macros.Add({ "IS_POST_FX", Numbers[info.Domain == MaterialDomain::PostProcess ? 1 : 0] });
    options.Macros.Add({ "IS_GUI", Numbers[info.Domain == MaterialDomain::GUI ? 1 : 0] });
    options.Macros.Add({ "IS_DECAL", Numbers[info.Domain == MaterialDomain::Decal ? 1 : 0] });
    options.Macros.Add({ "IS_TERRAIN", Numbers[info.Domain == MaterialDomain::Terrain ? 1 : 0] });
    options.Macros.Add({ "IS_PARTICLE", Numbers[info.Domain == MaterialDomain::Particle ? 1 : 0] });
    options.Macros.Add({ "IS_DEFORMABLE", Numbers[info.Domain == MaterialDomain::Deformable ? 1 : 0] });
    options.Macros.Add({ "USE_FORWARD", Numbers[useForward ? 1 : 0] });
    options.Macros.Add({ "USE_DEFERRED", Numbers[isSurfaceOrTerrainOrDeformable && info.BlendMode == MaterialBlendMode::Opaque ? 1 : 0] });
    options.Macros.Add({ "USE_DISTORTION", Numbers[useDistortion ? 1 : 0] });
#endif
}

bool Material::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);
    BytesContainer existingData = LoadSurface(true);
    if (existingData.IsInvalid())
        return true;
    MaterialGraph graph;
    MemoryWriteStream writeStream(existingData.Length());
    MemoryReadStream readStream(existingData);
    if (graph.Load(&readStream, true) || graph.Save(&writeStream, true))
        return true;
    BytesContainer data;
    data.Link(ToSpan(writeStream));
    auto materialInfo = _shaderHeader.Material.Info;
    return SaveSurface(data, materialInfo);
}

#endif

BytesContainer Material::LoadSurface(bool createDefaultIfMissing)
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

    LOG(Warning, "Material \'{0}\' surface data is missing.", ToString());

#if COMPILE_WITH_MATERIAL_GRAPH

    // Check if create default surface
    if (createDefaultIfMissing)
    {
        // Create default layer
        const auto layer = MaterialLayer::CreateDefault(GetID());

        // Serialize layer to stream
        MemoryWriteStream stream(256);
        layer->Graph.Save(&stream, false);

        // Set output data
        result.Copy(ToSpan(stream));
        return result;
    }

#endif

    return result;
}

#if USE_EDITOR

bool Material::SaveSurface(const BytesContainer& data, const MaterialInfo& info)
{
    if (OnCheckSave())
        return true;
    ScopeLock lock(Locker);

    // Release all chunks
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        ReleaseChunk(i);

    // Update material info
    Platform::MemoryClear(&_shaderHeader, sizeof(_shaderHeader));
    _shaderHeader.Material.GraphVersion = MATERIAL_GRAPH_VERSION;
    _shaderHeader.Material.Info = info;

    // Set Visject Surface data
    auto visjectSurfaceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
    ASSERT(visjectSurfaceChunk != nullptr);
    visjectSurfaceChunk->Data.Copy(data);

    if (SaveShaderAsset())
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
