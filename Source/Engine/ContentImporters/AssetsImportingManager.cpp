// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_ASSETS_IMPORTER

#include "AssetsImportingManager.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Cache/AssetsCache.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Platform.h"
#include "ImportTexture.h"
#include "ImportModelFile.h"
#include "ImportAudio.h"
#include "ImportShader.h"
#include "ImportFont.h"
#include "CreateMaterial.h"
#include "CreateMaterialInstance.h"
#include "CreateRawData.h"
#include "CreateCollisionData.h"
#include "CreateAnimationGraph.h"
#include "CreateSkeletonMask.h"
#include "CreateParticleEmitter.h"
#include "CreateParticleSystem.h"
#include "CreateSceneAnimation.h"
#include "CreateMaterialFunction.h"
#include "CreateParticleEmitterFunction.h"
#include "CreateAnimationGraphFunction.h"
#include "CreateVisualScript.h"

// Tags used to detect asset creation mode
const String AssetsImportingManager::CreateTextureTag(TEXT("Texture"));
const String AssetsImportingManager::CreateTextureAsTextureDataTag(TEXT("TextureAsTextureData"));
const String AssetsImportingManager::CreateTextureAsInitDataTag(TEXT("TextureAsInitData"));
const String AssetsImportingManager::CreateMaterialTag(TEXT("Material"));
const String AssetsImportingManager::CreateMaterialInstanceTag(TEXT("MaterialInstance"));
const String AssetsImportingManager::CreateCubeTextureTag(TEXT("CubeTexture"));
const String AssetsImportingManager::CreateModelTag(TEXT("Model"));
const String AssetsImportingManager::CreateRawDataTag(TEXT("RawData"));
const String AssetsImportingManager::CreateCollisionDataTag(TEXT("CollisionData"));
const String AssetsImportingManager::CreateAnimationGraphTag(TEXT("AnimationGraph"));
const String AssetsImportingManager::CreateSkeletonMaskTag(TEXT("SkeletonMask"));
const String AssetsImportingManager::CreateParticleEmitterTag(TEXT("ParticleEmitter"));
const String AssetsImportingManager::CreateParticleSystemTag(TEXT("ParticleSystem"));
const String AssetsImportingManager::CreateSceneAnimationTag(TEXT("SceneAnimation"));
const String AssetsImportingManager::CreateMaterialFunctionTag(TEXT("MaterialFunction"));
const String AssetsImportingManager::CreateParticleEmitterFunctionTag(TEXT("ParticleEmitterFunction"));
const String AssetsImportingManager::CreateAnimationGraphFunctionTag(TEXT("AnimationGraphFunction"));
const String AssetsImportingManager::CreateVisualScriptTag(TEXT("VisualScript"));

class AssetsImportingManagerService : public EngineService
{
public:

    AssetsImportingManagerService()
        : EngineService(TEXT("AssetsImportingManager"), -400)
    {
    }

    bool Init() override;
    void Dispose() override;
};

AssetsImportingManagerService AssetsImportingManagerServiceInstance;

Array<AssetImporter> AssetsImportingManager::Importers;
Array<AssetCreator> AssetsImportingManager::Creators;

CreateAssetContext::CreateAssetContext(const StringView& inputPath, const StringView& outputPath, const Guid& id, void* arg)
{
    InputPath = inputPath;
    TargetAssetPath = outputPath;
    CustomArg = arg;
    Data.Header.ID = id;
    SkipMetadata = false;

    // TODO: we should use ASNI only chars path (Assimp can use only that kind)
    OutputPath = Content::CreateTemporaryAssetPath();
}

CreateAssetResult CreateAssetContext::Run(const CreateAssetFunction& callback)
{
    ASSERT(callback.IsBinded());

    // Call action
    auto result = callback(*this);
    if (result != CreateAssetResult::Ok)
        return result;

    // Validate assigned TypeID
    if (Data.Header.TypeName.IsEmpty())
    {
        LOG(Warning, "Assigned asset TypeName is invalid.");
        return CreateAssetResult::InvalidTypeID;
    }

    // Add import metadata to the file (if it's empty)
    if (!SkipMetadata && Data.Metadata.IsInvalid())
    {
        rapidjson_flax::StringBuffer buffer;
        CompactJsonWriter writer(buffer);
        writer.StartObject();
        {
            AddMeta(writer);
        }
        writer.EndObject();
        Data.Metadata.Copy((const byte*)buffer.GetString(), (uint32)buffer.GetSize());
    }

    // Save file
    result = Save();

    if (result == CreateAssetResult::Ok)
    {
        _applyChangesResult = CreateAssetResult::Abort;
        INVOKE_ON_MAIN_THREAD(CreateAssetContext, CreateAssetContext::ApplyChanges, this);
        result = _applyChangesResult;
    }
    FileSystem::DeleteFile(OutputPath);

    return result;
}

