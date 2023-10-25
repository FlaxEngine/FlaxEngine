// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "ImportModel.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Content/Assets/Animation.h"
#include "Engine/Content/Content.h"
#include "Engine/Platform/FileSystem.h"
#include "AssetsImportingManager.h"

bool ImportModelFile::TryGetImportOptions(const StringView& path, Options& options)
{
#if IMPORT_MODEL_CACHE_OPTIONS
    if (FileSystem::FileExists(path))
    {
        // Try to load asset file and asset info
        auto tmpFile = ContentStorageManager::GetStorage(path);
        AssetInitData data;
        if (tmpFile
            && tmpFile->GetEntriesCount() == 1
            && (
                (tmpFile->GetEntry(0).TypeName == Model::TypeName && !tmpFile->LoadAssetHeader(0, data) && data.SerializedVersion >= 4)
                ||
                (tmpFile->GetEntry(0).TypeName == SkinnedModel::TypeName && !tmpFile->LoadAssetHeader(0, data) && data.SerializedVersion >= 1)
                ||
                (tmpFile->GetEntry(0).TypeName == Animation::TypeName && !tmpFile->LoadAssetHeader(0, data) && data.SerializedVersion >= 1)
            ))
        {
            // Check import meta
            rapidjson_flax::Document metadata;
            metadata.Parse((const char*)data.Metadata.Get(), data.Metadata.Length());
            if (metadata.HasParseError() == false)
            {
                options.Deserialize(metadata, nullptr);
                return true;
            }
        }
    }
#endif
    return false;
}

void TryRestoreMaterials(CreateAssetContext& context, ModelData& modelData)
{
    // Skip if file is missing
    if (!FileSystem::FileExists(context.TargetAssetPath))
        return;

    // Try to load asset that gets reimported
    AssetReference<Asset> asset = Content::LoadAsync<Asset>(context.TargetAssetPath);
    if (asset == nullptr)
        return;
    if (asset->WaitForLoaded())
        return;

    // Get model object
    ModelBase* model = nullptr;
    if (asset.Get()->GetTypeName() == Model::TypeName)
    {
        model = ((Model*)asset.Get());
    }
    else if (asset.Get()->GetTypeName() == SkinnedModel::TypeName)
    {
        model = ((SkinnedModel*)asset.Get());
    }
    if (!model)
        return;

    // Peek materials
    for (int32 i = 0; i < modelData.Materials.Count(); i++)
    {
        auto& dstSlot = modelData.Materials[i];

        if (model->MaterialSlots.Count() > i)
        {
            auto& srcSlot = model->MaterialSlots[i];

            dstSlot.Name = srcSlot.Name;
            dstSlot.ShadowsMode = srcSlot.ShadowsMode;
            dstSlot.AssetID = srcSlot.Material.GetID();
        }
    }
}

