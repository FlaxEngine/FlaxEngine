// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    uint32 EngineBuild;

    /// <summary>
    /// The object IDs mapping. Key is a serialized object id, value is mapped value to use.
    /// </summary>
    Dictionary<Guid, Guid> IdsMapping;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ISerializeModifier"/> class.
    /// </summary>
    ISerializeModifier()
        : EngineBuild(FLAXENGINE_VERSION_BUILD)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ISerializeModifier"/> class.
    /// </summary>
    /// <param name="engineBuild">The engine build.</param>
    ISerializeModifier(uint32 engineBuild)
        : EngineBuild(engineBuild)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ISerializeModifier"/> class.
    /// </summary>
    virtual ~ISerializeModifier()
    {
    }
};
