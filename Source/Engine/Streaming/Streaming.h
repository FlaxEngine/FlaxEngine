// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "TextureGroup.h"

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
};
