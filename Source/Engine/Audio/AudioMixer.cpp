// Copyright (c) Wojciech Figat. All rights reserved.

#include "AudioMixer.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/BinaryAssetUpgrader.h"
#include "Engine/Threading/Threading.h"
#include "AudioSettings.h"

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

REGISTER_BINARY_ASSET_WITH_UPGRADER(AudioMixer,"FlaxEngine.Audio.AudioMixer", AudioMixerUpgrader, true);

AudioMixer::AudioMixer(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Dictionary<String, Variant> AudioMixer::GetMixerValues() const
{
    ScopeLock lock(Locker);
    Dictionary<String,Variant> result;
    for (auto& e : AudioMixerVariables)
        result.Add(e.Key, e.Value.Value);
    return result;
}

#if USE_EDITOR
    bool AudioMixer::Save(const StringView& path)
    {
        if (OnCheckSave(path))
            return true;
        ScopeLock lock(Locker);

        // Save to bytes
        const auto AudioMixerGroup = AudioSettings::Get()->AudioMixerGroups;
        MemoryWriteStream stream(1024);
        stream.Write(AudioMixerVariables.Count());
        for (auto& i : AudioMixerGroup)
        {
            for (auto& e : AudioMixerVariables)
            {
                e.Key = i.Name; e.Value.DefaultValue = i.MixerVolume;
                stream.Write(e.Key, 71);
                stream.Write(e.Value.DefaultValue);
            }
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

    AudioMixerVariables.Clear();
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
    AudioMixerVariables.EnsureCapacity(count);
    String Name;
    for (int i = 0; i < count; i++) 
    {
        stream.Read(Name, 71);
        auto& e = AudioMixerVariables[Name];
        stream.Read(e.DefaultValue);
        e.Value = e.DefaultValue;
    }
    if(stream.HasError())
    {
        // Failed to load data
        AudioMixerVariables.Clear();
        return LoadResult::InvalidData;
    }

    return LoadResult::Ok;
}

void AudioMixer::unload(bool isReloading)
{
    AudioMixerVariables.Clear();
}

AssetChunksFlag AudioMixer::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
