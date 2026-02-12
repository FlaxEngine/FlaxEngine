// Copyright (c) Wojciech Figat. All rights reserved.

#include "GameplayGlobals.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Threading/Threading.h"

REGISTER_BINARY_ASSET(GameplayGlobals, "FlaxEngine.GameplayGlobals", true);

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
    for (auto it = Variables.Begin(); it.IsNotEnd(); ++it)
    {
        if (!values.ContainsKey(it->Key))
        {
            Variables.Remove(it);
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
    for (auto it = Variables.Begin(); it.IsNotEnd(); ++it)
    {
        if (!values.ContainsKey(it->Key))
        {
            Variables.Remove(it);
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

const Variant& GameplayGlobals::GetValue(const StringView& name) const
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
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);

    // Save to bytes
    MemoryWriteStream stream(1024);
    stream.Write(Variables.Count());
    for (auto& e : Variables)
    {
        stream.Write(e.Key, 71);
        stream.Write(e.Value.DefaultValue);
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
    chunk->Data.Copy(ToSpan(stream));

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
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(chunk->Get(), chunk->Size());

    // Load all variables
    int32 count;
    stream.Read(count);
    Variables.EnsureCapacity(count);
    String name;
    for (int32 i = 0; i < count; i++)
    {
        stream.Read(name, 71);
        auto& e = Variables[name];
        stream.Read(e.DefaultValue);
        e.Value = e.DefaultValue;
    }
    if (stream.HasError())
    {
        // Failed to load data
        Variables.Clear();
        return LoadResult::InvalidData;
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