CreateAssetResult ImportModelFile::Import(CreateAssetContext& context)
{
    // Get import options
    Options options;
    if (context.CustomArg != nullptr)
    {
        // Copy import options from argument
        options = *static_cast<Options*>(context.CustomArg);
    }
    else
    {
        // Restore the previous settings or use default ones
        if (!TryGetImportOptions(context.TargetAssetPath, options))
        {
            LOG(Warning, "Missing model import options. Using default values.");
        }
    }
    if (options.SplitObjects)
    {
        options.OnSplitImport.Bind([&context](Options& splitOptions, const String& objectName)
        {
            // Recursive importing of the split object
            String postFix = objectName;
            const int32 splitPos = postFix.FindLast(TEXT('|'));
            if (splitPos != -1)
                postFix = postFix.Substring(splitPos + 1);
            const String outputPath = String(StringUtils::GetPathWithoutExtension(context.TargetAssetPath)) + TEXT(" ") + postFix + TEXT(".flax");
            return AssetsImportingManager::Import(context.InputPath, outputPath, &splitOptions);
        });
    }

    // Import model file
    ModelData modelData;
    String errorMsg;
    String autoImportOutput(StringUtils::GetDirectoryName(context.TargetAssetPath));
    autoImportOutput /= options.SubAssetFolder.HasChars() ? options.SubAssetFolder.TrimTrailing() : String(StringUtils::GetFileNameWithoutExtension(context.InputPath));
    if (ModelTool::ImportModel(context.InputPath, modelData, options, errorMsg, autoImportOutput))
    {
        LOG(Error, "Cannot import model file. {0}", errorMsg);
        return CreateAssetResult::Error;
    }

    // Check if restore materials on model reimport
    if (options.RestoreMaterialsOnReimport && modelData.Materials.HasItems())
    {
        TryRestoreMaterials(context, modelData);
    }

    // Auto calculate LODs transition settings
    modelData.CalculateLODsScreenSizes();

    // Create destination asset type
    CreateAssetResult result = CreateAssetResult::InvalidTypeID;
    switch (options.Type)
    {
    case ModelTool::ModelType::Model:
        result = ImportModel(context, modelData, &options);
        break;
    case ModelTool::ModelType::SkinnedModel:
        result = ImportSkinnedModel(context, modelData, &options);
        break;
    case ModelTool::ModelType::Animation:
        result = ImportAnimation(context, modelData, &options);
        break;
    }
    if (result != CreateAssetResult::Ok)
        return result;

#if IMPORT_MODEL_CACHE_OPTIONS
    // Create json with import context
    rapidjson_flax::StringBuffer importOptionsMetaBuffer;
    importOptionsMetaBuffer.Reserve(256);
    CompactJsonWriter importOptionsMetaObj(importOptionsMetaBuffer);
    JsonWriter& importOptionsMeta = importOptionsMetaObj;
    importOptionsMeta.StartObject();
    {
        context.AddMeta(importOptionsMeta);
        options.Serialize(importOptionsMeta, nullptr);
    }
    importOptionsMeta.EndObject();
    context.Data.Metadata.Copy((const byte*)importOptionsMetaBuffer.GetString(), (uint32)importOptionsMetaBuffer.GetSize());
#endif

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModelFile::Create(CreateAssetContext& context)
{
    ASSERT(context.CustomArg != nullptr);
    auto& modelData = *(ModelData*)context.CustomArg;

    // Ensure model has any meshes
    if ((modelData.LODs.IsEmpty() || modelData.LODs[0].Meshes.IsEmpty()))
    {
        LOG(Warning, "Models has no valid meshes");
        return CreateAssetResult::Error;
    }

    // Auto calculate LODs transition settings
    modelData.CalculateLODsScreenSizes();

    // Import
    return ImportModel(context, modelData);
}

CreateAssetResult ImportModelFile::ImportModel(CreateAssetContext& context, ModelData& modelData, const Options* options)
{
    // Base
    IMPORT_SETUP(Model, Model::SerializedVersion);

    // Save model header
    MemoryWriteStream stream(4096);
    if (modelData.Pack2ModelHeader(&stream))
        return CreateAssetResult::Error;
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());

    // Pack model LODs data
    const auto lodCount = modelData.GetLODsCount();
    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        stream.SetPosition(0);

        // Pack meshes
        auto& meshes = modelData.LODs[lodIndex].Meshes;
        for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
        {
            if (meshes[meshIndex]->Pack2Model(&stream))
            {
                LOG(Warning, "Cannot pack mesh.");
                return CreateAssetResult::Error;
            }
        }

        const int32 chunkIndex = lodIndex + 1;
        if (context.AllocateChunk(chunkIndex))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[chunkIndex]->Data.Copy(stream.GetHandle(), stream.GetPosition());
    }

    // Generate SDF
    if (options && options->GenerateSDF)
    {
        stream.SetPosition(0);
        if (!ModelTool::GenerateModelSDF(nullptr, &modelData, options->SDFResolution, lodCount - 1, nullptr, &stream, context.TargetAssetPath))
        {
            if (context.AllocateChunk(15))
                return CreateAssetResult::CannotAllocateChunk;
            context.Data.Header.Chunks[15]->Data.Copy(stream.GetHandle(), stream.GetPosition());
        }
    }

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModelFile::ImportSkinnedModel(CreateAssetContext& context, ModelData& modelData, const Options* options)
{
    // Base
    IMPORT_SETUP(SkinnedModel, SkinnedModel::SerializedVersion);

    // Save skinned model header
    MemoryWriteStream stream(4096);
    if (modelData.Pack2SkinnedModelHeader(&stream))
        return CreateAssetResult::Error;
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());

    // Pack model LODs data
    const auto lodCount = modelData.GetLODsCount();
    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        stream.SetPosition(0);

        // Mesh Data Version
        stream.WriteByte(1);

        // Pack meshes
        auto& meshes = modelData.LODs[lodIndex].Meshes;
        for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
        {
            if (meshes[meshIndex]->Pack2SkinnedModel(&stream))
            {
                LOG(Warning, "Cannot pack mesh.");
                return CreateAssetResult::Error;
            }
        }

        const int32 chunkIndex = lodIndex + 1;
        if (context.AllocateChunk(chunkIndex))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[chunkIndex]->Data.Copy(stream.GetHandle(), stream.GetPosition());
    }

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModelFile::ImportAnimation(CreateAssetContext& context, ModelData& modelData, const Options* options)
{
    // Base
    IMPORT_SETUP(Animation, Animation::SerializedVersion);

    // Save animation data
    MemoryWriteStream stream(8182);
    if (modelData.Pack2AnimationHeader(&stream))
        return CreateAssetResult::Error;
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());

    return CreateAssetResult::Ok;
}

#endif
