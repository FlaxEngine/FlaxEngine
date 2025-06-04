// Copyright (c) Wojciech Figat. All rights reserved.

#include "JsonStorageProxy.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Level/Types.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#if USE_EDITOR
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Core/Collections/Dictionary.h"
#endif
#include <ThirdParty/rapidjson/document.h>

bool JsonStorageProxy::IsValidExtension(const StringView& extension)
{
    return extension == DEFAULT_SCENE_EXTENSION || extension == DEFAULT_PREFAB_EXTENSION || extension == DEFAULT_JSON_EXTENSION;
}

bool JsonStorageProxy::GetAssetInfo(const StringView& path, Guid& resultId, String& resultDataTypeName)
{
    PROFILE_CPU();
    // TODO: we could just open file and start reading until we find 'ID:..' without parsing whole file - could be much more faster

    // Load file
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        return false;
    }

    // Parse data
    rapidjson_flax::Document document;
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse((const char*)fileData.Get(), fileData.Count());
    }
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), path);
        return false;
    }

    // Try get asset metadata
    auto idNode = document.FindMember("ID");
    auto typeNameNode = document.FindMember("TypeName");
    if (idNode != document.MemberEnd() && typeNameNode != document.MemberEnd())
    {
        // Found
        resultId = JsonTools::GetGuid(idNode->value);
        resultDataTypeName = typeNameNode->value.GetText();
        return true;
    }

    return false;
}

#if USE_EDITOR

void FindObjectIds(const rapidjson_flax::Value& obj, const rapidjson_flax::Document& document, HashSet<Guid>& ids, const char* parentName = nullptr)
{
    if (obj.IsObject())
    {
        for (rapidjson_flax::Value::ConstMemberIterator i = obj.MemberBegin(); i != obj.MemberEnd(); ++i)
        {
            FindObjectIds(i->value, document, ids, i->name.GetString());
        }
    }
    else if (obj.IsArray())
    {
        for (rapidjson::SizeType i = 0; i < obj.Size(); i++)
        {
            FindObjectIds(obj[i], document, ids, parentName);
        }
    }
    else if (obj.IsString() && obj.GetStringLength() == 32)
    {
        if (parentName && StringUtils::Compare(parentName, "ID") == 0)
        {
            auto value = JsonTools::GetGuid(obj);
            if (value.IsValid())
            {
                ids.Add(value);
            }
        }
    }
}

#endif

bool JsonStorageProxy::ChangeId(const StringView& path, const Guid& newId)
{
#if USE_EDITOR
    PROFILE_CPU();

    // Load file
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
        return false;

    // Parse data
    rapidjson_flax::Document document;
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse((const char*)fileData.Get(), fileData.Count());
    }
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), path);
        return false;
    }

    // Get all IDs inside the file
    HashSet<Guid> ids;
    FindObjectIds(document, document, ids);

    // Remap into a unique IDs
    Dictionary<Guid, Guid> remap;
    remap.EnsureCapacity(ids.Count());
    for (const auto& id : ids)
        remap.Add(id.Item, Guid::New());

    // Remap asset ID using the provided value
    auto idNode = document.FindMember("ID");
    if (idNode == document.MemberEnd())
        return true;
    remap[JsonTools::GetGuid(idNode->value)] = newId;

    // Change IDs of asset and objects inside asset
    JsonTools::ChangeIds(document, remap);

    // Save to file
    rapidjson_flax::StringBuffer buffer;
    PrettyJsonWriter writer(buffer);
    document.Accept(writer.GetWriter());
    if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
        return true;

    return false;
#else
	LOG(Warning, "Editing cooked content is invalid.");
	return true;
#endif
}
