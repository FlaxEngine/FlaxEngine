// Copyright (c) Wojciech Figat. All rights reserved.

#include "LocalizedStringTable.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/SerializationFwd.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#if USE_EDITOR
#include "Engine/Threading/Threading.h"
#include "Engine/Core/Log.h"
#endif

REGISTER_JSON_ASSET(LocalizedStringTable, "FlaxEngine.LocalizedStringTable", true);

LocalizedStringTable::LocalizedStringTable(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
{
    DataTypeName = TypeName;
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

String LocalizedStringTable::GetString(const String& id) const
{
    StringView result;
    const auto messages = Entries.TryGet(id);
    if (messages && messages->Count() != 0)
        result = messages->At(0);
    if (result.IsEmpty() && FallbackTable)
        result = FallbackTable->GetString(id);
    return result;
}

String LocalizedStringTable::GetPluralString(const String& id, int32 n) const
{
    StringView result;
    const auto messages = Entries.TryGet(id);
    if (messages && messages->Count() > n)
        result = messages->At(n);
    if (result.IsEmpty() && FallbackTable)
        result = FallbackTable->GetPluralString(id, n);
    return String::Format(result.GetText(), n);
}

Asset::LoadResult LocalizedStringTable::loadAsset()
{
    // Base
    auto result = JsonAssetBase::loadAsset();
    if (result != LoadResult::Ok || IsInternalType())
        return result;

    JsonTools::GetString(Locale, *Data, "Locale");
    JsonTools::GetReference(FallbackTable, *Data, "FallbackTable");
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
    FallbackTable = nullptr;
    Entries.Clear();
}

void LocalizedStringTable::OnGetData(rapidjson_flax::StringBuffer& buffer) const
{
    PrettyJsonWriter writerObj(buffer);
    JsonWriter& writer = writerObj;
    writer.StartObject();
    {
        writer.JKEY("Locale");
        writer.String(Locale);

        if (FallbackTable.GetID().IsValid())
        {
            writer.JKEY("FallbackTable");
            writer.Guid(FallbackTable.GetID());
        }

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
}