bool CreateAssetContext::AllocateChunk(int32 index)
{
    // Check index
    if (index < 0 || index >= ASSET_FILE_DATA_CHUNKS)
    {
        LOG(Warning, "Invalid asset chunk index {0}.", index);
        return true;
    }

    // Check if chunk has been already allocated
    if (Data.Header.Chunks[index] != nullptr)
    {
        LOG(Warning, "Asset chunk {0} has been already allocated.", index);
        return true;
    }

    // Create new chunk
    const auto chunk = New<FlaxChunk>();
    Data.Header.Chunks[index] = chunk;

    if (chunk == nullptr)
    {
        OUT_OF_MEMORY;
    }

    return false;
}

void CreateAssetContext::AddMeta(JsonWriter& writer) const
{
    writer.JKEY("ImportPath");
    writer.String(InputPath);
    writer.JKEY("ImportUsername");
    writer.String(Platform::GetUserName());
}

CreateAssetResult CreateAssetContext::Save()
{
    return FlaxStorage::Create(OutputPath, Data) ? CreateAssetResult::CannotSaveFile : CreateAssetResult::Ok;
}

void CreateAssetContext::ApplyChanges()
{
    // Get access
    auto storage = ContentStorageManager::TryGetStorage(TargetAssetPath);
    if (storage && storage->IsLoaded())
    {
        storage->CloseFileHandles();
    }

    // Move file
    if (FileSystem::MoveFile(TargetAssetPath, OutputPath, true))
    {
        LOG(Warning, "Cannot move imported file {0} to the destination path {1}.", OutputPath, TargetAssetPath);
        _applyChangesResult = CreateAssetResult::CannotSaveFile;
        return;
    }

    // Reload (any asset using it will receive OnStorageReloaded event and handle it)
    if (storage)
    {
        storage->Reload();
    }

    _applyChangesResult = CreateAssetResult::Ok;
}

const AssetImporter* AssetsImportingManager::GetImporter(const String& extension)
{
    for (int32 i = 0; i < Importers.Count(); i++)
    {
        if (Importers[i].FileExtension.Compare(extension, StringSearchCase::IgnoreCase) == 0)
            return &Importers[i];
    }
    return nullptr;
}

const AssetCreator* AssetsImportingManager::GetCreator(const String& tag)
{
    for (int32 i = 0; i < Creators.Count(); i++)
    {
        if (Creators[i].Tag == tag)
            return &Creators[i];
    }
    return nullptr;
}

bool AssetsImportingManager::Create(const CreateAssetFunction& importFunc, const StringView& outputPath, Guid& assetId, void* arg)
{
    return Create(importFunc, StringView::Empty, outputPath, assetId, arg);
}

bool AssetsImportingManager::Create(const String& tag, const StringView& outputPath, Guid& assetId, void* arg)
{
    const auto creator = GetCreator(tag);
    if (creator == nullptr)
    {
        LOG(Warning, "Cannot find asset creator object for tag \'{0}\'.", tag);
        return true;
    }

    return Create(creator->Callback, outputPath, assetId, arg);
}

bool AssetsImportingManager::Import(const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg)
{
    ASSERT(outputPath.EndsWith(StringView(ASSET_FILES_EXTENSION)));

    LOG(Info, "Importing file '{0}' to '{1}'...", inputPath, outputPath);

    // Check if input file exists
    if (!FileSystem::FileExists(inputPath))
    {
        LOG(Error, "Missing file '{0}'", inputPath);
        return true;
    }

    // Get file extension and try to find import function for it
    const String extension = FileSystem::GetExtension(inputPath).ToLower();

    // Special case for raw assets
    const String assetExtension = ASSET_FILES_EXTENSION;
    if (assetExtension.Compare(extension, StringSearchCase::IgnoreCase) == 0)
    {
        // Simply copy file (content layer will resolve duplicated IDs, etc.)
        return FileSystem::CopyFile(outputPath, inputPath);
    }

    // Find valid importer for that file
    const auto importer = GetImporter(extension);
    if (importer == nullptr)
    {
        LOG(Error, "Cannot import file \'{0}\'. Unknown file type.", inputPath);
        return true;
    }

    return Create(importer->Callback, inputPath, outputPath, assetId, arg);
}

