// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_SHADER_CACHE_MANAGER

#include "ShaderCacheManager.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Globals.h"
#if USE_EDITOR
#include "Engine/Engine/CommandLine.h"
#endif
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Materials/MaterialShader.h"
#include "Engine/Particles/Graph/GPU/ParticleEmitterGraph.GPU.h"
#include "FlaxEngine.Gen.h"

const Char* ShaderProfileCacheDirNames[] =
{
    // @formatter:off
    TEXT(""), // Unknown
    TEXT("DX_SM4"), // DirectX_SM4
    TEXT("DX_SM5"), // DirectX_SM5
    TEXT("GLSL_410"), // GLSL_410
    TEXT("GLSL_440"), // GLSL_440
    TEXT("VK_SM5"), // Vulkan_SM5
    TEXT("PS4"), // PS4
    TEXT("DX_SM6"), // DirectX_SM6
    TEXT("PS5"), // PS5
    // @formatter:on
};

static_assert(ARRAY_COUNT(ShaderProfileCacheDirNames) == (int32)ShaderProfile::MAX, "Invalid shaders cache dirs");

class ShaderProfileDatabase
{
public:
    ShaderProfile Profile;
    String Folder;

public:
    void Init(ShaderProfile profile, const String& cacheRoot)
    {
        Profile = profile;
        Folder = cacheRoot / ShaderProfileCacheDirNames[static_cast<int32>(profile)];
        if (!FileSystem::DirectoryExists(Folder))
        {
            if (FileSystem::CreateDirectory(Folder))
            {
                LOG(Warning, "Cannot create cache directory for ShaderProfileDatabase: {0} (path: \'{1}\')", ::ToString(Profile), Folder);
            }
        }
    }

    bool TryGetEntry(const Guid& id, ShaderCacheManager::CachedEntryHandle& cachedEntry) const
    {
        ASSERT(id.IsValid());

        cachedEntry.ID = id;
        cachedEntry.Path = Folder / id.ToString(Guid::FormatType::D);
        return cachedEntry.Exists();
    }

    bool GetCache(const ShaderCacheManager::CachedEntryHandle& cachedEntry, BytesContainer& outputShaderCache) const
    {
        ASSERT(cachedEntry.IsValid());
        return File::ReadAllBytes(cachedEntry.Path, outputShaderCache);
    }

    static bool SetCache(const ShaderCacheManager::CachedEntryHandle& cachedEntry, MemoryWriteStream& inputShaderCache)
    {
        ASSERT(cachedEntry.IsValid() && inputShaderCache.GetHandle() != nullptr);
        return inputShaderCache.SaveToFile(cachedEntry.Path);
    }

    void RemoveCache(const Guid& id) const
    {
        ASSERT(id.IsValid());
        ShaderCacheManager::CachedEntryHandle cachedEntry;
        cachedEntry.ID = id;
        cachedEntry.Path = Folder / id.ToString(Guid::FormatType::D);
        FileSystem::DeleteFile(cachedEntry.Path);
    }
};

class ShaderCacheManagerService : public EngineService
{
public:
    ShaderCacheManagerService()
        : EngineService(TEXT("Shader Cache Manager"), -200)
    {
    }

    bool Init() override;
};

ShaderCacheManagerService ShaderCacheManagerServiceInstance;

ShaderProfileDatabase Databases[(int32)ShaderProfile::MAX - 1];

static int32 shaderProfile2Index(const ShaderProfile profile)
{
    return static_cast<int32>(profile) - 1;
}

static ShaderProfile index2ShaderProfile(const int32 index)
{
    return static_cast<ShaderProfile>(index + 1);
}

bool ShaderCacheManager::CachedEntryHandle::IsValid() const
{
    return ID.IsValid();
}

bool ShaderCacheManager::CachedEntryHandle::Exists() const
{
    return FileSystem::FileExists(Path);
}

DateTime ShaderCacheManager::CachedEntryHandle::GetModificationDate() const
{
    return FileSystem::GetFileLastEditTime(Path);
}

bool ShaderCacheManager::TryGetEntry(const ShaderProfile profile, const Guid& id, CachedEntryHandle& cachedEntry)
{
    ASSERT(profile != ShaderProfile::Unknown);
    return Databases[shaderProfile2Index(profile)].TryGetEntry(id, cachedEntry);
}

bool ShaderCacheManager::GetCache(const ShaderProfile profile, const CachedEntryHandle& cachedEntry, BytesContainer& outputShaderCache)
{
    ASSERT(profile != ShaderProfile::Unknown);
    return Databases[shaderProfile2Index(profile)].GetCache(cachedEntry, outputShaderCache);
}

