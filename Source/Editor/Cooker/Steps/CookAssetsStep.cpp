// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CookAssetsStep.h"
#include "Editor/Cooker/PlatformTools.h"
#include "Engine/Core/DeleteMe.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/BinaryAsset.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Render2D/SpriteAtlas.h"
#include "Engine/Content/Storage/FlaxFile.h"
#include "Engine/Particles/ParticleEmitter.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Core/Config/PlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/ShadersCompilation/ShadersCompilation.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Engine/Base/GameBase.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#if PLATFORM_TOOLS_WINDOWS
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_UWP
#include "Engine/Platform/UWP/UWPPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_LINUX
#include "Engine/Platform/Linux/LinuxPlatformSettings.h"
#endif
#include "FlaxEngine.Gen.h"

Dictionary<String, CookAssetsStep::ProcessAssetFunc> CookAssetsStep::AssetProcessors;

CookAssetsStep::CacheEntry& CookAssetsStep::CacheData::CreateEntry(const JsonAssetBase* asset, String& cachedFilePath)
{
    ASSERT(asset->DataTypeName.HasChars());
    auto& entry = Entries[asset->GetID()];
    entry.ID = asset->GetID();
    entry.TypeName = asset->DataTypeName;
    entry.FileModified = FileSystem::GetFileLastEditTime(asset->GetPath());
    cachedFilePath = CacheFolder / entry.ID.ToString(Guid::FormatType::N);
    return entry;
}

CookAssetsStep::CacheEntry& CookAssetsStep::CacheData::CreateEntry(const Asset* asset, String& cachedFilePath)
{
    auto& entry = Entries[asset->GetID()];
    entry.ID = asset->GetID();
    entry.TypeName = asset->GetTypeName();
    entry.FileModified = FileSystem::GetFileLastEditTime(asset->GetPath());
    cachedFilePath = CacheFolder / entry.ID.ToString(Guid::FormatType::N);
    return entry;
}

void CookAssetsStep::CacheData::InvalidateShaders()
{
    LOG(Info, "Invalidating cached shader assets.");

    for (auto e = Entries.Begin(); e.IsNotEnd(); ++e)
    {
        auto& typeName = e->Value.TypeName;
        if (
            typeName == Shader::TypeName ||
            typeName == Material::TypeName ||
            typeName == ParticleEmitter::TypeName
        )
        {
            Entries.Remove(e);
        }
    }
}

void CookAssetsStep::CacheData::Load(CookingData& data)
{
    HeaderFilePath = data.CacheDirectory / String::Format(TEXT("CookedHeader_{0}.bin"), FLAXENGINE_VERSION_BUILD);
    CacheFolder = data.CacheDirectory / TEXT("Cooked");
    Entries.Clear();

    if (!FileSystem::DirectoryExists(CacheFolder))
    {
        FileSystem::CreateDirectory(CacheFolder);
    }

    if (!FileSystem::FileExists(HeaderFilePath))
    {
        LOG(Warning, "Missing incremental build cooking assets cache.");
        return;
    }

    auto file = FileReadStream::Open(HeaderFilePath);
    if (file == nullptr)
        return;
    DeleteMe<FileReadStream> deleteFile(file);

    int32 buildNum;
    file->ReadInt32(&buildNum);
    if (buildNum != FLAXENGINE_VERSION_BUILD)
        return;
    int32 entriesCount;
    file->ReadInt32(&entriesCount);
    if (Math::IsNotInRange(entriesCount, 0, 1000000))
        return;

    LOG(Info, "Loading incremental build cooking cache (entries count: {0})", entriesCount);

    file->Read(&Settings);

    Entries.EnsureCapacity(Math::RoundUpToPowerOf2(static_cast<int32>(entriesCount * 3.0f)));

    Array<Pair<String, DateTime>> fileDependencies;
    for (int32 i = 0; i < entriesCount; i++)
    {
        Guid id;
        file->Read(&id);
        String typeName;
        file->ReadString(&typeName);
        DateTime fileModified;
        file->Read(&fileModified);
        int32 fileDependenciesCount;
        file->ReadInt32(&fileDependenciesCount);
        fileDependencies.Clear();
        fileDependencies.Resize(fileDependenciesCount);
        for (int32 j = 0; j < fileDependenciesCount; j++)
        {
            Pair<String, DateTime>& f = fileDependencies[j];
            file->ReadString(&f.First, 10);
            file->Read(&f.Second);
        }

        // Skip missing entries
        if (!FileSystem::FileExists(CacheFolder / id.ToString(Guid::FormatType::N)))
            continue;

        auto& e = Entries[id];
        e.ID = id;
        e.TypeName = typeName;
        e.FileModified = fileModified;
        e.FileDependencies = fileDependencies;
    }

    int32 checkChar;
    file->ReadInt32(&checkChar);
    if (checkChar != 13)
    {
        LOG(Warning, "Corrupted cooking cache header file.");
        Entries.Clear();
    }

    // Invalidate shaders and assets with shaders if need to rebuild them
    bool invalidateShaders = false;
    const auto buildSettings = BuildSettings::Get();
    const bool shadersNoOptimize = buildSettings->ShadersNoOptimize;
    const bool shadersGenerateDebugData = buildSettings->ShadersGenerateDebugData;
    if (shadersNoOptimize != Settings.Global.ShadersNoOptimize)
    {
        LOG(Info, "ShadersNoOptimize option has been modified.");
        invalidateShaders = true;
    }
    if (shadersGenerateDebugData != Settings.Global.ShadersGenerateDebugData)
    {
        LOG(Info, "ShadersGenerateDebugData option has been modified.");
        invalidateShaders = true;
    }
#if PLATFORM_TOOLS_WINDOWS
    if (data.Platform == BuildPlatform::Windows32 || data.Platform == BuildPlatform::Windows64)
    {
        const auto settings = WindowsPlatformSettings::Get();
        const bool modified =
                Settings.Windows.SupportDX11 != settings->SupportDX11 ||
                Settings.Windows.SupportDX10 != settings->SupportDX10 ||
                Settings.Windows.SupportVulkan != settings->SupportVulkan;
        if (modified)
        {
            LOG(Info, "Platform graphics backend options has been modified.");
            invalidateShaders = true;
        }
    }
#endif
#if PLATFORM_TOOLS_UWP
    if (data.Platform == BuildPlatform::UWPx86 || data.Platform == BuildPlatform::UWPx64)
    {
        const auto settings = UWPPlatformSettings::Get();
        const bool modified =
                Settings.UWP.SupportDX11 != settings->SupportDX11 ||
                Settings.UWP.SupportDX10 != settings->SupportDX10;
        if (modified)
        {
            LOG(Info, "Platform graphics backend options has been modified.");
            invalidateShaders = true;
        }
    }
#endif
#if PLATFORM_TOOLS_LINUX
    if (data.Platform == BuildPlatform::LinuxX64)
    {
        const auto settings = LinuxPlatformSettings::Get();
        const bool modified =
                Settings.Linux.SupportVulkan != settings->SupportVulkan;
        if (modified)
        {
            LOG(Info, "Platform graphics backend options has been modified.");
            invalidateShaders = true;
        }
    }
#endif
    if (invalidateShaders)
        InvalidateShaders();
}

