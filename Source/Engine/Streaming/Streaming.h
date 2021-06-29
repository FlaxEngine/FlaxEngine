// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "TextureGroup.h"

class GPUSampler;

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
