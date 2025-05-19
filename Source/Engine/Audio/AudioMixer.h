// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Variant.h"
#include "Audio.h"

API_CLASS(NoSpawn) class FLAXENGINE_API AudioMixer : public BinaryAsset 
{
    DECLARE_BINARY_ASSET_HEADER(AudioMixer,2);

public:
    struct MixerVariable
    {
        /// <summary>
        /// The current value.
        /// </summary>
        float Value;

        /// <summary>
        /// The default value.
        /// </summary>
        float DefaultValue;
    };

public:
    /// <summary>
    /// The collection of audio mixer variables identified by the name.
    /// </summary>
    Dictionary<String, MixerVariable> AudioMixerVariables;



public:
    // [BinaryAsset]
   void InitAsVirtual() override;
#if USE_EDITOR
   bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