void CookAssetsStep::CacheData::Save()
{
    LOG(Info, "Saving incremental build cooking cache (entries count: {0})", Entries.Count());

    auto file = FileWriteStream::Open(HeaderFilePath);
    if (file == nullptr)
        return;
    DeleteMe<FileWriteStream> deleteFile(file);

    // Serialize
    file->WriteInt32(FLAXENGINE_VERSION_BUILD);
    file->WriteInt32(Entries.Count());
    file->Write(&Settings);
    for (auto i = Entries.Begin(); i.IsNotEnd(); ++i)
    {
        auto& e = i->Value;
        file->Write(&e.ID);
        file->WriteString(e.TypeName);
        file->Write(&e.FileModified);
        file->WriteInt32(e.FileDependencies.Count());
        for (auto& f : e.FileDependencies)
        {
            file->WriteString(f.First, 10);
            file->Write(&f.Second);
        }
    }
    file->WriteInt32(13);
}

bool CookAssetsStep::ProcessDefaultAsset(AssetCookData& options)
{
    const auto asBinaryAsset = dynamic_cast<BinaryAsset*>(options.Asset);
    if (asBinaryAsset)
    {
        // Use default cooking rule (copy data)
        if (asBinaryAsset->LoadChunks(ALL_ASSET_CHUNKS))
            return true;
        for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        {
            const auto chunk = asBinaryAsset->GetChunk(i);
            if (chunk)
                options.InitData.Header.Chunks[i] = chunk->Clone();
        }

        return false;
    }

    const auto asJsonAsset = dynamic_cast<JsonAssetBase*>(options.Asset);
    if (asJsonAsset)
    {
        // Use compact json
        rapidjson_flax::StringBuffer buffer;
        rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
        asJsonAsset->Document.Accept(writer);

        // Store json data in the first chunk
        auto chunk = New<FlaxChunk>();
        chunk->Flags = FlaxChunkFlags::CompressedLZ4; // Compress json data (internal storage layer will handle it)
        chunk->Data.Copy((byte*)buffer.GetString(), (int32)buffer.GetSize());
        options.InitData.Header.Chunks[0] = chunk;

        return false;
    }

    LOG(Error, "Unknown asset type \'{0}\'", options.Asset->GetTypeName());
    return false;
}

bool CookAssetsStep::Process(CookingData& data, CacheData& cache, Asset* asset)
{
    // Validate asset
    if (asset->IsVirtual())
    {
        // Virtual assets are not included into the build
        return false;
    }
    if (asset->WaitForLoaded())
    {
        LOG(Error, "Failed to load asset \'{0}\'", asset->ToString());
        return false;
    }

    // Switch based on an asset type
    const auto asBinaryAsset = dynamic_cast<BinaryAsset*>(asset);
    if (asBinaryAsset)
        return Process(data, cache, asBinaryAsset);
    const auto asJsonAsset = dynamic_cast<JsonAssetBase*>(asset);
    if (asJsonAsset)
        return Process(data, cache, asJsonAsset);

    LOG(Error, "Unknown asset type \'{0}\'", asset->GetTypeName());
    return false;
}

