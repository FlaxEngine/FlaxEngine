// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "ShaderAssetBase.h"
#include "ShaderStorage.h"
#include "ShaderCacheManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#if COMPILE_WITH_SHADER_COMPILER
#include "Engine/Engine/CommandLine.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Serialization/MemoryReadStream.h"
#endif
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/ShadowsOfMordor/AtlasChartsPacker.h"

ShaderStorage::CachingMode ShaderStorage::CurrentCachingMode =
#if USE_EDITOR
        CachingMode::ProjectCache;
#else
        CachingMode::AssetInternal;
#endif
const int32 ShaderStorage::MagicCode = -842185133;

ShaderStorage::CachingMode ShaderStorage::GetCachingMode()
{
#if !COMPILE_WITH_SHADER_CACHE_MANAGER
    if (CurrentCachingMode == CachingMode::ProjectCache)
        return CachingMode::AssetInternal;
#endif
    return CurrentCachingMode;
}

#if COMPILE_WITH_SHADER_COMPILER

#include "Engine/Utilities/Encryption.h"
#include "Engine/ShadersCompilation/ShadersCompilation.h"

#endif

bool ShaderAssetBase::IsNullRenderer()
{
    return GPUDevice::Instance->GetRendererType() == RendererType::Null;
}

int32 ShaderAssetBase::GetCacheChunkIndex()
{
    return GetCacheChunkIndex(GPUDevice::Instance->GetShaderProfile());
}

int32 ShaderAssetBase::GetCacheChunkIndex(ShaderProfile profile)
{
    int32 result;
    switch (profile)
    {
    case ShaderProfile::DirectX_SM6:
        result = SHADER_FILE_CHUNK_INTERNAL_D3D_SM6_CACHE;
        break;
    case ShaderProfile::DirectX_SM5:
        result = SHADER_FILE_CHUNK_INTERNAL_D3D_SM5_CACHE;
        break;
    case ShaderProfile::DirectX_SM4:
        result = SHADER_FILE_CHUNK_INTERNAL_D3D_SM4_CACHE;
        break;
    case ShaderProfile::GLSL_410:
        result = SHADER_FILE_CHUNK_INTERNAL_GLSL_410_CACHE;
        break;
    case ShaderProfile::GLSL_440:
        result = SHADER_FILE_CHUNK_INTERNAL_GLSL_440_CACHE;
        break;
    case ShaderProfile::Vulkan_SM5:
        result = SHADER_FILE_CHUNK_INTERNAL_VULKAN_SM5_CACHE;
        break;
    default:
        result = SHADER_FILE_CHUNK_INTERNAL_GENERIC_CACHE;
        break;
    }
    return result;
}

bool ShaderAssetBase::initBase(AssetInitData& initData)
{
    // Validate
    if (initData.SerializedVersion != ShaderStorage::Header::Version)
    {
        LOG(Warning, "Invalid shader serialized version.");
        return true;
    }
    if (initData.CustomData.Length() != sizeof(_shaderHeader))
    {
        LOG(Warning, "Invalid shader header.");
        return true;
    }

    // Load header 'as-is'
    Platform::MemoryCopy(&_shaderHeader, initData.CustomData.Get(), sizeof(_shaderHeader));

    return false;
}

#if USE_EDITOR

bool ShaderAssetBase::SaveShaderAsset() const
{
    // Asset is being saved so no longer need to resave deprecated data in it
    ContentDeprecated::Clear();

    auto parent = GetShaderAsset();
    AssetInitData data;
    data.SerializedVersion = ShaderStorage::Header::Version;
    data.CustomData.Link(&_shaderHeader);
    parent->Metadata.Release();
    return parent->SaveAsset(data);
}

#endif

#if COMPILE_WITH_SHADER_COMPILER

bool IsValidShaderCache(DataContainer<byte>& shaderCache, Array<String>& includes)
{
    if (shaderCache.Length() == 0)
        return false;
    MemoryReadStream stream(shaderCache.Get(), shaderCache.Length());

    // Read cache format version
    int32 version;
    stream.ReadInt32(&version);
    if (version != GPU_SHADER_CACHE_VERSION)
        return false;

    // Read the location of additional data that contains list of included source files
    int32 additionalDataStart;
    stream.ReadInt32(&additionalDataStart);
    stream.SetPosition(additionalDataStart);

    // Read all includes
    int32 includesCount;
    stream.ReadInt32(&includesCount);
    includes.Clear();
    for (int32 i = 0; i < includesCount; i++)
    {
        String& include = includes.AddOne();
        stream.Read(include, 11);
        include  = ShadersCompilation::ResolveShaderPath(include);
        DateTime lastEditTime;
        stream.Read(lastEditTime);

        // Check if included file exists locally and has been modified since last compilation
        if (FileSystem::FileExists(include) && FileSystem::GetFileLastEditTime(include) > lastEditTime)
            return false;
    }

    return true;
}

