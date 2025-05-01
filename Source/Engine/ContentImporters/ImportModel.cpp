// Copyright (c) Wojciech Figat. All rights reserved.

#include "ImportModel.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Log.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Collections/ArrayExtensions.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Content/Assets/Animation.h"
#include "Engine/Content/Content.h"
#include "Engine/Animations/AnimEvent.h"
#include "Engine/Level/Actors/EmptyActor.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Level/Scripts/ModelPrefab.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Utilities/RectPack.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "AssetsImportingManager.h"

bool ImportModel::TryGetImportOptions(const StringView& path, Options& options)
{
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
    return false;
}

struct PrefabObject
{
    int32 NodeIndex;
    String Name;
    String AssetPath;
};

void RepackMeshLightmapUVs(ModelData& data)
{
    // Use weight-based coordinates space placement and rect-pack to allocate more space for bigger meshes in the model lightmap chart
    int32 lodIndex = 0;
    auto& lod = data.LODs[lodIndex];

    // Build list of meshes with their area
    struct LightmapUVsPack : RectPackNode<float>
    {
        LightmapUVsPack(Size x, Size y, Size width, Size height)
            : RectPackNode(x, y, width, height)
        {
        }

        void OnInsert()
        {
        }
    };
    struct MeshEntry
    {
        MeshData* Mesh;
        float Area;
        float Size;
        LightmapUVsPack* Slot;
    };
    Array<MeshEntry> entries;
    entries.Resize(lod.Meshes.Count());
    float areaSum = 0;
    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
    {
        auto& entry = entries[meshIndex];
        entry.Mesh = lod.Meshes[meshIndex];
        entry.Area = entry.Mesh->CalculateTrianglesArea();
        entry.Size = Math::Sqrt(entry.Area);
        areaSum += entry.Area;
    }

    if (areaSum > ZeroTolerance)
    {
        // Pack all surfaces into atlas
        float atlasSize = Math::Sqrt(areaSum) * 1.02f;
        int32 triesLeft = 10;
        while (triesLeft--)
        {
            bool failed = false;
            const float chartsPadding = (4.0f / 256.0f) * atlasSize;
            RectPackAtlas<LightmapUVsPack> atlas;
            atlas.Init(atlasSize, atlasSize, chartsPadding);
            for (auto& entry : entries)
            {
                entry.Slot = atlas.Insert(entry.Size, entry.Size);
                if (entry.Slot == nullptr)
                {
                    // Failed to insert surface, increase atlas size and try again
                    atlasSize *= 1.5f;
                    failed = true;
                    break;
                }
            }

            if (!failed)
            {
                // Transform meshes lightmap UVs into the slots in the whole atlas
                const float atlasSizeInv = 1.0f / atlasSize;
                for (const auto& entry : entries)
                {
                    Float2 uvOffset(entry.Slot->X * atlasSizeInv, entry.Slot->Y * atlasSizeInv);
                    Float2 uvScale(entry.Slot->Width * atlasSizeInv, entry.Slot->Height * atlasSizeInv);
                    if (entry.Mesh->LightmapUVsIndex == -1)
                        continue;
                    auto& lightmapUVs = entry.Mesh->UVs[entry.Mesh->LightmapUVsIndex];
                    for (auto& uv : lightmapUVs)
                    {
                        uv = uv * uvScale + uvOffset;
                    }
                }
                break;
            }
        }
    }
}

void SetupMaterialSlots(ModelData& data, const Array<MaterialSlotEntry>& materials)
{
    Array<int32> materialSlotsTable;
    materialSlotsTable.Resize(materials.Count());
    materialSlotsTable.SetAll(-1);
    for (auto& lod : data.LODs)
    {
        for (MeshData* mesh : lod.Meshes)
        {
            int32 newSlotIndex = materialSlotsTable[mesh->MaterialSlotIndex];
            if (newSlotIndex == -1)
            {
                newSlotIndex = data.Materials.Count();
                data.Materials.AddOne() = materials[mesh->MaterialSlotIndex];
            }
            mesh->MaterialSlotIndex = newSlotIndex;
        }
    }
}

bool SortMeshGroups(IGrouping<StringView, MeshData*> const& i1, IGrouping<StringView, MeshData*> const& i2)
{
    return i1.GetKey().Compare(i2.GetKey()) < 0;
}