bool ProcessShaderBase(CookAssetsStep::AssetCookData& data, ShaderAssetBase* assetBase)
{
    auto asset = static_cast<BinaryAsset*>(data.Asset);

    // Decrypt source code
    auto sourceChunk = asset->GetChunk(SHADER_FILE_CHUNK_SOURCE);
    auto source = sourceChunk->Get<char>();
    auto sourceLength = sourceChunk->Size();
    Encryption::DecryptBytes((byte*)source, sourceLength);
    source[sourceLength - 1] = 0;
    while (sourceLength > 2 && source[sourceLength - 1] == 0)
        sourceLength--;

    // Init shader cache output stream
    // TODO: reuse MemoryWriteStream per cooking process to reduce dynamic memory allocations
    MemoryWriteStream cacheStream(32 * 1024);

    // Compile shader source
    ShaderCompilationOptions options;
    options.TargetName = StringUtils::GetFileNameWithoutExtension(asset->GetPath());
    options.TargetID = asset->GetID();
    options.Source = source;
    options.SourceLength = sourceLength;
    options.NoOptimize = data.Cache.Settings.Global.ShadersNoOptimize;
    options.GenerateDebugData = data.Cache.Settings.Global.ShadersGenerateDebugData;
    options.TreatWarningsAsErrors = false;
    options.Output = &cacheStream;
    Array<String> includes;

#define COMPILE_PROFILE(profile, cacheChunk) \
	{ \
		cacheStream.SetPosition(0); \
		options.Profile = ShaderProfile::profile; \
		options.Macros.Clear(); \
		auto& platformDefine = options.Macros.AddOne(); \
		platformDefine.Name = platformDefineName; \
		platformDefine.Definition = nullptr; \
		assetBase->InitCompilationOptions(options); \
		if (ShadersCompilation::Compile(options)) \
		{ \
			data.Data.Error(TEXT("Failed to compile shader '{0}' (profile: {1})."), asset->ToString(), ::ToString(options.Profile)); \
			return true; \
		} \
        includes.Clear(); \
        ShadersCompilation::ExtractShaderIncludes(cacheStream.GetHandle(), cacheStream.GetPosition(), includes); \
        for (auto& include : includes) \
            data.FileDependencies.Add(ToPair(include, FileSystem::GetFileLastEditTime(include))); \
		auto chunk = New<FlaxChunk>(); \
		chunk->Data.Copy(cacheStream.GetHandle(), cacheStream.GetPosition()); \
		data.InitData.Header.Chunks[cacheChunk] = chunk; \
	}

    // Compile for a target platform
    switch (data.Data.Platform)
    {
    case BuildPlatform::Windows32:
    case BuildPlatform::Windows64:
    {
        const char* platformDefineName = "PLATFORM_WINDOWS";
        const auto settings = WindowsPlatformSettings::Get();
        if (settings->SupportDX12)
        {
            COMPILE_PROFILE(DirectX_SM6, SHADER_FILE_CHUNK_INTERNAL_D3D_SM6_CACHE);
        }
        if (settings->SupportDX11)
        {
            COMPILE_PROFILE(DirectX_SM5, SHADER_FILE_CHUNK_INTERNAL_D3D_SM5_CACHE);
        }
        if (settings->SupportDX10)
        {
            COMPILE_PROFILE(DirectX_SM4, SHADER_FILE_CHUNK_INTERNAL_D3D_SM4_CACHE);
        }
        if (settings->SupportVulkan)
        {
            COMPILE_PROFILE(Vulkan_SM5, SHADER_FILE_CHUNK_INTERNAL_VULKAN_SM5_CACHE);
        }
        break;
    }
#if PLATFORM_TOOLS_UWP
    case BuildPlatform::UWPx86:
    case BuildPlatform::UWPx64:
    {
        const char* platformDefineName = "PLATFORM_UWP";
        const auto settings = UWPPlatformSettings::Get();
        if (settings->SupportDX11)
        {
            COMPILE_PROFILE(DirectX_SM5, SHADER_FILE_CHUNK_INTERNAL_D3D_SM5_CACHE);
        }
        if (settings->SupportDX10)
        {
            COMPILE_PROFILE(DirectX_SM4, SHADER_FILE_CHUNK_INTERNAL_D3D_SM4_CACHE);
        }
        break;
    }
#endif
    case BuildPlatform::XboxOne:
    {
        const char* platformDefineName = "PLATFORM_XBOX_ONE";
        COMPILE_PROFILE(DirectX_SM4, SHADER_FILE_CHUNK_INTERNAL_D3D_SM4_CACHE);
        break;
    }
#if PLATFORM_TOOLS_LINUX
    case BuildPlatform::LinuxX64:
    {
        const char* platformDefineName = "PLATFORM_LINUX";
        const auto settings = LinuxPlatformSettings::Get();
        if (settings->SupportVulkan)
        {
            COMPILE_PROFILE(Vulkan_SM5, SHADER_FILE_CHUNK_INTERNAL_VULKAN_SM5_CACHE);
        }
        break;
    }
#endif
    case BuildPlatform::PS4:
    {
        const char* platformDefineName = "PLATFORM_PS4";
        COMPILE_PROFILE(PS4, SHADER_FILE_CHUNK_INTERNAL_GENERIC_CACHE);
        break;
    }
    case BuildPlatform::XboxScarlett:
    {
        const char* platformDefineName = "PLATFORM_XBOX_SCARLETT";
        COMPILE_PROFILE(DirectX_SM6, SHADER_FILE_CHUNK_INTERNAL_D3D_SM6_CACHE);
        break;
    }
    case BuildPlatform::AndroidARM64:
    {
        const char* platformDefineName = "PLATFORM_ANDROID";
        COMPILE_PROFILE(Vulkan_SM5, SHADER_FILE_CHUNK_INTERNAL_VULKAN_SM5_CACHE);
        break;
    }
    default:
    {
        LOG(Warning, "Not implemented platform or shaders not supported.");
        return true;
    }
    }

    // Encrypt source code
    Encryption::EncryptBytes(reinterpret_cast<byte*>(source), sourceLength);

    return false;
}

