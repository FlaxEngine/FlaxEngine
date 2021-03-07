// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CreateJson.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Log.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Storage/JsonStorageProxy.h"
#include "Engine/Serialization/JsonWriters.h"
#include "FlaxEngine.Gen.h"

bool CreateJson::Create(const StringView& path, rapidjson_flax::StringBuffer& data, const String& dataTypename)
{
    auto str = dataTypename.ToStringAnsi();
    return Create(path, data, str.Get());
}

bool CreateJson::Create(const StringView& path, rapidjson_flax::StringBuffer& data, const char* dataTypename)
{
    StringAnsiView data1((char*)data.GetString(), (int32)data.GetSize());
    StringAnsiView data2((char*)dataTypename, StringUtils::Length(dataTypename));
    return Create(path, data1, data2);
}

bool CreateJson::Create(const StringView& path, StringAnsiView& data, StringAnsiView& dataTypename)
{
    Guid id = Guid::New();

    LOG(Info, "Creating json resource of type \'{1}\' at \'{0}\'", path, String(dataTypename.Get()));

    // Try use the same asset ID
    if (FileSystem::FileExists(path))
    {
        String typeName;
        JsonStorageProxy::GetAssetInfo(path, id, typeName);
        if (typeName != String(dataTypename.Get(), dataTypename.Length()))
        {
            LOG(Warning, "Asset will have different type name {0} -> {1}", typeName, String(dataTypename.Get()));
        }
    }

    rapidjson_flax::StringBuffer buffer;

    // Serialize to json
    PrettyJsonWriter writerObj(buffer);
    JsonWriter& writer = writerObj;
    writer.StartObject();
    {
        // Json resource header
        writer.JKEY("ID");
        writer.Guid(id);
        writer.JKEY("TypeName");
        writer.String(dataTypename.Get(), dataTypename.Length());
        writer.JKEY("EngineBuild");
        writer.Int(FLAXENGINE_VERSION_BUILD);

        // Json resource data
        writer.JKEY("Data");
        writer.RawValue(data.Get(), data.Length());
    }
    writer.EndObject();

    // Save json to file
    if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
    {
        LOG(Warning, "Failed to save json to file");
        return true;
    }

    // Reload asset at the target location if is loaded
    auto asset = Content::GetAsset(path);
    if (asset)
    {
        asset->Reload();
    }

    return false;
}

#endif
