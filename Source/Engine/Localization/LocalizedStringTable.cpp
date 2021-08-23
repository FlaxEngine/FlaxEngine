// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "LocalizedStringTable.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/SerializationFwd.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#if USE_EDITOR
#include "Engine/ContentImporters/CreateJson.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Core/Log.h"
#endif

REGISTER_JSON_ASSET(LocalizedStringTable, "FlaxEngine.LocalizedStringTable", true);

LocalizedStringTable::LocalizedStringTable(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
{
}

void LocalizedStringTable::AddString(const StringView& id, const StringView& value)
{
    auto& values = Entries[id];
    values.Resize(1);
    values[0] = value;
}

void LocalizedStringTable::AddPluralString(const StringView& id, const StringView& value, int32 n)
{
    CHECK(n >= 0 && n < 1024);
    auto& values = Entries[id];
    values.Resize(Math::Max(values.Count(), n + 1));
    values[n] = value;
}



String LocalizedStringTable::GetString(const String& id)
{
    StringView result;
    const auto messages = Entries.TryGet(id);
    if (messages && messages->Count() != 0)
    {
        result = messages->At(0);
    }
    if (result.IsEmpty() && FallbackTable)
        result = FallbackTable->GetString(id);
    return result;
}

String LocalizedStringTable::GetPluralString(const String& id, int32 n)
{
    StringView result;
    const auto messages = Entries.TryGet(id);
    if (messages && messages->Count() != 0)
    {
        result = messages->At(0);
    }
    if (result.IsEmpty() && FallbackTable)
        result = FallbackTable->GetPluralString(id, n);
    return result;
}


#if USE_EDITOR

bool LocalizedStringTable::Save(const StringView& path)
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

    // Serialize data
    rapidjson_flax::StringBuffer outputData;
    PrettyJsonWriter writerObj(outputData);
    JsonWriter& writer = writerObj;
    writer.StartObject();
    {
        writer.JKEY("Locale");
        writer.String(Locale);

        writer.JKEY("Entries");
        writer.StartObject();
        for (auto& e : Entries)
        {
            writer.Key(e.Key);
            if (e.Value.Count() == 1)
            {
                writer.String(e.Value[0]);
            }
            else
            {
                writer.StartArray();
                for (auto& q : e.Value)
                    writer.String(q);
                writer.EndArray();
            }
        }
        writer.EndObject();
    }
    writer.EndObject();

    // Save asset
    const bool saveResult = CreateJson::Create(path.HasChars() ? path : StringView(GetPath()), outputData, TypeName);
    if (saveResult)
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif

Asset::LoadResult LocalizedStringTable::loadAsset()
{
    // Base
    auto result = JsonAssetBase::loadAsset();
    if (result != LoadResult::Ok || IsInternalType())
        return result;

    JsonTools::GetString(Locale, *Data, "Locale");
    const auto entriesMember = SERIALIZE_FIND_MEMBER((*Data), "Entries");
    if (entriesMember != Data->MemberEnd() && entriesMember->value.IsObject())
    {
        Entries.EnsureCapacity(entriesMember->value.MemberCount());
        for (auto i = entriesMember->value.MemberBegin(); i != entriesMember->value.MemberEnd(); ++i)
        {
            const String key(i->name.GetText());
            auto& e = Entries[key];
            auto& value = i->value;
            if (value.IsString())
            {
                e.Resize(1);
                e[0] = value.GetText();
            }
            else if (value.IsArray())
            {
                e.Resize(value.Size());
                for (int32 q = 0; q < e.Count(); q++)
                    e[q] = value[q].GetText();
            }
        }
    }

    return result;
}

void LocalizedStringTable::unload(bool isReloading)
{
    // Base
    JsonAssetBase::unload(isReloading);

    Locale.Clear();
    Entries.Clear();
}