bool ProcessMaterial(CookAssetsStep::AssetCookData& data)
{
    auto asset = static_cast<Material*>(data.Asset);

    // Material is loaded so it has valid source code generated from the Visject Surface.
    // Material::load performs any required upgrading and conversions.

    // Load material params and source code
    if (asset->LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS) | GET_CHUNK_FLAG(SHADER_FILE_CHUNK_SOURCE)))
        return true;

    // Copy material params data
    const auto paramsChunk = asset->GetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
    if (paramsChunk)
        data.InitData.Header.Chunks[SHADER_FILE_CHUNK_MATERIAL_PARAMS] = paramsChunk->Clone();

    // Compile shader for the target platform rendering devices
    return ProcessShaderBase(data, asset);
}

bool ProcessShader(CookAssetsStep::AssetCookData& data)
{
    auto asset = static_cast<Shader*>(data.Asset);

    // Load source code
    if (asset->LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_SOURCE)))
        return true;

    // Compile shader for the target platform rendering devices
    return ProcessShaderBase(data, asset);
}

bool ProcessParticleEmitter(CookAssetsStep::AssetCookData& data)
{
    auto asset = static_cast<ParticleEmitter*>(data.Asset);

    // Particle Emitter is loaded so it has valid source code generated from the Visject Surface.
    // ParticleEmitter::load performs any required upgrading and conversions.

    // Load surface, material params and source code
    if (asset->LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE) | GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS) | GET_CHUNK_FLAG(SHADER_FILE_CHUNK_SOURCE)))
        return true;

    // Copy surface data
    const auto surfaceChunk = asset->GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
    if (surfaceChunk)
        data.InitData.Header.Chunks[SHADER_FILE_CHUNK_VISJECT_SURFACE] = surfaceChunk->Clone();

    // Skip cooking shader if it's not using GPU particles
    const auto sourceChunk = asset->GetChunk(SHADER_FILE_CHUNK_SOURCE);
    if (sourceChunk == nullptr || asset->SimulationMode == ParticlesSimulationMode::CPU)
        return false;

    // Copy material params data
    const auto paramsChunk = asset->GetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
    if (paramsChunk)
        data.InitData.Header.Chunks[SHADER_FILE_CHUNK_MATERIAL_PARAMS] = paramsChunk->Clone();

    // Compile shader for the target platform rendering devices
    return ProcessShaderBase(data, asset);
}

