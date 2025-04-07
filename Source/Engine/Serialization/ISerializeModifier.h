// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Guid.h"
#include "FlaxEngine.Gen.h"

/// <summary>
/// Object serialization modification base class. Allows to extend the serialization process by custom effects like object ids mapping.
/// </summary>
class FLAXENGINE_API ISerializeModifier
{
public:

    /// <summary>
    /// Number of engine build when data was serialized. Useful to upgrade data from the older storage format.
    /// </summary>
    uint32 EngineBuild = FLAXENGINE_VERSION_BUILD;

    // Utility for scene deserialization to track currently mapped in Prefab Instance object IDs into IdsMapping.
    int32 CurrentInstance = -1;

    /// <summary>
    /// The object IDs mapping. Key is a serialized object id, value is mapped value to use.
    /// </summary>
    Dictionary<Guid, Guid> IdsMapping;
};
