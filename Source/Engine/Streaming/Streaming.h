// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "TextureGroup.h"

class GPUSampler;

// Streaming service statistics container.
API_STRUCT(NoDefault) struct FLAXENGINE_API StreamingStats
{
DECLARE_SCRIPTING_TYPE_MINIMAL(StreamingStats);
    // Amount of active streamable resources.
    API_FIELD() int32 ResourcesCount = 0;
    // Amount of resources that are during streaming in (target residency is higher that the current). Zero if all resources are streamed in.
    API_FIELD() int32 StreamingResourcesCount = 0;
};

/// <summary>
/// The content streaming service.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Streaming
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Streaming);

    /// <summary>
    /// Textures streaming configuration (per-group).
    /// </summary>
    API_FIELD() static Array<TextureGroup, InlinedAllocation<32>> TextureGroups;

    /// <summary>
    /// Gets streaming statistics.
    /// </summary>
    API_PROPERTY(Attributes="DebugCommand") static StreamingStats GetStats();

    /// <summary>
    /// Requests the streaming update for all the loaded resources. Use it to refresh content streaming after changing configuration.
    /// </summary>
    API_FUNCTION() static void RequestStreamingUpdate();

    /// <summary>
    /// Gets the texture sampler for a given texture group. Sampler objects is managed and cached by streaming service. Returned value is always valid (uses fallback object).
    /// </summary>
    /// <param name="index">The texture group index.</param>
    /// <returns>The texture sampler (always valid).</returns>
    API_FUNCTION() static GPUSampler* GetTextureGroupSampler(int32 index);
};