bool ShaderCacheManager::SetCache(const ShaderProfile profile, const CachedEntryHandle& cachedEntry, MemoryWriteStream& inputShaderCache)
{
    ASSERT(profile != ShaderProfile::Unknown);
    return Databases[shaderProfile2Index(profile)].SetCache(cachedEntry, inputShaderCache);
}

void ShaderCacheManager::RemoveCache(const ShaderProfile profile, const Guid& id)
{
    ASSERT(profile != ShaderProfile::Unknown);
    Databases[shaderProfile2Index(profile)].RemoveCache(id);
}

void ShaderCacheManager::RemoveCache(const Guid& id)
{
    for (int32 i = 0; i < ARRAY_COUNT(Databases); i++)
        Databases[i].RemoveCache(id);
}

void ShaderCacheManager::CopyCache(const Guid& dstId, const Guid& srcId)
{
    ASSERT(dstId.IsValid() && srcId.IsValid());

    String dstFilename = dstId.ToString(Guid::FormatType::D);
    String srcFilename = srcId.ToString(Guid::FormatType::D);
    for (int32 i = 0; i < ARRAY_COUNT(Databases); i++)
    {
        const String& folder = Databases[i].Folder;
        String dstPath = folder / dstFilename;
        String srcPath = folder / srcFilename;

        if (FileSystem::FileExists(srcPath))
            FileSystem::CopyFile(dstPath, srcPath);
        else if (FileSystem::FileExists(dstPath))
            FileSystem::DeleteFile(dstPath);
    }
}

bool ShaderCacheManagerService::Init()
{
#if USE_EDITOR
    const String rootDir = Globals::ProjectCacheFolder / TEXT("Shaders/Cache");
#else
    const String rootDir = Globals::ProductLocalFolder / TEXT("Shaders/Cache");
#endif

    // Validate the database cache version (need to recompile all shaders on shader cache format change)
    struct CacheVersion
    {
        int32 EngineVersion = -1;
        int32 ShaderCacheVersion = -1;
        int32 MaterialGraphVersion = -1;
        int32 ParticleGraphVersion = -1;
        bool ShaderDebug;
        bool ShaderProfile;
    };
    CacheVersion cacheVersion;
    const String cacheVerFile = rootDir / TEXT("CacheVersion");
#if USE_EDITOR
    const bool shaderDebug = CommandLine::Options.ShaderDebug.IsTrue();
    const bool shaderProfile = CommandLine::Options.ShaderProfile.IsTrue();
#else
    const bool shaderDebug = false;
#endif
    if (FileSystem::FileExists(cacheVerFile))
    {
        if (File::ReadAllBytes(cacheVerFile, (byte*)&cacheVersion, sizeof(cacheVersion)))
        {
            cacheVersion = CacheVersion();
            LOG(Warning, "Failed to read the shaders cache database version file.");
        }
    }
    if (cacheVersion.EngineVersion != FLAXENGINE_VERSION_BUILD
        || cacheVersion.ShaderCacheVersion != GPU_SHADER_CACHE_VERSION
        || cacheVersion.MaterialGraphVersion != MATERIAL_GRAPH_VERSION
        || cacheVersion.ParticleGraphVersion != PARTICLE_GPU_GRAPH_VERSION
        || cacheVersion.ShaderDebug != shaderDebug
        || cacheVersion.ShaderProfile != shaderProfile
    )
    {
        LOG(Warning, "Shaders cache database is invalid. Performing reset.");

        if (FileSystem::DirectoryExists(rootDir) && FileSystem::DeleteDirectory(rootDir))
        {
            LOG(Warning, "Failed to reset the shaders cache database.");
        }
        if (FileSystem::CreateDirectory(rootDir))
        {
            LOG(Error, "Failed to createe the shaders cache database directory.");
        }

        cacheVersion.EngineVersion = FLAXENGINE_VERSION_BUILD;
        cacheVersion.ShaderCacheVersion = GPU_SHADER_CACHE_VERSION;
        cacheVersion.MaterialGraphVersion = MATERIAL_GRAPH_VERSION;
        cacheVersion.ParticleGraphVersion = PARTICLE_GPU_GRAPH_VERSION;
        cacheVersion.ShaderDebug = shaderDebug;
        cacheVersion.ShaderProfile = shaderProfile;
        if (File::WriteAllBytes(cacheVerFile, (byte*)&cacheVersion, sizeof(cacheVersion)))
        {
            LOG(Error, "Failed to create the shaders cache database version file.");
        }
    }

    // Init shader formats databases
    for (int32 i = 0; i < ARRAY_COUNT(Databases); i++)
    {
        Databases[i].Init(index2ShaderProfile(i), rootDir);
    }

    return false;
}

#endif
