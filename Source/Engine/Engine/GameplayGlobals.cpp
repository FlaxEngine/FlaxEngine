// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GameplayGlobals.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/BinaryAssetUpgrader.h"
#include "Engine/Threading/Threading.h"

#if USE_EDITOR

class GameplayGlobalsUpgrader : public BinaryAssetUpgrader
{
public:

    GameplayGlobalsUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 2, &Upgrade_1_To_2 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:

    static bool Upgrade_1_To_2(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 2);

        if (context.AllocateChunk(0))
            return true;
        auto& data = context.Input.Header.Chunks[0]->Data;
        MemoryReadStream stream(data.Get(), data.Length());
        MemoryWriteStream output;
        int32 count;
        stream.ReadInt32(&count);
        output.WriteInt32(count);
        String name;
        for (int32 i = 0; i < count; i++)
        {
            stream.ReadString(&name, 71);
            CommonValue commonValue;
            stream.ReadCommonValue(&commonValue);
            Variant variant(commonValue);
            output.WriteVariant(variant);
        }
        context.Output.Header.Chunks[0]->Data.Copy(output.GetHandle(), output.GetPosition());

        return false;
    }
};

#endif

REGISTER_BINARY_ASSET_WITH_UPGRADER(GameplayGlobals, "FlaxEngine.GameplayGlobals", GameplayGlobalsUpgrader, true);

GameplayGlobals::GameplayGlobals(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Dictionary<String, Variant> GameplayGlobals::GetValues() const
{
    ScopeLock lock(Locker);
    Dictionary<String, Variant> result;
    for (auto& e : Variables)
        result.Add(e.Key, e.Value.Value);
    return result;
}

void GameplayGlobals::SetValues(const Dictionary<String, Variant>& values)
{
    ScopeLock lock(Locker);
    for (auto& e : values)
    {
        bool hasKey = false;
        for (auto& q : values)
        {
            if (e.Key == q.Key)
            {
                hasKey = true;
                break;
            }
        }
        if (!hasKey)
        {
            Variables.Remove(e.Key);
        }
    }
    for (auto i = values.Begin(); i.IsNotEnd(); ++i)
    {
        auto e = Variables.TryGet(i->Key);
        if (!e)
        {
            e = &Variables[i->Key];
            e->DefaultValue = i->Value;
        }
        e->Value = i->Value;
    }
}

Dictionary<String, Variant> GameplayGlobals::GetDefaultValues() const
{
    ScopeLock lock(Locker);
    Dictionary<String, Variant> result;
    for (auto& e : Variables)
        result.Add(e.Key, e.Value.DefaultValue);
    return result;
}

void GameplayGlobals::SetDefaultValues(const Dictionary<String, Variant>& values)
{
    ScopeLock lock(Locker);
    for (auto& e : values)
    {
        bool hasKey = false;
        for (auto& q : values)
        {
            if (e.Key == q.Key)
            {
                hasKey = true;
                break;
            }
        }
        if (!hasKey)
        {
            Variables.Remove(e.Key);
        }
    }
    for (auto i = values.Begin(); i.IsNotEnd(); ++i)
    {
        auto e = Variables.TryGet(i->Key);
        if (!e)
        {
            e = &Variables[i->Key];
            e->Value = i->Value;
        }
        e->DefaultValue = i->Value;
    }
}

Variant GameplayGlobals::GetValue(const StringView& name) const
{
    ScopeLock lock(Locker);
    auto e = Variables.TryGet(name);
    return e ? e->Value : Variant::Zero;
}

void GameplayGlobals::SetValue(const StringView& name, const Variant& value)
{
    ScopeLock lock(Locker);
    auto e = Variables.TryGet(name);
    if (e)
    {
        e->Value = value;
    }
}

void GameplayGlobals::ResetValues()
{
    ScopeLock lock(Locker);
    for (auto& e : Variables)
    {
        e.Value.Value = e.Value.DefaultValue;
    }
}

#if USE_EDITOR

bool GameplayGlobals::Save(const StringView& path)
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

    // Save to bytes
    MemoryWriteStream stream(1024);
    stream.WriteInt32(Variables.Count());
    for (auto& e : Variables)
    {
        stream.WriteString(e.Key, 71);
        stream.WriteVariant(e.Value.DefaultValue);
    }

    // Set chunk data
    FlaxChunk* chunk;
    if (IsVirtual())
    {
        _header.Chunks[0] = chunk = New<FlaxChunk>();
    }
    else
    {
        chunk = GetOrCreateChunk(0);
    }
    chunk->Data.Copy(stream.GetHandle(), stream.GetPosition());

    // Save
    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
    const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
    if (IsVirtual())
    {
        _header.Chunks[0] = nullptr;
        Delete(chunk);
    }
    if (saveResult)
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif

void GameplayGlobals::InitAsVirtual()
{
    BinaryAsset::InitAsVirtual();

    Variables.Clear();
}

Asset::LoadResult GameplayGlobals::load()
{
    // Get data
    const auto chunk = GetChunk(0);
    if (!chunk || !chunk->IsLoaded())
    {
        return LoadResult::MissingDataChunk;
    }
    MemoryReadStream stream(chunk->Get(), chunk->Size());

    // Load all variables
    int32 count;
    stream.ReadInt32(&count);
    Variables.EnsureCapacity(count);
    String name;
    for (int32 i = 0; i < count; i++)
    {
        stream.ReadString(&name, 71);
        if (name.IsEmpty())
        {
            LOG(Warning, "Empty variable name");
            return LoadResult::InvalidData;
        }
        auto& e = Variables[name];
        stream.ReadVariant(&e.DefaultValue);
        e.Value = e.DefaultValue;
    }

    return LoadResult::Ok;
}

void GameplayGlobals::unload(bool isReloading)
{
    Variables.Clear();
}

AssetChunksFlag GameplayGlobals::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
