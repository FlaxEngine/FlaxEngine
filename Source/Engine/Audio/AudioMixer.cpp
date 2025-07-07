// Copyright (c) Wojciech Figat. All rights reserved.

#include "AudioMixer.h"
#include "AudioSettings.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/BinaryAssetUpgrader.h"
#include "Engine/Threading/Threading.h"

#if USE_EDITOR
class AudioMixerUpgrader : public BinaryAssetUpgrader
{
public:
    AudioMixerUpgrader()
    {
        const Upgrader upgraders[] =
        {
            { 1, 2, &Upgrade_1_To_2 }, // [Deprecated on 31.07.2020, expires on 31.07.2022]
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    static bool Upgrade_1_To_2(AssetMigrationContext& context)
    {
        // [Deprecated on 31.07.2020, expires on 31.07.2022]
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
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
            stream.Read(name, 71);
            CommonValue commonValue;
            stream.ReadCommonValue(&commonValue);
            Variant variant(commonValue);
            output.WriteVariant(variant);
        }
        context.Output.Header.Chunks[0]->Data.Copy(output.GetHandle(), output.GetPosition());
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

            return false;
    }
};
#endif

REGISTER_BINARY_ASSET_WITH_UPGRADER(AudioMixer,"FlaxEngine.AudioMixer", AudioMixerUpgrader, true);

AudioMixer::AudioMixer(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

void AudioMixer::MixerInit()
{
    auto audioMixerGroups = AudioSettings::Get()->MixerGroupChannels;

    if (audioMixerGroups.Count() == 0) 
    {
        if (!MixerGroupsVariables.IsEmpty()) MixerGroupsVariables.Clear();
        return;
    }

    HashSet<String> currentGroupNames;
    for (auto& group : audioMixerGroups) currentGroupNames.Add(group.Name);

    Array<String> keysToRemove;
    for (auto& pair : MixerGroupsVariables)
        if (!currentGroupNames.Contains(pair.Key)) keysToRemove.Add(pair.Key);

    for (auto& key : keysToRemove) MixerGroupsVariables.Remove(key);

    for (auto& audioMixerGroup : audioMixerGroups)
    {
        if (!MixerGroupsVariables.ContainsKey(audioMixerGroup.Name)) 
        {
            auto& var = MixerGroupsVariables[audioMixerGroup.Name];
            var.Volume = audioMixerGroup.MixerVolume; var.isMuted = audioMixerGroup.isMuted;
        }
    }
}

Dictionary<String, Variant> AudioMixer::GetMixerVariablesValues() const
{
    ScopeLock lock(Locker);
    Dictionary<String, Variant> result;
    for (auto& e : MixerGroupsVariables)
        result.Add(e.Key, e.Value.Volume);
    return result;
}

void AudioMixer::SetMixerVariablesValues(const Dictionary<String, Variant>& values)
{
    ScopeLock lock(Locker);
    for (auto it = MixerGroupsVariables.Begin(); it.IsNotEnd(); ++it)
    {
        if (!values.ContainsKey(it->Key)) 
        {
            MixerGroupsVariables.Remove(it);
        }
    }
    for (auto i = values.Begin(); i.IsNotEnd(); ++i) 
    {
        auto e = MixerGroupsVariables.TryGet(i->Key);
        if (!e) 
        {
            e = &MixerGroupsVariables[i->Key];
            e->Volume = i->Value;
        }
        e->Volume = i->Value;
    }
}

Dictionary<String, Variant> AudioMixer::GetDefaultValues() const
{
    ScopeLock lock(Locker);
    Dictionary<String, Variant> result;
    for (auto& e : MixerGroupsVariables)
        result.Add(e.Key, e.Value.Volume);
    return result;
}

void AudioMixer::SetDefaultValues(const Dictionary<String, Variant>& values)
{
    ScopeLock lock(Locker);
    for (auto it = MixerGroupsVariables.Begin(); it.IsNotEnd(); ++it)
    {
        if (!values.ContainsKey(it->Key))
        {
            MixerGroupsVariables.Remove(it);
        }
    }
    for (auto i = values.Begin(); i.IsNotEnd(); ++i)
    {
        auto e = MixerGroupsVariables.TryGet(i->Key);
        if (!e)
        {
            e = &MixerGroupsVariables[i->Key];
            e->Volume = i->Value;
        }
        e->Volume = i->Value;
    }
}

const Variant& AudioMixer::GetMixerChannelVolume(const StringView& nameChannel) const
{
    ScopeLock lock(Locker);
    auto e = MixerGroupsVariables.TryGet(nameChannel);
    return e ? e->Volume : Variant::Zero;
}

void AudioMixer::SetMixerChannelVolume(const StringView& nameChannel, const Variant& value)
{
    ScopeLock lock(Locker);
    auto e = MixerGroupsVariables.TryGet(nameChannel);
    if (e) e->Volume = value;
}

void AudioMixer::ResetMixer() 
{
    ScopeLock lock(Locker);

    auto audioMixerGroups = AudioSettings::Get()->MixerGroupChannels;

    for (auto& group : audioMixerGroups) 
    {
        auto mixerGroup = MixerGroupsVariables.TryGet(group.Name);
        mixerGroup->Volume = group.MixerVolume;
        mixerGroup->isMuted = group.isMuted;
    }
}

#if USE_EDITOR

    bool AudioMixer::Save(const StringView& path)
    {
        if (OnCheckSave(path))
            return true;
        ScopeLock lock(Locker);

        // Save to bytes
        MemoryWriteStream stream(1024);
        stream.Write(MixerGroupsVariables.Count());
        for (auto& e : MixerGroupsVariables)
        {
            stream.Write(e.Key, 71);
            stream.Write(e.Value.Volume);
            stream.Write(e.Value.isMuted);
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

void AudioMixer::InitAsVirtual()
{
    BinaryAsset::InitAsVirtual();

    MixerGroupsVariables.Clear();
}

Asset::LoadResult AudioMixer::load() 
{
    // Get data
    const auto chunk = GetChunk(0);
    if (!chunk || !chunk->IsLoaded())
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(chunk->Get(), chunk->Size());

    // Load all variables
    int32 count;
    stream.Read(count);
    MixerGroupsVariables.EnsureCapacity(count);
    String Name;
    for (int i = 0; i < count; i++) 
    {
        stream.Read(Name, 71);
        auto& e = MixerGroupsVariables[Name];
        stream.Read(e.Volume);
        e.Volume = e.Volume;
        stream.Read(e.isMuted);
        e.isMuted = e.isMuted;
    }
    if(stream.HasError())
    {
        // Failed to load data
        MixerGroupsVariables.Clear();
        return LoadResult::InvalidData;
    }

    return LoadResult::Ok;
}

void AudioMixer::unload(bool isReloading)
{
    MixerGroupsVariables.Clear();
}

AssetChunksFlag AudioMixer::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
