// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Variant.h"
#include "BusGroups.h"

/// <summary>
/// 
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API AudioMixer : public BinaryAsset 
{
    DECLARE_BINARY_ASSET_HEADER(AudioMixer,2);

public:
    /// <summary>
    /// The collection of audio mixer variables identified by the name.
    /// </summary>
    Dictionary<String, BusGroups> MasterBuses;

public:
    /// <summary>
    /// Initialise which adds the AudioSettings data to the dictionary.
    /// </summary>
    API_FUNCTION() void MixerInit();

    API_PROPERTY()
    FORCE_INLINE Dictionary<String, BusGroups> GetMasterBuses() 
    {
        return MasterBuses;
    }

    API_PROPERTY() void SetMasterBuses(Dictionary<String, BusGroups>& Value);

public:
    /// <summary>
    /// Gets the value of the mixer volume variable
    /// </summary>
    /// <param name="nameChannel">The mixer channel name.</param>
    /// <returns>The value.</returns>
    API_FUNCTION() const Variant& GetMixerChannelVolume(const StringView& nameChannel) const;

    // <summary>
    /// Sets the value of the global variable (it must be added first).
    /// </summary>
    /// <param name="nameChannel">The mixer channel name.</param>
    /// <param name="value">The mixer volume value.</param>
    API_FUNCTION() void SetMixerChannelVolume(const StringView& nameChannel, const Variant& value);

    /// <summary>
    /// Resets the variables values to default values.
    /// </summary>
    API_FUNCTION() void ResetMixer();

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