#endif

bool ShaderAssetBase::LoadShaderCache(ShaderCacheResult& result)
{
    PROFILE_CPU();
    auto parent = GetShaderAsset();
    const ShaderProfile shaderProfile = GPUDevice::Instance->GetShaderProfile();
    const int32 cacheChunkIndex = GetCacheChunkIndex(shaderProfile);
    const auto cachingMode = ShaderStorage::GetCachingMode();
#if COMPILE_WITH_SHADER_CACHE_MANAGER
    ShaderCacheManager::CachedEntryHandle cachedEntry;
#endif

#if COMPILE_WITH_SHADER_COMPILER

    // Try to get cached shader (based on current caching policy)
    bool hasCache = false;
    if (cachingMode == ShaderStorage::CachingMode::AssetInternal)
    {
        if (parent->HasChunkLoaded(cacheChunkIndex))
        {
            // Load cached shader data, create reading stream (memory free, super fast, super easy, no overhead at all)
            const auto cacheChunk = parent->GetChunk(cacheChunkIndex);
            result.Data.Link(cacheChunk->Data);
            if (IsValidShaderCache(result.Data, result.Includes))
            {
                hasCache = true;
            }
            else
            {
                result.Data.Release();
            }
        }
    }
#if COMPILE_WITH_SHADER_CACHE_MANAGER
    else if (cachingMode == ShaderStorage::CachingMode::ProjectCache)
    {
        // Try get cached shader cache
        const DateTime assetModificationDate = parent->Storage ? FileSystem::GetFileLastEditTime(parent->Storage->GetPath()) : DateTime::MinValue();
        if (ShaderCacheManager::TryGetEntry(shaderProfile, parent->GetID(), cachedEntry)
            && cachedEntry.GetModificationDate() > assetModificationDate
            && !ShaderCacheManager::GetCache(shaderProfile, cachedEntry, result.Data)
            && IsValidShaderCache(result.Data, result.Includes))
        {
            hasCache = true;
        }
        else
        {
            result.Data.Release();
        }
    }
#endif

    // Check if shader should be compiled
#if BUILD_DEBUG
    const bool forceRecompile = false;
#else
    const bool forceRecompile = false;
#endif
    if ((forceRecompile || GPU_FORCE_RECOMPILE_SHADERS || !hasCache)
        && parent->HasChunk(SHADER_FILE_CHUNK_SOURCE))
    {
        result.Data.Release();
        const String parentPath = parent->GetPath();
        const Guid parentID = parent->GetID();
        LOG(Info, "Compiling shader '{0}':{1}...", parentPath, parentID);

        // Load all chunks (except that with internal cache for current shader profile, it will be generated)
        AssetChunksFlag chunksToLoad = ALL_ASSET_CHUNKS;
        chunksToLoad &= ~GET_CHUNK_FLAG(cacheChunkIndex);
        if (parent->LoadChunks(chunksToLoad))
        {
            LOG(Warning, "Cannot load '{0}' data from chunk {1}.", parent->ToString(), chunksToLoad);
            return true;
        }

        // Remove current profile internal chunk (it could be loaded by the asset itself during precaching phrase)
        parent->ReleaseChunk(cacheChunkIndex);

        // Decrypt source code
        auto sourceChunk = parent->GetChunk(SHADER_FILE_CHUNK_SOURCE);
        auto source = sourceChunk->Get<char>();
        auto sourceLength = sourceChunk->Size();
        Encryption::DecryptBytes((byte*)source, sourceLength);
        source[sourceLength - 1] = 0;

        // Init shader cache output stream
        // TODO: use cached data per thread?
        MemoryWriteStream cacheStream(32 * 1024);

        // Compile shader source
        ShaderCompilationOptions options;
        options.TargetName = StringUtils::GetFileNameWithoutExtension(parentPath);
        options.TargetID = parentID;
        options.Source = source;
        options.SourceLength = sourceLength;
        options.Profile = shaderProfile;
        options.Output = &cacheStream;
        if (CommandLine::Options.ShaderDebug.IsTrue())
        {
            options.GenerateDebugData = true;
            options.NoOptimize = true;
        }
        else if (CommandLine::Options.ShaderProfile.IsTrue())
        {
            options.GenerateDebugData = true;
        }
        auto& platformDefine = options.Macros.AddOne();
#if PLATFORM_WINDOWS
        platformDefine.Name = "PLATFORM_WINDOWS";
#elif PLATFORM_LINUX
        platformDefine.Name = "PLATFORM_LINUX";
#elif PLATFORM_MAC
        platformDefine.Name = "PLATFORM_MAC";
#else
#error "Unknown platform."
#endif
        platformDefine.Definition = "1";
#if USE_EDITOR
        auto& editorDefine = options.Macros.AddOne();
        editorDefine.Name = "USE_EDITOR";
        editorDefine.Definition = "1";
#endif
        InitCompilationOptions(options);
        const bool failed = ShadersCompilation::Compile(options);

        // Encrypt source code
        Encryption::EncryptBytes(reinterpret_cast<byte*>(source), sourceLength);

        if (failed)
        {
            LOG(Error, "Failed to compile shader '{0}'", parent->ToString());
            return true;
        }
        LOG(Info, "Shader '{0}' compiled! Cache size: {1} bytes", parent->ToString(), cacheStream.GetPosition());

        // Save compilation result (based on current caching policy)
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal)
        {
            // Save cache to the internal shader cache chunk
            auto cacheChunk = parent->GetOrCreateChunk(cacheChunkIndex);
            if (cacheChunk == nullptr)
                return true;
            cacheChunk->Data.Copy(ToSpan(cacheStream));

#if USE_EDITOR
            // Save chunks to the asset file
            if (SaveShaderAsset())
            {
                LOG(Warning, "Cannot save '{0}'.", parent->ToString());
                return true;
            }
#endif
        }
#if COMPILE_WITH_SHADER_CACHE_MANAGER
        else if (cachingMode == ShaderStorage::CachingMode::ProjectCache)
        {
            // Save results to cache
            if (ShaderCacheManager::SetCache(shaderProfile, cachedEntry, cacheStream))
            {
                LOG(Warning, "Cannot save shader cache.");
                return true;
            }
        }
#endif
        else
        {
            // Use temporary generated data without caching (but read the includes from cache)
            result.Data.Copy(ToSpan(cacheStream));
            IsValidShaderCache(result.Data, result.Includes);
            return false;
        }
    }
    else if (hasCache && result.Data.IsValid())
    {
        // The cached version is valid
        return false;
    }

