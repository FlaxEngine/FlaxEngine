// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "TextureGroup.h"
#include "StreamableResourcesCollection.h"

/// <summary>
/// The content streaming service.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Streaming
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Streaming);

    /// <summary>
    /// List with all resources
    /// </summary>
    static StreamableResourcesCollection Resources;

    /// <summary>
    /// Textures streaming configuration (per-group).
    /// </summary>
    API_FIELD() static Array<TextureGroup, InlinedAllocation<32>> TextureGroups;
};
