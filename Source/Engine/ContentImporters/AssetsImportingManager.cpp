// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_ASSETS_IMPORTER

#include "AssetsImportingManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Cache/AssetsCache.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Engine/Globals.h"
#include "ImportTexture.h"
#include "ImportModel.h"
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
#include "CreateAnimation.h"
#include "CreateBehaviorTree.h"
#include "CreateJson.h"

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
const String AssetsImportingManager::CreateAnimationTag(TEXT("Animation"));
const String AssetsImportingManager::CreateBehaviorTreeTag(TEXT("BehaviorTree"));
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
bool AssetsImportingManager::UseImportPathRelative = false;

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

    // Skip for non-flax assets (eg. json resource or custom asset type)
    if (!TargetAssetPath.EndsWith(ASSET_FILES_EXTENSION))
        return CreateAssetResult::Ok;

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
    result = FlaxStorage::Create(OutputPath, Data) ? CreateAssetResult::CannotSaveFile : CreateAssetResult::Ok;
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
    Data.Header.Chunks[index] = New<FlaxChunk>();
    return false;
}

void CreateAssetContext::AddMeta(JsonWriter& writer) const
{
    writer.JKEY("ImportPath");
    writer.String(AssetsImportingManager::GetImportPath(InputPath));
    writer.JKEY("ImportUsername");
    writer.String(Platform::GetUserName());
}

void CreateAssetContext::ApplyChanges()
{
    // Get access
    auto storage = ContentStorageManager::TryGetStorage(TargetAssetPath);
    if (storage && storage->IsLoaded())
    {
        if (storage->CloseFileHandles())
        {
            LOG(Error, "Cannot close file access for '{}'", TargetAssetPath);
            _applyChangesResult = CreateAssetResult::CannotSaveFile;
            return;
        }
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
    if (StringView(ASSET_FILES_EXTENSION).Compare(StringView(extension), StringSearchCase::IgnoreCase) == 0)
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

String AssetsImportingManager::GetImportPath(const String& path)
{
    if (UseImportPathRelative && !FileSystem::IsRelative(path)
#if PLATFORM_WINDOWS
        // Import path from other drive should be stored as absolute on Windows to prevent issues
        && path.Length() > 2 && Globals::ProjectFolder.Length() > 2 && path[0] == Globals::ProjectFolder[0]
#endif
    )
    {
        return FileSystem::ConvertAbsolutePathToRelative(Globals::ProjectFolder, path);
    }
    return path;
}

bool AssetsImportingManager::Create(const Function<CreateAssetResult(CreateAssetContext&)>& callback, const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg)
{
    PROFILE_CPU();
    ZoneText(*outputPath, outputPath.Length());
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
            const String outputDirectory = StringUtils::GetDirectoryName(outputPath);
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
        Content::GetRegistry()->RegisterAsset(context.Data.Header, context.TargetAssetPath);

        // Done
        const auto endTime = Platform::GetTimeSeconds();
        LOG(Info, "Asset '{0}' imported in {2}s! {1}", outputPath, context.Data.Header.ToString(), Utilities::RoundTo2DecimalPlaces(endTime - startTime));
    }
    else if (result == CreateAssetResult::Abort)
    {
        // Do nothing
        return true;
    }
    else if (result != CreateAssetResult::Skip)
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
        { TEXT("tga"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("dds"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("png"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("bmp"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("gif"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("tiff"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("tif"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("jpeg"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("jpg"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("hdr"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("raw"), ASSET_FILES_EXTENSION, ImportTexture::Import },
        { TEXT("exr"), ASSET_FILES_EXTENSION, ImportTexture::Import },

        // IES Profiles
        { TEXT("ies"), ASSET_FILES_EXTENSION, ImportTexture::ImportIES },

        // Shaders
        { TEXT("shader"), ASSET_FILES_EXTENSION, ImportShader::Import },

        // Audio
        { TEXT("wav"), ASSET_FILES_EXTENSION, ImportAudio::ImportWav },
        { TEXT("mp3"), ASSET_FILES_EXTENSION, ImportAudio::ImportMp3 },
#if COMPILE_WITH_OGG_VORBIS
        { TEXT("ogg"), ASSET_FILES_EXTENSION, ImportAudio::ImportOgg },
#endif

        // Fonts
        { TEXT("ttf"), ASSET_FILES_EXTENSION, ImportFont::Import },
        { TEXT("otf"), ASSET_FILES_EXTENSION, ImportFont::Import },

        // Models
        { TEXT("obj"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("fbx"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("x"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("dae"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("gltf"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("glb"), ASSET_FILES_EXTENSION, ImportModel::Import },

        // gettext PO files
        { TEXT("po"), TEXT("json"), CreateJson::ImportPo },

        // Models (untested formats - may fail :/)
        { TEXT("blend"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("bvh"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("ase"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("ply"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("dxf"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("ifc"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("nff"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("smd"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("vta"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("mdl"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("md2"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("md3"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("md5mesh"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("q3o"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("q3s"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("ac"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("stl"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("lwo"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("lws"), ASSET_FILES_EXTENSION, ImportModel::Import },
        { TEXT("lxo"), ASSET_FILES_EXTENSION, ImportModel::Import },
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
        { AssetsImportingManager::CreateModelTag, ImportModel::Create },

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
        { AssetsImportingManager::CreateAnimationTag, CreateAnimation::Create },
        { AssetsImportingManager::CreateBehaviorTreeTag, CreateBehaviorTree::Create },
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
