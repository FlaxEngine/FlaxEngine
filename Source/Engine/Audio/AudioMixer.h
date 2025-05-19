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
    /// <summary>
    /// The collection of audio mixer variables identified by the name.
    /// </summary>
    Dictionary<String, float> AudioMixerVariables;



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