void CloneObject(rapidjson_flax::StringBuffer& buffer, SceneObject* src, SceneObject* dst, bool stripName = false)
{
    // Serialize source
    buffer.Clear();
    CompactJsonWriter writer(buffer);
    writer.StartObject();
    const void* defaultInstance = src->GetType().GetDefaultInstance();
    src->Serialize(writer, defaultInstance);
    writer.EndObject();

    // Parse json
    rapidjson_flax::Document document;
    document.Parse(buffer.GetString(), buffer.GetSize());

    // Strip unwanted data
    document.RemoveMember("ID");
    document.RemoveMember("ParentID");
    document.RemoveMember("PrefabID");
    document.RemoveMember("PrefabObjectID");
    if (stripName)
        document.RemoveMember("Name");

    // Deserialize destination
    auto modifier = Cache::ISerializeModifier.Get();
    dst->Deserialize(document, &*modifier);
}

CreateAssetResult ImportModel::Import(CreateAssetContext& context)
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

    // Import model file
    ModelData* data = options.Cached ? options.Cached->Data : nullptr;
    ModelData dataThis;
    Array<IGrouping<StringView, MeshData*>>* meshesByNamePtr = options.Cached ? (Array<IGrouping<StringView, MeshData*>>*)options.Cached->MeshesByName : nullptr;
    Array<IGrouping<StringView, MeshData*>> meshesByNameThis;
    String autoImportOutput;
    if (!data)
    {
        String errorMsg;
        autoImportOutput = StringUtils::GetDirectoryName(context.TargetAssetPath);
        autoImportOutput /= options.SubAssetFolder.HasChars() ? options.SubAssetFolder.TrimTrailing() : String(StringUtils::GetFileNameWithoutExtension(context.InputPath));
        if (ModelTool::ImportModel(context.InputPath, dataThis, options, errorMsg, autoImportOutput))
        {
            LOG(Error, "Cannot import model file. {0}", errorMsg);
            return CreateAssetResult::Error;
        }
        data = &dataThis;

        // Group meshes by the name (the same mesh name can be used by multiple meshes that use different materials)
        if (data->LODs.Count() != 0)
        {
            const Function<StringView(MeshData* const&)> f = [](MeshData* const& x) -> StringView
            {
                return x->Name;
            };
            ArrayExtensions::GroupBy(data->LODs[0].Meshes, f, meshesByNameThis);
            Sorting::QuickSort(meshesByNameThis.Get(), meshesByNameThis.Count(), &SortMeshGroups);
        }
        meshesByNamePtr = &meshesByNameThis;
    }
    Array<IGrouping<StringView, MeshData*>>& meshesByName = *meshesByNamePtr;

    // Import objects from file separately
    ModelTool::Options::CachedData cached = { data, (void*)meshesByNamePtr };
    Array<PrefabObject> prefabObjects;
    if (options.Type == ModelTool::ModelType::Prefab)
    {
        // Normalize options
        options.SplitObjects = false;
        options.ObjectIndex = -1;

        // Import all of the objects recursive but use current model data to skip loading file again
        options.Cached = &cached;
        Function<bool(Options& splitOptions, const StringView& objectName, String& outputPath, MeshData* meshData)> splitImport = [&context, &autoImportOutput](Options& splitOptions, const StringView& objectName, String& outputPath, MeshData* meshData)
        {
            // Recursive importing of the split object
            String postFix = objectName;
            const int32 splitPos = postFix.FindLast(TEXT('|'));
            if (splitPos != -1)
                postFix = postFix.Substring(splitPos + 1);
            // TODO: check for name collisions with material/texture assets
            outputPath = autoImportOutput / String(StringUtils::GetFileNameWithoutExtension(context.TargetAssetPath)) + TEXT(" ") + postFix + TEXT(".flax");
            splitOptions.SubAssetFolder = TEXT(" "); // Use the same folder as asset as they all are imported to the subdir for the prefab (see SubAssetFolder usage above)

            if (splitOptions.Type == ModelTool::ModelType::Model && meshData)
            {
                // These settings interfere with submesh reimporting
                splitOptions.CenterGeometry = false;
                splitOptions.UseLocalOrigin = false;

                // This properly sets the transformation of the mesh during reimport
                auto* nodes = &splitOptions.Cached->Data->Nodes;
                Vector3 scale = Vector3::One;

                // TODO: Improve this hack.
                // This is the same hack as in ImportModel::CreatePrefab(), and it is documented further there
                auto* currentNode = &(*nodes)[meshData->NodeIndex];
                while (true)
                {
                    if (currentNode->ParentIndex == -1)
                    {
                        scale *= currentNode->LocalTransform.Scale;
                        break;
                    }
                    currentNode = &(*nodes)[currentNode->ParentIndex];
                }

                splitOptions.Translation = meshData->OriginTranslation * scale * -1.0f;
            }

            return AssetsImportingManager::Import(context.InputPath, outputPath, &splitOptions);
        };
        auto splitOptions = options;
        LOG(Info, "Splitting imported {0} meshes", meshesByName.Count());
        PrefabObject prefabObject;
        for (int32 groupIndex = 0; groupIndex < meshesByName.Count(); groupIndex++)
        {
            auto& group = meshesByName[groupIndex];

            // Cache object options (nested sub-object import removes the meshes)
            prefabObject.NodeIndex = group.First()->NodeIndex;
            prefabObject.Name = group.First()->Name;

            splitOptions.Type = ModelTool::ModelType::Model;
            splitOptions.ObjectIndex = groupIndex;
            if (!splitImport(splitOptions, group.GetKey(), prefabObject.AssetPath, group.First()))
            {
                prefabObjects.Add(prefabObject);
            }
        }
        LOG(Info, "Splitting imported {0} animations", data->Animations.Count());
        for (int32 i = 0; i < data->Animations.Count(); i++)
        {
            auto& animation = data->Animations[i];
            splitOptions.Type = ModelTool::ModelType::Animation;
            splitOptions.ObjectIndex = i;
            splitImport(splitOptions, animation.Name, prefabObject.AssetPath, nullptr);
        }
    }
    else if (options.SplitObjects)
    {
        // Import the first object within this call
        options.SplitObjects = false;
        options.ObjectIndex = 0;

        // Import rest of the objects recursive but use current model data to skip loading file again
        options.Cached = &cached;
        Function<bool(Options& splitOptions, const StringView& objectName)> splitImport;
        splitImport.Bind([&context](Options& splitOptions, const StringView& objectName)
        {
            // Recursive importing of the split object
            String postFix = objectName;
            const int32 splitPos = postFix.FindLast(TEXT('|'));
            if (splitPos != -1)
                postFix = postFix.Substring(splitPos + 1);
            const String outputPath = String(StringUtils::GetPathWithoutExtension(context.TargetAssetPath)) + TEXT(" ") + postFix + TEXT(".flax");
            return AssetsImportingManager::Import(context.InputPath, outputPath, &splitOptions);
        });
        auto splitOptions = options;
        switch (options.Type)
        {
        case ModelTool::ModelType::Model:
        case ModelTool::ModelType::SkinnedModel:
            LOG(Info, "Splitting imported {0} meshes", meshesByName.Count());
            for (int32 groupIndex = 1; groupIndex < meshesByName.Count(); groupIndex++)
            {
                auto& group = meshesByName[groupIndex];
                splitOptions.ObjectIndex = groupIndex;
                splitImport(splitOptions, group.GetKey());
            }
            break;
        case ModelTool::ModelType::Animation:
            LOG(Info, "Splitting imported {0} animations", data->Animations.Count());
            for (int32 i = 1; i < data->Animations.Count(); i++)
            {
                auto& animation = data->Animations[i];
                splitOptions.ObjectIndex = i;
                splitImport(splitOptions, animation.Name);
            }
            break;
        }
    }

    // When importing a single object as model asset then select a specific mesh group
    Array<MeshData*> meshesToDelete;
    if (options.ObjectIndex >= 0 &&
        options.ObjectIndex < meshesByName.Count() &&
        (options.Type == ModelTool::ModelType::Model || options.Type == ModelTool::ModelType::SkinnedModel))
    {
        auto& group = meshesByName[options.ObjectIndex];
        if (&dataThis == data)
        {
            // Use meshes only from the grouping (others will be removed manually)
            {
                auto& lod = dataThis.LODs[0];
                meshesToDelete.Add(lod.Meshes);
                lod.Meshes.Clear();
                for (MeshData* mesh : group)
                {
                    lod.Meshes.Add(mesh);
                    meshesToDelete.Remove(mesh);
                }
            }
            for (int32 lodIndex = 1; lodIndex < dataThis.LODs.Count(); lodIndex++)
            {
                auto& lod = dataThis.LODs[lodIndex];
                Array<MeshData*> lodMeshes = lod.Meshes;
                lod.Meshes.Clear();
                for (MeshData* lodMesh : lodMeshes)
                {
                    if (lodMesh->Name == group.GetKey())
                        lod.Meshes.Add(lodMesh);
                    else
                        meshesToDelete.Add(lodMesh);
                }
            }

            // Use only materials references by meshes from the first grouping
            {
                auto materials = dataThis.Materials;
                dataThis.Materials.Clear();
                SetupMaterialSlots(dataThis, materials);
            }
        }
        else
        {
            // Copy data from others data
            dataThis.Skeleton = data->Skeleton;
            dataThis.Nodes = data->Nodes;

            // Move meshes from this group (including any LODs of them)
            {
                auto& lod = dataThis.LODs.AddOne();
                lod.ScreenSize = data->LODs[0].ScreenSize;
                lod.Meshes.Add(group);
                for (MeshData* mesh : group)
                    data->LODs[0].Meshes.Remove(mesh);
            }
            for (int32 lodIndex = 1; lodIndex < data->LODs.Count(); lodIndex++)
            {
                Array<MeshData*> lodMeshes = data->LODs[lodIndex].Meshes;
                for (int32 i = lodMeshes.Count() - 1; i >= 0; i--)
                {
                    MeshData* lodMesh = lodMeshes[i];
                    if (lodMesh->Name == group.GetKey())
                        data->LODs[lodIndex].Meshes.Remove(lodMesh);
                    else
                        lodMeshes.RemoveAtKeepOrder(i);
                }
                if (lodMeshes.Count() == 0)
                    break; // No meshes of that name in this LOD so skip further ones
                auto& lod = dataThis.LODs.AddOne();
                lod.ScreenSize = data->LODs[lodIndex].ScreenSize;
                lod.Meshes.Add(lodMeshes);
            }

            // Copy materials used by the meshes
            SetupMaterialSlots(dataThis, data->Materials);
        }
        data = &dataThis;
    }

    // Check if restore local changes on asset reimport
    constexpr bool RestoreAnimEventsOnReimport = true;
    const bool restoreMaterials = options.RestoreMaterialsOnReimport && data->Materials.HasItems();
    const bool restoreAnimEvents = RestoreAnimEventsOnReimport && options.Type == ModelTool::ModelType::Animation && data->Animations.HasItems();
    if ((restoreMaterials || restoreAnimEvents) && FileSystem::FileExists(context.TargetAssetPath))
    {
        AssetReference<Asset> asset = Content::LoadAsync<Asset>(context.TargetAssetPath);
        if (asset && !asset->WaitForLoaded())
        {
            auto* model = ScriptingObject::Cast<ModelBase>(asset);
            auto* animation = ScriptingObject::Cast<Animation>(asset);
            if (restoreMaterials && model)
            {
                // Copy material settings
                for (int32 i = 0; i < data->Materials.Count(); i++)
                {
                    auto& dstSlot = data->Materials[i];
                    if (model->MaterialSlots.Count() > i)
                    {
                        auto& srcSlot = model->MaterialSlots[i];
                        dstSlot.Name = srcSlot.Name;
                        dstSlot.ShadowsMode = srcSlot.ShadowsMode;
                        dstSlot.AssetID = srcSlot.Material.GetID();
                    }
                }
            }
            if (restoreAnimEvents && animation)
            {
                // Copy anim event tracks
                for (const auto& e : animation->Events)
                {
                    auto& clone = data->Animations[0].Events.AddOne();
                    clone.First = e.First;
                    const auto& eKeys = e.Second.GetKeyframes();
                    auto& cloneKeys = clone.Second.GetKeyframes();
                    clone.Second.Resize(eKeys.Count());
                    for (int32 i = 0; i < eKeys.Count(); i++)
                    {
                        const auto& eKey = eKeys[i];
                        auto& cloneKey = cloneKeys[i];
                        cloneKey.Time = eKey.Time;
                        cloneKey.Value.Duration = eKey.Value.Duration;
                        if (eKey.Value.Instance)
                        {
                            cloneKey.Value.TypeName = eKey.Value.Instance->GetType().Fullname;
                            rapidjson_flax::StringBuffer buffer;
                            CompactJsonWriter writer(buffer);
                            writer.StartObject();
                            eKey.Value.Instance->Serialize(writer, nullptr);
                            writer.EndObject();
                            cloneKey.Value.JsonData.Set(buffer.GetString(), (int32)buffer.GetSize());
                        }
                    }
                }
            }
        }
    }

    // When using generated lightmap UVs those coordinates needs to be moved so all meshes are in unique locations in [0-1]x[0-1] coordinates space
    // Model importer generates UVs in [0-1] space for each mesh so now we need to pack them inside the whole model (only when using multiple meshes)
    if (options.Type == ModelTool::ModelType::Model && options.LightmapUVsSource == ModelLightmapUVsSource::Generate && data->LODs.HasItems() && data->LODs[0].Meshes.Count() > 1)
    {
        RepackMeshLightmapUVs(*data);
    }

    // Create destination asset type
    CreateAssetResult result = CreateAssetResult::InvalidTypeID;
    switch (options.Type)
    {
    case ModelTool::ModelType::Model:
        result = CreateModel(context, *data, &options);
        break;
    case ModelTool::ModelType::SkinnedModel:
        result = CreateSkinnedModel(context, *data, &options);
        break;
    case ModelTool::ModelType::Animation:
        result = CreateAnimation(context, *data, &options);
        break;
    case ModelTool::ModelType::Prefab:
        result = CreatePrefab(context, *data, options, prefabObjects);
        break;
    }
    for (auto mesh : meshesToDelete)
        Delete(mesh);
    if (result != CreateAssetResult::Ok)
        return result;

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

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModel::Create(CreateAssetContext& context)
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

    return CreateModel(context, modelData);
}