bool AssetsImportingManager::ImportIfEdited(const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg)
{
    ASSERT(outputPath.EndsWith(StringView(ASSET_FILES_EXTENSION)));

    // Check if asset not exists
    if (!FileSystem::FileExists(outputPath))
    {
        return Import(inputPath, outputPath, assetId, arg);
    }

    // Check if need to reimport file (it could be checked via asset header but this way is faster and works in general)
    const DateTime sourceEdited = FileSystem::GetFileLastEditTime(inputPath);
    const DateTime assetEdited = FileSystem::GetFileLastEditTime(outputPath);
    if (sourceEdited > assetEdited)
    {
        return Import(inputPath, outputPath, assetId, arg);
    }

    // No import
    if (!assetId.IsValid())
    {
        AssetInfo assetInfo;
        Content::GetAssetInfo(outputPath, assetInfo);
        assetId = assetInfo.ID;
    }
    return false;
}

bool AssetsImportingManager::Create(const Function<CreateAssetResult(CreateAssetContext&)>& callback, const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg)
{
    const auto startTime = Platform::GetTimeSeconds();

    // Pick ID if not specified
    if (!assetId.IsValid())
        assetId = Guid::New();

    // Check if asset at target path is loaded
    AssetReference<Asset> asset = Content::GetAsset(outputPath);
    if (asset)
    {
        // Use the same ID
        assetId = asset->GetID();
    }
    else
    {
        // Check if asset already exists and isn't empty
        if (FileSystem::FileExists(outputPath) && FileSystem::GetFileSize(outputPath) > 0)
        {
            // Load storage container
            const auto storage = ContentStorageManager::GetStorage(outputPath);
            if (storage)
            {
                // Try to load old asset header and then use old ID
                Array<FlaxStorage::Entry> e;
                storage->GetEntries(e);
                if (e.Count() == 1)
                {
                    // Override asset id (use the old value)
                    assetId = e[0].ID;
                    LOG(Info, "Asset already exists. Using old ID: {0}", assetId);
                }
                else
                {
                    LOG(Warning, "File {0} is a package.", outputPath);
                }
            }
            else
            {
                LOG(Warning, "Cannot open storage container at {0}", outputPath);
            }
        }
        else
        {
            // Ensure that path exists
            const String outputDirectory = StringUtils::GetDirectoryName(*outputPath);
            if (FileSystem::CreateDirectory(outputDirectory))
            {
                LOG(Warning, "Cannot create directory '{0}'", outputDirectory);
            }
        }
    }

    // Import file
    CreateAssetContext context(inputPath, outputPath, assetId, arg);
    const auto result = context.Run(callback);

    // Clear reference
    asset = nullptr;

    // Switch result
    if (result == CreateAssetResult::Ok)
    {
        // Register asset
        Content::GetRegistry()->RegisterAsset(context.Data.Header, outputPath);

        // Done
        const auto endTime = Platform::GetTimeSeconds();
        LOG(Info, "Asset '{0}' imported in {2}s! {1}", outputPath, context.Data.Header.ToString(), Utilities::RoundTo2DecimalPlaces(endTime - startTime));
    }
    else if (result == CreateAssetResult::Abort)
    {
        // Do nothing
        return true;
    }
    else
    {
        LOG(Error, "Cannot import file '{0}'! Result: {1}", inputPath, ::ToString(result));
        return true;
    }

    return false;
}

