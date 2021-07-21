// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Animations/AnimationData.h"

class SkinnedModel;

/// <summary>
/// Asset that contains an animation spline represented by a set of keyframes, each representing an endpoint of a linear curve.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Animation : public BinaryAsset
{
DECLARE_BINARY_ASSET_HEADER(Animation, 1);

    /// <summary>
    /// Contains basic information about the animation asset contents.
    /// </summary>
    API_STRUCT() struct InfoData
    {
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(InfoData);

        /// <summary>
        /// Length of the animation in seconds.
        /// </summary>
        API_FIELD() float Length;

        /// <summary>
        /// Amount of animation frames (some curve tracks may use less keyframes).
        /// </summary>
        API_FIELD() int32 FramesCount;

        /// <summary>
        /// Amount of animation channel tracks.
        /// </summary>
        API_FIELD() int32 ChannelsCount;

        /// <summary>
        /// The total amount of keyframes in the animation tracks.
        /// </summary>
        API_FIELD() int32 KeyframesCount;

        /// <summary>
        /// The estimated memory usage (in bytes) of the animation (all tracks and keyframes size in memory).
        /// </summary>
        API_FIELD() int32 MemoryUsage;
    };

public:

    /// <summary>
    /// The animation data.
    /// </summary>
    AnimationData Data;

    /// <summary>
    /// Contains the mapping for every skeleton node to the animation data channels.
    /// Can be used for a simple lookup or to check if a given node is animated (unused nodes are using -1 index).
    /// </summary>
    typedef Array<int32> NodeToChannel;

    /// <summary>
    /// The skeleton nodes to animation channel indices mapping cache. Use it as read-only. It's being maintained internally by the asset.
    /// </summary>
    Dictionary<SkinnedModel*, NodeToChannel> MappingCache;

public:

    /// <summary>
    /// Gets the length of the animation (in seconds).
    /// </summary>
    API_PROPERTY() float GetLength() const
    {
        return IsLoaded() ? Data.GetLength() : 0.0f;
    }

    /// <summary>
    /// Gets the duration of the animation (in frames).
    /// </summary>
    API_PROPERTY() float GetDuration() const
    {
        return (float)Data.Duration;
    }

    /// <summary>
    /// Gets the amount of the animation frames per second.
    /// </summary>
    API_PROPERTY() float GetFramesPerSecond() const
    {
        return (float)Data.FramesPerSecond;
    }

    /// <summary>
    /// Gets the animation clip info.
    /// </summary>
    API_PROPERTY() InfoData GetInfo() const;

    /// <summary>
    /// Clears the skeleton mapping cache.
    /// </summary>
    void ClearCache();

    /// <summary>
    /// Clears the skeleton mapping cache.
    /// </summary>
    /// <param name="obj">The target skinned model to get mapping to its skeleton.</param>
    /// <returns>The cached node-to-channel mapping for the fast animation sampling for the skinned model skeleton nodes.</returns>
    const NodeToChannel* GetMapping(SkinnedModel* obj);

#if USE_EDITOR

    /// <summary>
    /// Gets the animation as serialized timeline data. Used to show it in Editor.
    /// </summary>
    /// <param name="result">The output timeline data container. Empty if failed to load.</param>
    API_FUNCTION() void LoadTimeline(API_PARAM(Out) BytesContainer& result) const;

    /// <summary>
    /// Saves the serialized timeline data to the asset as animation.
    /// </summary>
    /// <remarks>The cannot be used by virtual assets.</remarks>
    /// <param name="data">The timeline data container.</param>
    /// <returns><c>true</c> failed to save data; otherwise, <c>false</c>.</returns>
    API_FUNCTION() bool SaveTimeline(BytesContainer& data);

    /// <summary>
    /// Saves the animation data to the asset. Supported only in Editor.
    /// </summary>
    /// <remarks>The cannot be used by virtual assets.</remarks>
    /// <returns><c>true</c> failed to save data; otherwise, <c>false</c>.</returns>
    bool Save(const StringView& path = StringView::Empty);

#endif

private:

    void OnSkinnedModelUnloaded(Asset* obj);

protected:

    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
