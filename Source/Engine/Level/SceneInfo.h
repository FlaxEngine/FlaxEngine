// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Renderer/Lightmaps.h"

/// <summary>
/// Scene information metadata
/// </summary>
class SceneInfo : public Object, public ISerializable
{
public:
    /// <summary>
    /// Scene title
    /// </summary>
    String Title;

    /// <summary>
    /// Scene description
    /// </summary>
    String Description;

    /// <summary>
    /// Scene copyrights note
    /// </summary>
    String Copyright;

public:
    /// <summary>
    /// Array with cached lightmaps ID for the scene
    /// </summary>
    Array<SavedLightmapInfo> Lightmaps;

    /// <summary>
    /// Custom settings for static lightmaps baking
    /// </summary>
    LightmapSettings LightmapSettings;

public:
    // [Object]
    String ToString() const override;

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