bool AssetsImportingManagerService::Init()
{
    // Initialize with in-build importers
    AssetImporter InBuildImporters[] =
    {
        // Textures and Cube Textures
        { TEXT("tga"), ImportTexture::Import },
        { TEXT("dds"), ImportTexture::Import },
        { TEXT("png"), ImportTexture::Import },
        { TEXT("bmp"), ImportTexture::Import },
        { TEXT("gif"), ImportTexture::Import },
        { TEXT("tiff"), ImportTexture::Import },
        { TEXT("tif"), ImportTexture::Import },
        { TEXT("jpeg"), ImportTexture::Import },
        { TEXT("jpg"), ImportTexture::Import },
        { TEXT("hdr"), ImportTexture::Import },
        { TEXT("raw"), ImportTexture::Import },

        // IES Profiles
        { TEXT("ies"), ImportTexture::ImportIES },

        // Shaders
        { TEXT("shader"), ImportShader::Import },

        // Audio
        { TEXT("wav"), ImportAudio::ImportWav },
        { TEXT("mp3"), ImportAudio::ImportMp3 },
#if COMPILE_WITH_OGG_VORBIS
        { TEXT("ogg"), ImportAudio::ImportOgg },
#endif

        // Fonts
        { TEXT("ttf"), ImportFont::Import },
        { TEXT("otf"), ImportFont::Import },

        // Models
        { TEXT("obj"), ImportModelFile::Import },
        { TEXT("fbx"), ImportModelFile::Import },
        { TEXT("x"), ImportModelFile::Import },
        { TEXT("dae"), ImportModelFile::Import },
        { TEXT("gltf"), ImportModelFile::Import },
        { TEXT("glb"), ImportModelFile::Import },

        // Models (untested formats - may fail :/)
        { TEXT("blend"), ImportModelFile::Import },
        { TEXT("bvh"), ImportModelFile::Import },
        { TEXT("ase"), ImportModelFile::Import },
        { TEXT("ply"), ImportModelFile::Import },
        { TEXT("dxf"), ImportModelFile::Import },
        { TEXT("ifc"), ImportModelFile::Import },
        { TEXT("nff"), ImportModelFile::Import },
        { TEXT("smd"), ImportModelFile::Import },
        { TEXT("vta"), ImportModelFile::Import },
        { TEXT("mdl"), ImportModelFile::Import },
        { TEXT("md2"), ImportModelFile::Import },
        { TEXT("md3"), ImportModelFile::Import },
        { TEXT("md5mesh"), ImportModelFile::Import },
        { TEXT("q3o"), ImportModelFile::Import },
        { TEXT("q3s"), ImportModelFile::Import },
        { TEXT("ac"), ImportModelFile::Import },
        { TEXT("stl"), ImportModelFile::Import },
        { TEXT("lwo"), ImportModelFile::Import },
        { TEXT("lws"), ImportModelFile::Import },
        { TEXT("lxo"), ImportModelFile::Import },
    };
    AssetsImportingManager::Importers.Add(InBuildImporters, ARRAY_COUNT(InBuildImporters));

    // Initialize with in-build creators
    AssetCreator InBuildCreators[] =
    {
        // Textures
        { AssetsImportingManager::CreateTextureTag, ImportTexture::Import },
        { AssetsImportingManager::CreateTextureAsTextureDataTag, ImportTexture::ImportAsTextureData },
        { AssetsImportingManager::CreateTextureAsInitDataTag, ImportTexture::ImportAsInitData },
        { AssetsImportingManager::CreateCubeTextureTag, ImportTexture::ImportCube },

        // Materials
        { AssetsImportingManager::CreateMaterialTag, CreateMaterial::Create },
        { AssetsImportingManager::CreateMaterialInstanceTag, CreateMaterialInstance::Create },

        // Models
        { AssetsImportingManager::CreateModelTag, ImportModelFile::Create },

        // Other
        { AssetsImportingManager::CreateRawDataTag, CreateRawData::Create },
        { AssetsImportingManager::CreateCollisionDataTag, CreateCollisionData::Create },
        { AssetsImportingManager::CreateAnimationGraphTag, CreateAnimationGraph::Create },
        { AssetsImportingManager::CreateSkeletonMaskTag, CreateSkeletonMask::Create },
        { AssetsImportingManager::CreateParticleEmitterTag, CreateParticleEmitter::Create },
        { AssetsImportingManager::CreateParticleSystemTag, CreateParticleSystem::Create },
        { AssetsImportingManager::CreateSceneAnimationTag, CreateSceneAnimation::Create },
        { AssetsImportingManager::CreateMaterialFunctionTag, CreateMaterialFunction::Create },
        { AssetsImportingManager::CreateParticleEmitterFunctionTag, CreateParticleEmitterFunction::Create },
        { AssetsImportingManager::CreateAnimationGraphFunctionTag, CreateAnimationGraphFunction::Create },
        { AssetsImportingManager::CreateVisualScriptTag, CreateVisualScript::Create },
    };
    AssetsImportingManager::Creators.Add(InBuildCreators, ARRAY_COUNT(InBuildCreators));

    return false;
}

void AssetsImportingManagerService::Dispose()
{
    // Cleanup
    AssetsImportingManager::Importers.Clear();
    AssetsImportingManager::Creators.Clear();
}

#endif
