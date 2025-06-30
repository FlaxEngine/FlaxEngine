// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Variant.h"
#include "MixerGroupChannels.h"

/// <summary>
/// 
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API AudioMixer : public BinaryAsset 
{
    DECLARE_BINARY_ASSET_HEADER(AudioMixer,2);

public:
    /// <summary>
    /// The mixer variable data.
    /// </summary>
    struct MixerVariable
    {
        /// <summary>
        /// The current value.
        /// </summary>
        Variant Value;

        /// <summary>
        /// The default value.
        /// </summary>
        Variant DefaultValue;
    };

public:
    /// <summary>
    /// The collection of audio mixer variables identified by the name.
    /// </summary>
    Dictionary<String, MixerVariable> AudioMixerVariables;

public:
    API_FUNCTION() void MixerInit();

    /// <summary>
    /// Gets the values (run-time).
    /// </summary>
    /// <returns>The values (run-time).</returns>
    API_PROPERTY() Dictionary<String, Variant> GetMixerVariablesValues() const;

    /// <summary>
    /// Sets the values (run-time).
    /// </summary>
    /// <param name="values">The values (run-time).</param>
    API_PROPERTY() void SetMixerVariablesValues(const Dictionary<String, Variant>& values);

    /// <summary>
    /// Gets the default values (edit-time).
    /// </summary>
    /// <returns>The default values (edit-time).</returns>
    API_PROPERTY() Dictionary<String, Variant> GetDefaultValues() const;

    /// <summary>
    /// Sets the default values (edit-time).
    /// </summary>
    /// <param name="values">The default values (edit-time).</param>
    API_PROPERTY() void SetDefaultValues(const Dictionary<String, Variant>& values);

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
    API_FUNCTION() void ResetValues();

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