CreateAssetResult ImportModel::CreateModel(CreateAssetContext& context, const ModelData& modelData, const Options* options)
{
    PROFILE_CPU();
    IMPORT_SETUP(Model, Model::SerializedVersion);
    static_assert(Model::SerializedVersion == 30, "Update code.");

    // Save model header
    MemoryWriteStream stream(4096);
    if (Model::SaveHeader(stream, modelData))
        return CreateAssetResult::Error;
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

    // Pack model LODs data
    const auto lodCount = modelData.LODs.Count();
    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        stream.SetPosition(0);
        if (Model::SaveLOD(stream, modelData, lodIndex))
            return CreateAssetResult::Error;
        const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
        if (context.AllocateChunk(chunkIndex))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[chunkIndex]->Data.Copy(ToSpan(stream));
    }

    // Generate SDF
    if (options && options->GenerateSDF)
    {
        stream.SetPosition(0);
        if (!ModelTool::GenerateModelSDF(nullptr, &modelData, options->SDFResolution, lodCount - 1, nullptr, &stream, context.TargetAssetPath))
        {
            if (context.AllocateChunk(15))
                return CreateAssetResult::CannotAllocateChunk;
            context.Data.Header.Chunks[15]->Data.Copy(ToSpan(stream));
        }
    }

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModel::CreateSkinnedModel(CreateAssetContext& context, const ModelData& modelData, const Options* options)
{
    PROFILE_CPU();
    IMPORT_SETUP(SkinnedModel, SkinnedModel::SerializedVersion);
    static_assert(SkinnedModel::SerializedVersion == 30, "Update code.");

    // Save skinned model header
    MemoryWriteStream stream(4096);
    if (SkinnedModel::SaveHeader(stream, modelData))
        return CreateAssetResult::Error;
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

    // Pack model LODs data
    const auto lodCount = modelData.LODs.Count();
    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        stream.SetPosition(0);
        if (SkinnedModel::SaveLOD(stream, modelData, lodIndex, SkinnedModel::SaveMesh))
            return CreateAssetResult::Error;
        const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
        if (context.AllocateChunk(chunkIndex))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[chunkIndex]->Data.Copy(ToSpan(stream));
    }

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModel::CreateAnimation(CreateAssetContext& context, const ModelData& modelData, const Options* options)
{
    PROFILE_CPU();
    IMPORT_SETUP(Animation, Animation::SerializedVersion);
    static_assert(Animation::SerializedVersion == 1, "Update code.");

    // Save animation data
    MemoryWriteStream stream(8182);
    int32 animIndex = options ? options->ObjectIndex : -1; // Single animation per asset
    if (animIndex == -1)
    {
        // Pick the longest animation by default (eg. to skip ref pose anim if exported as the first one)
        animIndex = 0;
        for (int32 i = 1; i < modelData.Animations.Count(); i++)
        {
            if (modelData.Animations[i].GetLength() > modelData.Animations[animIndex].GetLength())
                animIndex = i;
        }
    }
    if (Animation::SaveHeader(modelData, stream, animIndex))
        return CreateAssetResult::Error;
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportModel::CreatePrefab(CreateAssetContext& context, const ModelData& data, const Options& options, const Array<PrefabObject>& prefabObjects)
{
    PROFILE_CPU();
    if (data.Nodes.Count() == 0)
        return CreateAssetResult::Error;

    // If that prefab already exists then we need to use it as base to preserve object IDs and local changes applied by user
    const String outputPath = String(StringUtils::GetPathWithoutExtension(context.TargetAssetPath)) + DEFAULT_PREFAB_EXTENSION_DOT;
    auto* prefab = FileSystem::FileExists(outputPath) ? Content::Load<Prefab>(outputPath) : nullptr;
    if (prefab)
    {
        // Ensure that prefab has Default Instance so ObjectsCache is valid (used below)
        prefab->GetDefaultInstance();
    }

    // Create prefab structure
    Dictionary<int32, Actor*> nodeToActor;
    Dictionary<Guid, SceneObject*> newPrefabObjects; // Maps prefab object id to the restored and linked object
    rapidjson_flax::StringBuffer jsonBuffer;
    Array<Actor*> nodeActors;
    Actor* rootActor = nullptr;
    for (int32 nodeIndex = 0; nodeIndex < data.Nodes.Count(); nodeIndex++)
    {
        const auto& node = data.Nodes[nodeIndex];

        // Create actor(s) for this node
        nodeActors.Clear();
        for (const PrefabObject& e : prefabObjects)
        {
            if (e.NodeIndex == nodeIndex)
            {
                auto* actor = New<StaticModel>();
                actor->SetName(e.Name);
                if (auto* model = Content::LoadAsync<Model>(e.AssetPath))
                {
                    actor->Model = model;
                }
                nodeActors.Add(actor);
            }
        }
        Actor* nodeActor = nodeActors.Count() == 1 ? nodeActors[0] : New<EmptyActor>();
        if (nodeActors.Count() > 1)
        {
            for (Actor* e : nodeActors)
            {
                e->SetParent(nodeActor);
            }
        }
        if (nodeActors.Count() != 1)
        {
            // Include default actor to iterate over it properly in code below
            nodeActors.Add(nodeActor);
        }

        // Setup node in hierarchy
        nodeToActor.Add(nodeIndex, nodeActor);
        nodeActor->SetName(node.Name);

        // When use local origin is checked, it shifts everything over the same amount, including the root. This tries to work around that.
        if (!(nodeIndex == 0 && options.UseLocalOrigin))
        {
            // TODO: Improve this hack.
            // Assimp importer has the meter -> centimeter conversion scale applied to the local transform of
            // the root node, and only the root node. The OpenFBX importer has the same scale applied
            // to each node, *except* the root node. This difference makes it hard to calculate the
            // global scale properly. Position offsets are not calculated properly from Assimp without summing up
            // the global scale because translations from Assimp don't get scaled with the global scaler option,
            // but the OpenFBX importer does scale them. So this hack will end up only applying the global scale
            // change if its using Assimp due to the difference in where the nodes' local transform scales are set.
            auto* currentNode = &node;
            Vector3 scale = Vector3::One;
            while (true)
            {
                if (currentNode->ParentIndex == -1)
                {
                    scale *= currentNode->LocalTransform.Scale;
                    break;
                }
                currentNode = &data.Nodes[currentNode->ParentIndex];
            }

            // Only set translation, since scale and rotation is applied earlier.
            Transform positionOffset = Transform::Identity;
            positionOffset.Translation = node.LocalTransform.Translation * scale;

            if (options.UseLocalOrigin)
            {
                positionOffset.Translation += data.Nodes[0].LocalTransform.Translation;
            }

            nodeActor->SetLocalTransform(positionOffset);
        }

        if (nodeIndex == 0)
        {
            // Special case for root actor to link any unlinked nodes
            nodeToActor.Add(-1, nodeActor);
            rootActor = nodeActor;
        }
        else
        {
            Actor* parentActor;
            if (nodeToActor.TryGet(node.ParentIndex, parentActor))
                nodeActor->SetParent(parentActor);
        }

        // Link with object from prefab (if reimporting)
        if (prefab)
        {
            for (Actor* a : nodeActors)
            {
                for (const auto& i : prefab->ObjectsCache)
                {
                    if (i.Value->GetTypeHandle() != a->GetTypeHandle()) // Type match
                        continue;
                    auto* o = (Actor*)i.Value;
                    if (o->GetName() != a->GetName()) // Name match
                        continue;

                    // Preserve local changes made in the prefab
                    CloneObject(jsonBuffer, o, a, true);

                    // Mark as this object already exists in prefab so will be preserved when updating it
                    const Guid prefabObjectId = o->GetPrefabObjectID();
                    a->LinkPrefab(o->GetPrefabID(), prefabObjectId);
                    newPrefabObjects.Add(prefabObjectId, a);
                    break;
                }
            }
        }
    }
    ASSERT_LOW_LAYER(rootActor);
    {
        // Add script with import options
        auto* modelPrefabScript = New<ModelPrefab>();
        modelPrefabScript->SetParent(rootActor);
        modelPrefabScript->ImportPath = AssetsImportingManager::GetImportPath(context.InputPath);
        modelPrefabScript->ImportOptions = options;

        // Link with existing prefab instance
        if (prefab)
        {
            for (const auto& i : prefab->ObjectsCache)
            {
                if (i.Value->GetTypeHandle() == modelPrefabScript->GetTypeHandle())
                {
                    const Guid prefabObjectId = i.Value->GetPrefabObjectID();
                    modelPrefabScript->LinkPrefab(i.Value->GetPrefabID(), prefabObjectId);
                    newPrefabObjects.Add(prefabObjectId, modelPrefabScript);
                    break;
                }
            }
        }
    }
    if (prefab)
    {
        // Preserve existing objects added by user (eg. colliders, sfx, vfx, scripts)
        for (const auto& i : prefab->ObjectsCache)
        {
            // Skip already restored objects
            const Guid prefabObjectId = i.Key;
            if (newPrefabObjects.ContainsKey(prefabObjectId))
                continue;
            SceneObject* defaultObject = i.Value;
            // TODO: ignore objects that were imported previously but not now (eg. mesh was removed from source asset)

            // Find parent to link
            SceneObject* parent;
            if (!newPrefabObjects.TryGet(defaultObject->GetParent()->GetPrefabObjectID(), parent))
                continue;

            // Duplicate object
            SceneObject* restoredObject = (SceneObject*)Scripting::NewObject(defaultObject->GetTypeHandle());
            if (!restoredObject)
                continue;
            CloneObject(jsonBuffer, defaultObject, restoredObject);
            restoredObject->SetParent((Actor*)parent);

            // Link with existing prefab instance
            restoredObject->LinkPrefab(i.Value->GetPrefabID(), prefabObjectId);
            newPrefabObjects.Add(prefabObjectId, restoredObject);
        }
    }

    // Create prefab instead of native asset
    bool failed;
    if (prefab)
    {
        failed = prefab->ApplyAll(rootActor);
    }
    else
    {
        failed = PrefabManager::CreatePrefab(rootActor, outputPath, false);
    }

    // Cleanup objects from memory
    rootActor->DeleteObjectNow();

    if (failed)
        return CreateAssetResult::Error;
    return CreateAssetResult::Skip;
}

#endif
