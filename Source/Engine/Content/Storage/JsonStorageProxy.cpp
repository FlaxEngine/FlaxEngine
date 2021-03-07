// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "JsonStorageProxy.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Level/Types.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include <ThirdParty/rapidjson/document.h>

bool JsonStorageProxy::IsValidExtension(const StringView& extension)
{
    return extension == DEFAULT_SCENE_EXTENSION || extension == DEFAULT_PREFAB_EXTENSION || extension == DEFAULT_JSON_EXTENSION;
}

bool JsonStorageProxy::GetAssetInfo(const StringView& path, Guid& resultId, String& resultDataTypeName)
{
    // TODO: we could just open file and start reading until we find 'ID:..' without parsing whole file - could be much more faster

    // Load file
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        return false;
    }

    // Parse data
    rapidjson_flax::Document document;
    document.Parse((const char*)fileData.Get(), fileData.Count());
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), String(path));
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

void ChangeIds(rapidjson_flax::Value& obj, rapidjson_flax::Document& document, const StringAnsi& srcId, const StringAnsi& dstId)
{
    if (obj.IsObject())
    {
        for (rapidjson_flax::Value::MemberIterator i = obj.MemberBegin(); i != obj.MemberEnd(); ++i)
        {
            ChangeIds(i->value, document, srcId, dstId);
        }
    }
    else if (obj.IsArray())
    {
        for (rapidjson::SizeType i = 0; i < obj.Size(); i++)
        {
            ChangeIds(obj[i], document, srcId, dstId);
        }
    }
    else if (obj.IsString())
    {
        if (StringUtils::Compare(srcId.Get(), obj.GetString()) == 0)
        {
            obj.SetString(dstId.Get(), document.GetAllocator());
        }
    }
}

#endif

bool JsonStorageProxy::ChangeId(const StringView& path, const Guid& newId)
{
#if USE_EDITOR

    // Load file
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        return false;
    }

    // Parse data
    rapidjson_flax::Document document;
    document.Parse((const char*)fileData.Get(), fileData.Count());
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), String(path));
        return false;
    }

    // Try get asset metadata
    auto idNode = document.FindMember("ID");
    if (idNode == document.MemberEnd())
    {
        return true;
    }

    // Change IDs
    auto oldIdStr = idNode->value.GetString();
    auto newIdStr = newId.ToString(Guid::FormatType::N).ToStringAnsi();
    ChangeIds(document, document, oldIdStr, newIdStr);

    // Save to file
    rapidjson_flax::StringBuffer buffer;
    PrettyJsonWriter writer(buffer);
    document.Accept(writer.GetWriter());
    if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
    {
        return true;
    }

    return false;

#else

	LOG(Warning, "Editing cooked content is invalid.");
	return true;

#endif
}
