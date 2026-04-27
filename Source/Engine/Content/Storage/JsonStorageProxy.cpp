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

StringAnsiView ParseJsonString(const StringAnsiView& json, int32 start)
{
    while (start < json.Length() && json[start] != '\"')
        start++;
    int32 end = start + 1;
    while (end < json.Length() && json[end] != '\"')
        end++;
    return json.Substring(start + 1, end - start - 1);
}

bool JsonStorageProxy::GetAssetInfo(const StringView& path, Guid& resultId, String& resultDataTypeName)
{
    PROFILE_CPU();
    PROFILE_MEM(Content);
    ZoneText(*path, path.Length());

    // Read the first part of the file to get asset metadata (ID and TypeName)
    auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::All);
    if (!file)
        return false;
    Array<byte> fileData;
    fileData.Resize(256);
    uint32 read = 0;
    file->Read(fileData.Get(), fileData.Count(), &read);
    Delete(file);
    file = nullptr;
    if (read != 0)
    {
        // Naive Json parsing to get ID and TypeName without full parsing
        StringAnsiView json((const char*)fileData.Get(), read);
        StringAnsiView idStart("\"ID\": ");
        StringAnsiView typenameStart("\"TypeName\": ");
        bool hasOneOfThem = false;
        for (int32 i = 0; i < json.Length() - 7; i++)
        {
            if (json.Substring(i).StartsWith(idStart))
            {
                StringAnsiView value = ParseJsonString(json, i + idStart.Length());
                if (Guid::Parse(value, resultId))
                    continue;
                if (hasOneOfThem)
                    return true;
                hasOneOfThem = true;
                i += value.Length() + idStart.Length() + 2;
            }
            if (json.Substring(i).StartsWith(typenameStart))
            {
                StringAnsiView value = ParseJsonString(json, i + typenameStart.Length());
                resultDataTypeName = String(value);
                if (hasOneOfThem)
                    return true;
                hasOneOfThem = true;
                i += value.Length() + typenameStart.Length() + 2;
            }
        }
    }

    // Load file
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

bool JsonStorageProxy::ChangeId(const StringView& path, const Guid& newId)
{
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
}

#endif
