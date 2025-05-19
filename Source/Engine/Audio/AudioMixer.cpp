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
    bool AudioMixer::Save(const StringView& path)
    {
        if (OnCheckSave(path))
            return true;
        ScopeLock lock(Locker);

        // Save to bytes
        const auto AudioMixerGroup = AudioSettings::Get()->AudioMixerGroups;
        MemoryWriteStream stream(1024);
        stream.Write(AudioMixerVariables.Count());
        for (int index = 0; index < AudioMixerGroup.Count(); index++) 
        {
            for (auto& e : AudioMixerVariables)
            {
                e.Key = AudioMixerGroup[index].Name; e.Value.DefaultValue = AudioMixerGroup[index].MixerVolume;
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