bool ProcessTextureBase(CookAssetsStep::AssetCookData& data)
{
    const auto asset = static_cast<TextureBase*>(data.Asset);

    // Check if target platform doesn't support the texture format
    const auto format = asset->Format();
    const auto targetFormat = data.Data.Tools->GetTextureFormat(data.Data, asset, format);
    if (format != targetFormat)
    {
        // Extract texture data from the asset
        TextureData textureData;
        auto assetLock = asset->LockData();
        if (asset->GetTextureData(textureData, false))
        {
            LOG(Error, "Failed to load data from texture {0}", asset->ToString());
            return true;
        }

        // Convert texture data to the target format
        TextureData targetTextureData;
        if (TextureTool::Convert(targetTextureData, textureData, targetFormat))
        {
            LOG(Error, "Failed to convert texture {0} from format {1} to {2}", asset->ToString(), (int32)format, (int32)targetFormat);
            return true;
        }

        // Adjust texture header
        auto& header = *(TextureHeader*)data.InitData.CustomData.Get();
        header.Width = targetTextureData.Width;
        header.Height = targetTextureData.Height;
        header.Format = targetTextureData.Format;
        header.MipLevels = targetTextureData.GetMipLevels();

        // Serialize texture data into the asset chunks
        for (int32 mipIndex = 0; mipIndex < targetTextureData.GetMipLevels(); mipIndex++)
        {
            auto chunk = New<FlaxChunk>();
            data.InitData.Header.Chunks[mipIndex] = chunk;

            // Calculate the texture data storage layout
            uint32 rowPitch, slicePitch;
            const int32 mipWidth = Math::Max(1, targetTextureData.Width >> mipIndex);
            const int32 mipHeight = Math::Max(1, targetTextureData.Height >> mipIndex);
            RenderTools::ComputePitch(targetTextureData.Format, mipWidth, mipHeight, rowPitch, slicePitch);
            chunk->Data.Allocate(slicePitch * targetTextureData.GetArraySize());

            // Copy array slices into mip data (sequential)
            for (int32 arrayIndex = 0; arrayIndex < targetTextureData.Items.Count(); arrayIndex++)
            {
                auto& mipData = targetTextureData.Items[arrayIndex].Mips[mipIndex];
                byte* src = mipData.Data.Get();
                byte* dst = chunk->Data.Get() + (slicePitch * arrayIndex);

                // Faster path if source and destination data layout matches
                if (rowPitch == mipData.RowPitch && slicePitch == mipData.DepthPitch)
                {
                    Platform::MemoryCopy(dst, src, slicePitch);
                }
                else
                {
                    const auto copyRowSize = Math::Min(mipData.RowPitch, rowPitch);
                    for (uint32 line = 0; line < mipData.Lines; line++)
                    {
                        Platform::MemoryCopy(dst, src, copyRowSize);
                        src += mipData.RowPitch;
                        dst += rowPitch;
                    }
                }
            }
        }

        // Clone any custom asset chunks (eg. sprite atlas data, mips are in 0-13 chunks)
        for (int32 i = 14; i < ASSET_FILE_DATA_CHUNKS; i++)
        {
            const auto chunk = asset->GetChunk(i);
            if (chunk != nullptr && chunk->IsMissing() && chunk->ExistsInFile())
            {
                if (asset->Storage->LoadAssetChunk(chunk))
                    return true;
                data.InitData.Header.Chunks[i] = chunk->Clone();
            }
        }

        return false;
    }

    // Fallback to the default asset processing
    return CookAssetsStep::ProcessDefaultAsset(data);
}

CookAssetsStep::CookAssetsStep()
    : AssetsRegistry(1024)
    , AssetPathsMapping(256)
{
    AssetProcessors.Add(Material::TypeName, ProcessMaterial);
    AssetProcessors.Add(Shader::TypeName, ProcessShader);
    AssetProcessors.Add(ParticleEmitter::TypeName, ProcessParticleEmitter);
    AssetProcessors.Add(Texture::TypeName, ProcessTextureBase);
    AssetProcessors.Add(CubeTexture::TypeName, ProcessTextureBase);
    AssetProcessors.Add(SpriteAtlas::TypeName, ProcessTextureBase);
}

bool CookAssetsStep::Process(CookingData& data, CacheData& cache, BinaryAsset* asset)
{
    ASSERT(asset->IsLoaded() && asset->Storage != nullptr);
    FileDependenciesList fileDependencies;

    // Prepare asset data
    AssetInitData initData;
    if (asset->Storage->LoadAssetHeader(asset->GetID(), initData))
        return true;
    initData.Header.UnlinkChunks();
    initData.Metadata.Release();
    for (auto& e : initData.Dependencies)
    {
        AssetInfo info;
        if (Content::GetAssetInfo(e.First, info))
        {
            fileDependencies.Add(ToPair(info.Path, FileSystem::GetFileLastEditTime(info.Path)));
        }
    }
    initData.Dependencies.Resize(0);

    // Lock source asset chunks so they can be reused
    auto chunksLock = asset->Storage->LockSafe();

    // Process asset
    ProcessAssetFunc assetProcessor = nullptr;
    AssetProcessors.TryGet(asset->GetTypeName(), assetProcessor);
    AssetCookData options
    {
        data,
        cache,
        initData,
        asset,
        fileDependencies
    };
    if (!assetProcessor)
        assetProcessor = ProcessDefaultAsset;
    if (assetProcessor(options))
        return true;

    // Save cache
    String cachedFilePath;
    auto& entry = cache.CreateEntry(asset, cachedFilePath);
    entry.FileDependencies = MoveTemp(fileDependencies);
    const bool result = FlaxStorage::Create(cachedFilePath, initData);

    // Cleanup allocated data chunks
    initData.Header.DeleteChunks();

    if (result)
    {
        LOG(Warning, "Failed to save cooked file data.");
        return true;
    }
    return false;
}

bool CookAssetsStep::Process(CookingData& data, CacheData& cache, JsonAssetBase* asset)
{
    ASSERT(asset->IsLoaded() && asset->Data != nullptr);
    FileDependenciesList fileDependencies;

    // Create binary asset header
    AssetInitData initData;
    initData.SerializedVersion = 1;
    initData.Header.ID = asset->GetID();
    initData.Header.TypeName = asset->GetTypeName();

    // Process asset
    ProcessAssetFunc assetProcessor = nullptr;
    AssetProcessors.TryGet(asset->GetTypeName(), assetProcessor);
    AssetCookData options
    {
        data,
        cache,
        initData,
        asset,
        fileDependencies
    };
    if (!assetProcessor)
        assetProcessor = ProcessDefaultAsset;
    if (assetProcessor(options))
        return true;

    // Save cache
    String cachedFilePath;
    auto& entry = cache.CreateEntry(asset, cachedFilePath);
    entry.FileDependencies = MoveTemp(fileDependencies);
    const bool result = FlaxStorage::Create(cachedFilePath, initData);

    // Cleanup allocated data chunks
    initData.Header.DeleteChunks();

    if (result)
    {
        LOG(Warning, "Failed to save cooked file data.");
        return true;
    }
    return false;
}

