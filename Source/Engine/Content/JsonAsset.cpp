// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "JsonAsset.h"
#include "Storage/ContentStorageManager.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Platform/File.h"
#endif
#include "FlaxEngine.Gen.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"

JsonAssetBase::JsonAssetBase(const SpawnParams& params, const AssetInfo* info)
    : Asset(params, info)
    , _path(info->Path)
    , Data(nullptr)
    , DataEngineBuild(0)
{
}

String JsonAssetBase::GetData() const
{
    if (Data == nullptr)
        return String::Empty;

    // Get serialized data
    rapidjson_flax::StringBuffer buffer;
    rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
    Data->Accept(writer);

    return String((const char*)buffer.GetString(), (int32)buffer.GetSize());
}

const String& JsonAssetBase::GetPath() const
{
    return _path;
}

#if USE_EDITOR

void FindIds(ISerializable::DeserializeStream& node, Array<Guid>& output)
{
    if (node.IsObject())
    {
        for (auto i = node.MemberBegin(); i != node.MemberEnd(); ++i)
        {
            FindIds(i->value, output);
        }
    }
    else if (node.IsArray())
    {
        for (rapidjson::SizeType i = 0; i < node.Size(); i++)
        {
            FindIds(node[i], output);
        }
    }
    else if (node.IsString())
    {
        const auto length = node.GetStringLength();
        if (length == 32)
        {
            // Try parse as Guid in format `N` (32 hex chars)
            Guid id;
            if (!Guid::Parse(node.GetText(), id))
                output.Add(id);
        }
    }
}

void JsonAssetBase::GetReferences(Array<Guid>& output) const
{
    if (Data == nullptr)
        return;

    // Unified way to find asset references inside a generic asset.
    // This could deserialize managed/unmanaged object or load actors in case of SceneAsset or PrefabAsset.
    // But this would be performance killer.
    // The fastest way is to just iterate through the json and find all the Guid values.
    // It produces many invalid ids (like refs to scene objects).
    // But it's super fast, super low-memory and doesn't involve any advanced systems integration.

    FindIds(*Data, output);
}

#endif

Asset::LoadResult JsonAssetBase::loadAsset()
{
    // Load data (raw json file in editor, cooked asset in build game)
#if USE_EDITOR

    BytesContainer data;
    if (File::ReadAllBytes(GetPath(), data))
    {
        LOG(Warning, "Filed to load json asset data. {0}", ToString());
        return LoadResult::CannotLoadData;
    }
    if (data.Length() == 0)
    {
        return LoadResult::MissingDataChunk;
    }

#else

    // Get the asset storage container but don't load it now
    const auto storage = ContentStorageManager::GetStorage(GetPath(), true);
    if (!storage)
        return LoadResult::CannotLoadStorage;

    // Load header
    AssetInitData initData;
    if (storage->LoadAssetHeader(GetID(), initData))
        return LoadResult::CannotLoadInitData;

    // Load the actual data
    auto chunk = initData.Header.Chunks[0];
    if (chunk == nullptr)
        return LoadResult::MissingDataChunk;
    if (storage->LoadAssetChunk(chunk))
        return LoadResult::CannotLoadData;
    auto& data = chunk->Data;

#endif

    // Parse json document
    Document.Parse(data.Get<char>(), data.Length());
    if (Document.HasParseError())
    {
        Log::JsonParseException(Document.GetParseError(), Document.GetErrorOffset());
        return LoadResult::CannotLoadData;
    }

    // Gather information from the header
    const auto id = JsonTools::GetGuid(Document, "ID");
    if (id != _id)
    {
        LOG(Warning, "Invalid json asset id. Asset: {0}, serialized: {1}.", _id, id);
        return LoadResult::InvalidData;
    }
    DataTypeName = JsonTools::GetString(Document, "TypeName");
    DataEngineBuild = JsonTools::GetInt(Document, "EngineBuild", FLAXENGINE_VERSION_BUILD);
    auto dataMember = Document.FindMember("Data");
    if (dataMember == Document.MemberEnd())
    {
        LOG(Warning, "Missing json asset data.");
        return LoadResult::InvalidData;
    }
    Data = &dataMember->value;

    return LoadResult::Ok;
}

void JsonAssetBase::unload(bool isReloading)
{
    ISerializable::SerializeDocument tmp;
    Document.Swap(tmp);
    Data = nullptr;
    DataTypeName.Clear();
    DataEngineBuild = 0;
}

#if USE_EDITOR

void JsonAssetBase::onRename(const StringView& newPath)
{
    ScopeLock lock(Locker);

    // Rename
    _path = newPath;
}

#endif

REGISTER_JSON_ASSET(JsonAsset, "FlaxEngine.JsonAsset");

////////////////////////////////////////////////////////////////////////////////////

#include "Engine/Physics/PhysicalMaterial.h"

// Unmanaged json asset types that are serialized to JsonAsset and should be created by auto by asset.
// This allows to reuse JsonAsset without creating dedicated asset types. It has been designed for lightweight resources.

typedef ISerializable* (*UnmanagedJsonInstanceCreator)();

template<typename T>
ISerializable* Create()
{
    return New<T>();
}

Dictionary<String, UnmanagedJsonInstanceCreator> UnmanagedTypes(32);

void InitUnmanagedJsonTypes()
{
    ASSERT(UnmanagedTypes.IsEmpty());

    // Key: managed class typename, Value: unmanaged instance spawner function
    UnmanagedTypes[TEXT("FlaxEngine.PhysicalMaterial")] = &Create<PhysicalMaterial>;
}

////////////////////////////////////////////////////////////////////////////////////

JsonAsset::JsonAsset(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
    , Instance(nullptr)
{
    if (UnmanagedTypes.IsEmpty())
        InitUnmanagedJsonTypes();
}

Asset::LoadResult JsonAsset::loadAsset()
{
    // Base
    auto result = JsonAssetBase::loadAsset();
    if (result != LoadResult::Ok)
        return result;

    UnmanagedJsonInstanceCreator instanceSpawner = nullptr;
    if (UnmanagedTypes.TryGet(DataTypeName, instanceSpawner))
    {
        Instance = instanceSpawner();
        Instance->Deserialize(*Data, nullptr);
    }

    return result;
}

void JsonAsset::unload(bool isReloading)
{
    // Base
    JsonAssetBase::unload(isReloading);

    SAFE_DELETE(Instance);
}
