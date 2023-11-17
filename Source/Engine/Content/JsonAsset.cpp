// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "JsonAsset.h"
#if USE_EDITOR
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Level/Level.h"
#else
#include "Storage/ContentStorageManager.h"
#endif
#include "Content.h"
#include "FlaxEngine.Gen.h"
#include "Cache/AssetsCache.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include "Engine/Core/Cache.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Threading/Threading.h"

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
    PROFILE_CPU_NAMED("JsonAsset.GetData");
    rapidjson_flax::StringBuffer buffer;
    OnGetData(buffer);
    return String((const char*)buffer.GetString(), (int32)buffer.GetSize());
}

void JsonAssetBase::SetData(const StringView& value)
{
    if (!IsLoaded())
        return;
    PROFILE_CPU_NAMED("JsonAsset.SetData");
    const StringAnsi dataJson(value);
    ScopeLock lock(Locker);
    const StringView dataTypeName = DataTypeName;
    if (Init(dataTypeName, dataJson))
    {
        LOG(Error, "Failed to set Json asset data.");
    }
}

bool JsonAssetBase::Init(const StringView& dataTypeName, const StringAnsiView& dataJson)
{
    unload(true);
    DataTypeName = dataTypeName;
    DataEngineBuild = FLAXENGINE_VERSION_BUILD;

    // Parse json document
    {
        PROFILE_CPU_NAMED("Json.Parse");
        Document.Parse(dataJson.Get(), dataJson.Length());
    }
    if (Document.HasParseError())
    {
        Log::JsonParseException(Document.GetParseError(), Document.GetErrorOffset());
        return true;
    }
    Data = &Document;
    _isVirtualDocument = true;

    // Load asset-specific data
    return loadAsset() != LoadResult::Ok;
}

void JsonAssetBase::OnGetData(rapidjson_flax::StringBuffer& buffer) const
{
    PrettyJsonWriter writerObj(buffer);
    Data->Accept(writerObj.GetWriter());
}

const String& JsonAssetBase::GetPath() const
{
#if USE_EDITOR
    return _path;
#else
    // In build all assets are packed into packages so use ID for original path lookup
    return Content::GetRegistry()->GetEditorAssetPath(_id);
#endif
}

uint64 JsonAssetBase::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = Asset::GetMemoryUsage();
    result += sizeof(JsonAssetBase) - sizeof(Asset);
    if (Data)
        result += Document.GetAllocator().Capacity();
    Locker.Unlock();
    return result;
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
        if (node.GetStringLength() == 32)
        {
            // Try parse as Guid in format `N` (32 hex chars)
            Guid id;
            if (!Guid::Parse(node.GetStringAnsiView(), id))
                output.Add(id);
        }
    }
}

void JsonAssetBase::GetReferences(const StringAnsiView& json, Array<Guid>& output)
{
    ISerializable::SerializeDocument document;
    document.Parse(json.Get(), json.Length());
    if (document.HasParseError())
        return;
    FindIds(document, output);
}

bool JsonAssetBase::Save(const StringView& path) const
{
    // Validate state
    if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }
    if (IsVirtual() && path.IsEmpty())
    {
        LOG(Error, "To save virtual asset asset you need to specify the target asset path location.");
        return true;
    }
    ScopeLock lock(Locker);

    // Serialize to json to the buffer
    rapidjson_flax::StringBuffer buffer;
    PrettyJsonWriter writerObj(buffer);
    Save(writerObj);

    // Save json to file
    if (File::WriteAllBytes(path.HasChars() ? path : StringView(GetPath()), (byte*)buffer.GetString(), (int32)buffer.GetSize()))
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

bool JsonAssetBase::Save(JsonWriter& writer) const
{
    // Validate state
    if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }
    ScopeLock lock(Locker);

    writer.StartObject();
    {
        // Json resource header
        writer.JKEY("ID");
        writer.Guid(GetID());
        writer.JKEY("TypeName");
        writer.String(DataTypeName);
        writer.JKEY("EngineBuild");
        writer.Int(FLAXENGINE_VERSION_BUILD);

        // Json resource data
        rapidjson_flax::StringBuffer dataBuffer;
        OnGetData(dataBuffer);
        writer.JKEY("Data");
        writer.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
    }
    writer.EndObject();

    return false;
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
    if (IsVirtual() || _isVirtualDocument)
        return LoadResult::Ok;

    // Load data (raw json file in editor, cooked asset in build game)
#if USE_EDITOR
    BytesContainer data;
    if (File::ReadAllBytes(_path, data))
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
    const auto storage = ContentStorageManager::GetStorage(_path, true);
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
    {
        PROFILE_CPU_NAMED("Json.Parse");
        Document.Parse(data.Get<char>(), data.Length());
    }
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
    _isVirtualDocument = false;
}

#if USE_EDITOR

void JsonAssetBase::onRename(const StringView& newPath)
{
    ScopeLock lock(Locker);

    // Rename
    _path = newPath;
}

