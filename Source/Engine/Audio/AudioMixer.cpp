// Copyright (c) Wojciech Figat. All rights reserved.

#include "AudioMixer.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/BinaryAssetUpgrader.h"
#include "Engine/Threading/Threading.h"

#if USE_EDITOR
    bool AudioMixer::Save(const StringView& path)
    {
        if (OnCheckSave(path))
            return true;
        ScopeLock lock(Locker);

        // Save to bytes
        MemoryWriteStream stream(1024);
        stream.Write(AudioMixerVariables.Count());

        for (auto& e : AudioMixerVariables) 
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

        return false;
    }
#endif

void AudioMixer::InitAsVirtual()
{
    BinaryAsset::InitAsVirtual();

    AudioMixerVariables.Clear();
}