#endif

    // Check if has internal shader cache
    if (parent->HasChunkLoaded(cacheChunkIndex))
    {
        // Load cached shader data, create reading stream (memory free, super fast, super easy, no overhead at all)
        const auto cacheChunk = parent->GetChunk(cacheChunkIndex);
        result.Data.Link(cacheChunk->Data);
    }
#if COMPILE_WITH_SHADER_CACHE_MANAGER
    // Check if has cached shader
    else if (cachedEntry.IsValid() || ShaderCacheManager::TryGetEntry(shaderProfile, parent->GetID(), cachedEntry))
    {
        // Load results from cache
        if (ShaderCacheManager::GetCache(shaderProfile, cachedEntry, result.Data))
        {
            LOG(Warning, "Cannot load shader cache.");
            return true;
        }
    }
#endif
    else
    {
        LOG(Warning, "Missing shader cache '{0}'.", parent->ToString());
        return true;
    }

    ASSERT(result.Data.IsValid());

#if COMPILE_WITH_SHADER_COMPILER
    // Read includes from cache
    IsValidShaderCache(result.Data, result.Includes);
#endif

    return false;
}

#if COMPILE_WITH_SHADER_COMPILER

void ShaderAssetBase::RegisterForShaderReloads(Asset* asset, const ShaderCacheResult& shaderCache)
{
    for (auto& include : shaderCache.Includes)
    {
        ShadersCompilation::RegisterForShaderReloads(asset, include);
    }
}

void ShaderAssetBase::UnregisterForShaderReloads(Asset* asset)
{
    ShadersCompilation::UnregisterForShaderReloads(asset);
}

#endif