#endif

REGISTER_JSON_ASSET(JsonAsset, "FlaxEngine.JsonAsset", true);

JsonAsset::JsonAsset(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
    , Instance(nullptr)
{
}

uint64 JsonAsset::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = JsonAssetBase::GetMemoryUsage();
    result += sizeof(JsonAsset) - sizeof(JsonAssetBase);
    if (Instance && InstanceType)
        result += InstanceType.GetType().Size;
    Locker.Unlock();
    return result;
}

Asset::LoadResult JsonAsset::loadAsset()
{
    const auto result = JsonAssetBase::loadAsset();
    if (result != LoadResult::Ok || IsInternalType())
        return result;

    if (CreateInstance())
        return LoadResult::Failed;

#if USE_EDITOR
    // Reload instance when module with this type gets reloaded
    Level::ScriptsReloadStart.Bind<JsonAsset, &JsonAsset::OnScriptsReloadStart>(this);
    Level::ScriptsReloaded.Bind<JsonAsset, &JsonAsset::OnScriptsReloaded>(this);
#endif

    // Destroy instance on scripting shutdown (eg. asset from scripts)
    Scripting::ScriptsUnload.Bind<JsonAsset, &JsonAsset::DeleteInstance>(this);

    return LoadResult::Ok;
}

void JsonAsset::unload(bool isReloading)
{
#if USE_EDITOR
    Level::ScriptsReloadStart.Unbind<JsonAsset, &JsonAsset::OnScriptsReloadStart>(this);
    Level::ScriptsReloaded.Unbind<JsonAsset, &JsonAsset::OnScriptsReloaded>(this);
#endif
    Scripting::ScriptsUnload.Unbind<JsonAsset, &JsonAsset::DeleteInstance>(this);
    DeleteInstance();
    _isAfterReload |= isReloading;

    JsonAssetBase::unload(isReloading);
}

void JsonAsset::onLoaded_MainThread()
{
    JsonAssetBase::onLoaded_MainThread();

    // Special case for Settings assets to flush them after edited and saved in Editor
    const StringAsANSI<> dataTypeNameAnsi(DataTypeName.Get(), DataTypeName.Length());
    const auto typeHandle = Scripting::FindScriptingType(StringAnsiView(dataTypeNameAnsi.Get(), DataTypeName.Length()));
    if (Instance && typeHandle && typeHandle.IsSubclassOf(SettingsBase::TypeInitializer) && _isAfterReload)
    {
        _isAfterReload = false;
        ((SettingsBase*)Instance)->Apply();
    }
}

bool JsonAsset::CreateInstance()
{
    ScopeLock lock(Locker);
    if (Instance)
        return false;

    // Try to scripting type for this data
    const StringAsANSI<> dataTypeNameAnsi(DataTypeName.Get(), DataTypeName.Length());
    const auto typeHandle = Scripting::FindScriptingType(StringAnsiView(dataTypeNameAnsi.Get(), DataTypeName.Length()));
    if (typeHandle)
    {
        auto& type = typeHandle.GetType();
        switch (type.Type)
        {
        case ScriptingTypes::Class:
        {
            // Ensure that object can deserialized
            const ScriptingType::InterfaceImplementation* interface = type.GetInterface(ISerializable::TypeInitializer);
            if (!interface)
            {
                LOG(Warning, "Cannot deserialize {0} from Json Asset because it doesn't implement ISerializable interface.", type.ToString());
                break;
            }

            // Allocate object
            const auto instance = Allocator::Allocate(type.Size);
            if (!instance)
                return true;
            Instance = instance;
            InstanceType = typeHandle;
            _dtor = type.Class.Dtor;
            type.Class.Ctor(instance);

            // Deserialize object
            auto modifier = Cache::ISerializeModifier.Get();
            modifier->EngineBuild = DataEngineBuild;
            ((ISerializable*)((byte*)instance + interface->VTableOffset))->Deserialize(*Data, modifier.Value);
            break;
        }
        }
    }

    return false;
}

void JsonAsset::DeleteInstance()
{
    ScopeLock lock(Locker);

    // C# instance
    MObject* object = GetManagedInstance();
    MClass* klass = GetClass();
    if (object && klass)
    {
        const MField* field = klass->GetField("_instance");
        if (field)
            field->SetValue(object, nullptr);
    }

    // C++ instance
    if (!Instance || !_dtor)
        return;
    _dtor(Instance);
    InstanceType = ScriptingTypeHandle();
    Allocator::Free(Instance);
    Instance = nullptr;
    _dtor = nullptr;
}

#if USE_EDITOR

void JsonAsset::OnScriptsReloadStart()
{
    DeleteInstance();
}

void JsonAsset::OnScriptsReloaded()
{
    if (CreateInstance())
    {
        LOG(Warning, "Failed to reload {0} instance {1}.", ToString(), DataTypeName);
    }
}

#endif
