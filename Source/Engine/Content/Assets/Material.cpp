// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Material.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Upgraders/ShaderAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Materials/MaterialShader.h"
#include "Engine/Graphics/Shaders/Cache/ShaderCacheManager.h"
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"
#include "Engine/Serialization/MemoryReadStream.h"
#if COMPILE_WITH_SHADER_COMPILER
#include "MaterialFunction.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Tools/MaterialGenerator/MaterialGenerator.h"
#include "Engine/ShadersCompilation/Config.h"
#endif

/// <summary>
/// Enable/disable automatic material shader source code generation (if missing)
/// </summary>
#define MATERIAL_AUTO_GENERATE_MISSING_SOURCE (USE_EDITOR)

REGISTER_BINARY_ASSET(Material, "FlaxEngine.Material", ::New<ShaderAssetUpgrader>(), false);

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

    // Special case for Null renderer
    if (GPUDevice::Instance->GetRendererType() == RendererType::Null)
    {
        // Hack loading
        MemoryReadStream shaderCacheStream(nullptr, 0);
        _materialShader = MaterialShader::CreateDummy(shaderCacheStream, _shaderHeader.Material.Info);
        if (_materialShader == nullptr)
        {
            LOG(Warning, "Cannot load material.");
            return LoadResult::Failed;
        }
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
        return LoadResult::Ok;
    }

    // If engine was compiled with shaders compiling service:
    // - Material should be changed in need to convert it to the newer version (via Visject Surface)
    //   Shader should be recompiled if shader source code has been modified
    // otherwise:
    // - If material version is not supported then material cannot be loaded
#if COMPILE_WITH_SHADER_COMPILER

#if BUILD_DEBUG
    // Materials force reload!
    Globals::ConvertLoadedMaterialsByForce = false;
#endif

    // Check if current engine has different materials version or convert it by force or has no source generated at all
    if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION
        || Globals::ConvertLoadedMaterialsByForce
#if MATERIAL_AUTO_GENERATE_MISSING_SOURCE
        || !HasChunk(SHADER_FILE_CHUNK_SOURCE)
#endif
        || HasDependenciesModified()
    )
    {
        // Prepare
        MaterialGenerator generator;
        generator.Error.Bind(&OnGeneratorError);
        if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION)
            LOG(Info, "Converting material \'{0}\', from version {1} to {2}...", ToString(), _shaderHeader.Material.GraphVersion, MATERIAL_GRAPH_VERSION);
        else
            LOG(Info, "Updating material \'{0}\'...", ToString());

        // Load or create material surface
        MaterialLayer* layer;
        if (HasChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
        {
            // Load graph
            if (LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
            {
                LOG(Warning, "Cannot load \'{0}\' data from chunk {1}.", ToString(), SHADER_FILE_CHUNK_VISJECT_SURFACE);
                return LoadResult::Failed;
            }

            // Get stream with graph data
            auto surfaceChunk = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
            MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());

            // Load layer
            layer = MaterialLayer::Load(GetID(), &stream, _shaderHeader.Material.Info, ToString());
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
            surfaceChunk->Data.Copy(stream.GetHandle(), stream.GetPosition());
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
            LOG(Error, "Cannot generate material source code for \'{0}\'. Please see log for more info.", ToString());
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
        source.SaveToFile(Globals::ProjectCacheFolder / TEXT("material.txt"));
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
    if (LoadShaderCache(shaderCache))
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
        _materialShader = MaterialShader::Create(GetPath(), shaderCacheStream, _shaderHeader.Material.Info);
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
            (info.UsageFlags & MaterialUsageFlags::UseRefraction) != 0 &&
            (info.FeaturesFlags & MaterialFeaturesFlags::DisableDistortion) == 0;

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
    options.Macros.Add({ "MATERIAL_MASKED", Numbers[info.UsageFlags & MaterialUsageFlags::UseMask ? 1 : 0] });
    options.Macros.Add({ "DECAL_BLEND_MODE", Numbers[(int32)info.DecalBlendingMode] });
    options.Macros.Add({ "USE_EMISSIVE", Numbers[info.UsageFlags & MaterialUsageFlags::UseEmissive ? 1 : 0] });
    options.Macros.Add({ "USE_NORMAL", Numbers[info.UsageFlags & MaterialUsageFlags::UseNormal ? 1 : 0] });
    options.Macros.Add({ "USE_POSITION_OFFSET", Numbers[info.UsageFlags & MaterialUsageFlags::UsePositionOffset ? 1 : 0] });
    options.Macros.Add({ "USE_VERTEX_COLOR", Numbers[info.UsageFlags & MaterialUsageFlags::UseVertexColor ? 1 : 0] });
    options.Macros.Add({ "USE_DISPLACEMENT", Numbers[info.UsageFlags & MaterialUsageFlags::UseDisplacement ? 1 : 0] });
    options.Macros.Add({ "USE_DITHERED_LOD_TRANSITION", Numbers[info.FeaturesFlags & MaterialFeaturesFlags::DitheredLODTransition ? 1 : 0] });
    options.Macros.Add({ "USE_GBUFFER_CUSTOM_DATA", Numbers[useCustomData ? 1 : 0] });
    options.Macros.Add({ "USE_REFLECTIONS", Numbers[info.FeaturesFlags & MaterialFeaturesFlags::DisableReflections ? 0 : 1] });
    options.Macros.Add({ "USE_FOG", Numbers[info.FeaturesFlags & MaterialFeaturesFlags::DisableFog ? 0 : 1] });
    if (useForward)
        options.Macros.Add({ "USE_PIXEL_NORMAL_OFFSET_REFRACTION", Numbers[info.FeaturesFlags & MaterialFeaturesFlags::PixelNormalOffsetRefraction ? 1 : 0] });

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
        options.Macros.Add({ "MAX_TESSELLATION_FACTOR", Numbers[info.MaxTessellationFactor] });
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

#endif

BytesContainer Material::LoadSurface(bool createDefaultIfMissing)
{
    BytesContainer result;
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

    LOG(Warning, "Material \'{0}\' surface data is missing.", GetPath());

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
        result.Copy(stream.GetHandle(), stream.GetPosition());
        return result;
    }

#endif

    return result;
}

#if USE_EDITOR

bool Material::SaveSurface(BytesContainer& data, const MaterialInfo& info)
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