/// <summary>
/// Helper utility to build a package of set of assets (using limits parameters).
/// </summary>
class PackageBuilder : public NonCopyable
{
private:

    int32 _packageIndex;
    int32 MaxAssetsPerPackage;
    int32 MaxPackageSize;
    FlaxStorage::CustomData CustomData;

    Array<FlaxFile*> files;
    Array<AssetsCache::Entry*> addedEntries;
    uint64 bytesAdded;

    uint64 packagesSizeTotal;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="PackageBuilder" /> class.
    /// </summary>
    /// <param name="maxAssetsPerPackage">The maximum assets per package.</param>
    /// <param name="maxPackageSizeMB">The maximum package size in MB.</param>
    /// <param name="contentKey">The content keycode.</param>
    PackageBuilder(int32 maxAssetsPerPackage, int32 maxPackageSizeMB, int32 contentKey)
        : _packageIndex(0)
        , MaxAssetsPerPackage(maxAssetsPerPackage)
        , MaxPackageSize(maxPackageSizeMB * (1024 * 1024))
        , files(maxAssetsPerPackage)
        , addedEntries(maxAssetsPerPackage)
        , bytesAdded(0)
        , packagesSizeTotal(0)
    {
        Platform::MemoryClear(&CustomData, sizeof(CustomData));
        CustomData.ContentKey = contentKey;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="PackageBuilder"/> class.
    /// </summary>
    ~PackageBuilder()
    {
        Reset();
    }

public:

    uint64 GetPackagesSizeTotal() const
    {
        return packagesSizeTotal;
    }

    void Reset()
    {
        for (int32 i = 0; i < files.Count(); i++)
        {
            files[i]->Dispose();
            Delete(files[i]);
        }
        files.Clear();
        addedEntries.Clear();
        bytesAdded = 0;
        _packageIndex++;
    }

    bool Add(CookingData& data, AssetsCache::Entry& entry, const String& sourcePath)
    {
        const uint64 size = FileSystem::GetFileSize(sourcePath);

        // Check if this will step out of the limit
        if (addedEntries.Count() + 1 > MaxAssetsPerPackage || (bytesAdded + size) > MaxPackageSize)
        {
            if (Package(data))
                return true;
        }

        // Add
        addedEntries.Add(&entry);
        bytesAdded += size;

        // Gather the asset to package it later
        auto file = New<FlaxFile>(sourcePath);
        if (file->Load())
        {
            Delete(file);
            data.Error(TEXT("Failed to load cooked asset."));
            return true;
        }
        files.Add(file);

        return false;
    }

    bool Package(CookingData& data)
    {
        // Skip if has no assets has been added
        const int32 count = addedEntries.Count();
        if (count == 0)
            return false;

        // Get assets init data and load all chunks
        Array<AssetInitData> assetsData;
        assetsData.Resize(count);
        for (int32 i = 0; i < count; i++)
        {
            if (files[i]->LoadAssetHeader(0, assetsData[i]))
            {
                data.Error(TEXT("Failed to load asset header data."));
                return true;
            }
            for (int32 j = 0; j < ASSET_FILE_DATA_CHUNKS; j++)
            {
                const auto chunk = assetsData[i].Header.Chunks[j];
                if (chunk)
                {
                    if (files[i]->LoadAssetChunk(chunk))
                    {
                        data.Error(TEXT("Failed to load asset data."));
                        return true;
                    }
                }
            }
        }

        // Create package
        // Note: FlaxStorage::Create overrides chunks locations in file so don't use files anymore (only readonly)
        const String localPath = String::Format(TEXT("Content/Data_{0}.{1}"), _packageIndex, PACKAGE_FILES_EXTENSION);
        const String path = data.OutputPath / localPath;
        if (FlaxStorage::Create(path, assetsData, false, &CustomData))
        {
            data.Error(TEXT("Failed to create assets package."));
            return true;
        }

        // Link storage info to all packaged assets
        for (int32 i = 0; i < count; i++)
        {
            addedEntries[i]->Info.Path = localPath;
        }

        packagesSizeTotal += FileSystem::GetFileSize(path);

        Reset();

        return false;
    }
};

bool CookAssetsStep::Perform(CookingData& data)
{
    float Step1ProgressStart = 0.1f;
    float Step1ProgressEnd = 0.6f;
    String Step1Info = TEXT("Cooking assets");
    float Step2ProgressStart = Step1ProgressEnd;
    float Step2ProgressEnd = 0.9f;
    String Step2Info = TEXT("Packaging assets");

    data.StepProgress(TEXT("Loading build cache"), 0);

    // Prepare
    const auto gameSettings = GameSettings::Get();
    const auto buildSettings = BuildSettings::Get();
    const int32 contentKey = buildSettings->ContentKey == 0 ? rand() : buildSettings->ContentKey;
    AssetsRegistry.Clear();
    AssetPathsMapping.Clear();

    // Load incremental build cache
    CacheData cache;
    cache.Load(data);

    // Update build settings
    {
        const auto settings = WindowsPlatformSettings::Get();
        cache.Settings.Windows.SupportDX11 = settings->SupportDX11;
        cache.Settings.Windows.SupportDX10 = settings->SupportDX10;
        cache.Settings.Windows.SupportVulkan = settings->SupportVulkan;
    }
    {
        const auto settings = UWPPlatformSettings::Get();
        cache.Settings.UWP.SupportDX11 = settings->SupportDX11;
        cache.Settings.UWP.SupportDX10 = settings->SupportDX10;
    }
    {
        const auto settings = LinuxPlatformSettings::Get();
        cache.Settings.Linux.SupportVulkan = settings->SupportVulkan;
    }
    {
        cache.Settings.Global.ShadersNoOptimize = buildSettings->ShadersNoOptimize;
        cache.Settings.Global.ShadersGenerateDebugData = buildSettings->ShadersGenerateDebugData;
    }

    // Note: this step converts all the assets (even the json) into the binary files (FlaxStorage format).
    // Then files cooked files are packed into the packages.

    // Process all assets
    AssetInfo assetInfo;
#if ENABLE_ASSETS_DISCOVERY
    auto minDateTime = DateTime::MinValue();
#endif
    int32 subStepIndex = 0;
    for (auto i = data.Assets.Begin(); i.IsNotEnd(); ++i)
    {
        BUILD_STEP_CANCEL_CHECK;

        data.StepProgress(Step1Info, Math::Lerp(Step1ProgressStart, Step1ProgressEnd, static_cast<float>(subStepIndex++) / data.Assets.Count()));
        const Guid assetId = i->Item;

        // Register asset
        auto& e = AssetsRegistry[assetId];
        e.Info.ID = assetId;
#if ENABLE_ASSETS_DISCOVERY
        e.FileModified = minDateTime;
#endif

        // Check if asset is in cooking cache and was not modified since last build
        const auto cachedEntry = cache.Entries.TryGet(assetId);
        if (cachedEntry)
        {
            ASSERT(cachedEntry->ID == assetId);

            // Get actual asset info
            if (Content::GetAssetInfo(assetId, assetInfo))
            {
                // Ensure that cached entry is valid
                if (cachedEntry->TypeName == assetInfo.TypeName)
                {
                    // Check if file hasn't been modified
                    if (FileSystem::GetFileLastEditTime(assetInfo.Path) <= cachedEntry->FileModified)
                    {
                        // Check all dependant files
                        bool isValid = true;
                        for (auto& f : cachedEntry->FileDependencies)
                        {
                            if (FileSystem::GetFileLastEditTime(f.First) > f.Second)
                            {
                                isValid = false;
                                break;
                            }
                        }

                        if (isValid)
                        {
                            // Cache hit!
                            e.Info.TypeName = assetInfo.TypeName;
                            continue;
                        }
                    }
                }
                else
                {
                    // Remove invalid entry
                    cache.Entries.Remove(assetId);
                }
            }
        }

        // Load asset (and keep ref)
        AssetReference<Asset> ref = Content::LoadAsync<Asset>(assetId);
        if (ref == nullptr)
        {
            data.Error(TEXT("Failed to load asset included in build."));
            return true;
        }
        e.Info.TypeName = ref->GetTypeName();

        // Cook asset
        if (Process(data, cache, ref.Get()))
            return true;
        data.Stats.CookedAssets++;

        // Auto save build cache after every few cooked assets (reduces next build time if cooking fails later)
        if (data.Stats.CookedAssets % 50 == 0)
        {
            cache.Save();
        }
    }

    // Save build cache header
    cache.Save();

    // Create build game header
    {
        GameHeaderFlags gameFlags = GameHeaderFlags::None;
        if (!gameSettings->NoSplashScreen)
            gameFlags |= GameHeaderFlags::ShowSplashScreen;

        // Open file
        auto stream = FileWriteStream::Open(data.OutputPath / TEXT("Content/head"));
        if (stream == nullptr)
        {
            data.Error(TEXT("Failed to create game data file."));
            return true;
        }

        stream->WriteInt32(('x' + 'D') * 131); // think about it as '131 times xD'

        stream->WriteInt32(FLAXENGINE_VERSION_BUILD);

        Array<byte> bytes;
        bytes.Resize(808 + sizeof(Guid));
        Platform::MemoryClear(bytes.Get(), bytes.Count());
        int32 length = sizeof(Char) * gameSettings->ProductName.Length();
        Platform::MemoryCopy(bytes.Get() + 0, gameSettings->ProductName.Get(), length);
        bytes[length] = 0;
        bytes[length + 1] = 0;
        length = sizeof(Char) * gameSettings->CompanyName.Length();
        Platform::MemoryCopy(bytes.Get() + 400, gameSettings->CompanyName.Get(), length);
        bytes[length + 400] = 0;
        bytes[length + 401] = 0;
        *(int32*)(bytes.Get() + 800) = (int32)gameFlags;
        *(int32*)(bytes.Get() + 804) = contentKey;
        *(Guid*)(bytes.Get() + 808) = gameSettings->SplashScreen;
        Encryption::EncryptBytes(bytes.Get(), bytes.Count());
        stream->WriteArray(bytes);

        Delete(stream);
    }

    // Package all registered assets into packages
    {
        PackageBuilder packageBuilder(buildSettings->MaxAssetsPerPackage, buildSettings->MaxPackageSizeMB, contentKey);

        subStepIndex = 0;
        for (auto i = AssetsRegistry.Begin(); i.IsNotEnd(); ++i)
        {
            BUILD_STEP_CANCEL_CHECK;

            data.StepProgress(Step2Info, Math::Lerp(Step2ProgressStart, Step2ProgressEnd, static_cast<float>(subStepIndex++) / AssetsRegistry.Count()));
            const auto assetId = i->Key;

            String cookedFilePath;
            cache.GetFilePath(assetId, cookedFilePath);

            if (!FileSystem::FileExists(cookedFilePath))
            {
                LOG(Warning, "Missing cooked file for asset \'{0}\'", assetId);
                continue;
            }

            auto& assetStats = data.Stats.AssetStats[i->Value.Info.TypeName];
            assetStats.Count++;
            assetStats.ContentSize += FileSystem::GetFileSize(cookedFilePath);

            if (packageBuilder.Add(data, i->Value, cookedFilePath))
                return true;
        }
        if (packageBuilder.Package(data))
            return true;
        for (auto& e : data.Stats.AssetStats)
            e.Value.TypeName = e.Key;
        data.Stats.ContentSizeMB = static_cast<int32>(packageBuilder.GetPackagesSizeTotal() / (1024 * 1024));
    }

    BUILD_STEP_CANCEL_CHECK;

    data.StepProgress(TEXT("Creating assets cache"), Step2ProgressEnd);

    // Create asset paths mapping for the assets.
    // Assets mapping is use to convert paths used in Content::Load(path) into the asset id.
    // It fixes the issues when in build game all assets are in the packages and are requested by path.
    // E.g. game settings are loaded from `Content/GameSettings.json` file which is packages in one of the packages.
    // Additionally it improves the in-build assets loading performance (no more registry linear lookup for path by dictionary access).
    for (auto i = data.Assets.Begin(); i.IsNotEnd(); ++i)
    {
        if (Content::GetAssetInfo(i->Item, assetInfo))
        {
            // Use local path relative to the game dir (assets cache is converting them to absolute paths because RelativePaths flag is set)
            String localPath;
            if (assetInfo.Path.StartsWith(Globals::StartupFolder))
                localPath = assetInfo.Path.Right(assetInfo.Path.Length() - Globals::StartupFolder.Length() - 1);
            else if (assetInfo.Path.StartsWith(Globals::ProjectFolder))
                localPath = assetInfo.Path.Right(assetInfo.Path.Length() - Globals::ProjectFolder.Length() - 1);
            else
                localPath = assetInfo.Path;
            AssetPathsMapping[localPath] = assetInfo.ID;
        }
    }

    BUILD_STEP_CANCEL_CHECK;

    // Save assets cache
    if (AssetsCache::Save(data.OutputPath / TEXT("Content/AssetsCache.dat"), AssetsRegistry, AssetPathsMapping, AssetsCacheFlags::RelativePaths))
    {
        data.Error(TEXT("Failed to create assets registry."));
        return true;
    }

    // Print stats
    LOG(Info, "Cooked {0} assets, total assets: {1}, total content packages size: {2} MB", data.Stats.CookedAssets, AssetsRegistry.Count(), data.Stats.ContentSizeMB);
    {
        Array<CookingData::AssetTypeStatistics> assetTypes;
        data.Stats.AssetStats.GetValues(assetTypes);
        Sorting::QuickSort(assetTypes.Get(), assetTypes.Count());

        LOG(Info, "");
        LOG(Info, "Top assets types stats:");
        for (int32 i = 0; i < 10 && i < assetTypes.Count(); i++)
        {
            auto& e = assetTypes[i];
            String typeName;
            const int32 MinLength = 35;
            const int32 lengthDiff = MinLength - e.TypeName.Length();
            if (lengthDiff > 0)
            {
                typeName.ReserveSpace(MinLength);
                for (int32 j = 0; j < e.TypeName.Length(); j++)
                    typeName[j] = e.TypeName[j];
                for (int32 j = 0; j < lengthDiff; j++)
                    typeName[j + e.TypeName.Length()] = ' ';
            }
            else
            {
                typeName = e.TypeName;
            }
            LOG(Info, "{0}: {1:>4} assets of total size {2}", typeName, e.Count, Utilities::BytesToText(e.ContentSize));
        }
        LOG(Info, "");
    }

    return false;
}
