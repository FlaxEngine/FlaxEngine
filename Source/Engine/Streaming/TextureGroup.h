// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/ISerializable.h"
#if USE_EDITOR
#include "Engine/Core/Collections/Dictionary.h"
#endif

/// <summary>
/// Settings container for a group of textures. Defines the data streaming options and resource quality.
/// </summary>
API_STRUCT() struct TextureGroup : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_MINIMAL(TextureGroup);

    /// <summary>
    /// The name of the group.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    String Name;

    /// <summary>
    /// The quality scale factor applied to textures in this group. Can be used to increase or decrease textures resolution. In range 0-1 where 0 means lowest quality, 1 means full quality.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), Limit(0, 1)")
    float Quality = 1.0f;

    /// <summary>
    /// The quality scale factor applied when texture is invisible for some time (defined by TimeToInvisible). Used to decrease texture quality when it's not rendered.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(25), Limit(0, 1)")
    float QualityIfInvisible = 0.5f;

    /// <summary>
    /// The time (in seconds) after which texture is considered to be invisible (if it's not rendered by a certain amount of time).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(26), Limit(0)")
    float TimeToInvisible = 20.0f;

    /// <summary>
    /// The minimum amount of loaded mip levels for textures in this group. Defines the amount of the mips that should be always loaded. Higher values decrease streaming usage and keep more mips loaded.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), Limit(0, 14)")
    int32 MipLevelsMin = 0;

    /// <summary>
    /// The maximum amount of loaded mip levels for textures in this group. Defines the maximum amount of the mips that can be loaded. Overriden per-platform. Lower values reduce textures quality and improve performance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), Limit(1, 14)")
    int32 MipLevelsMax = 14;

    /// <summary>
    /// The loaded mip levels bias for textures in this group. Can be used to increase or decrease quality of the streaming for textures in this group (eg. bump up the quality during cinematic sequence).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), Limit(-14, 14)")
    int32 MipLevelsBias = 0;

#if USE_EDITOR
    /// <summary>
    /// The per-platform maximum amount of mip levels for textures in this group. Can be used to strip textures quality when cooking game for a target platform.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50)")
    Dictionary<PlatformType, int32> MipLevelsMaxPerPlatform;
#endif
};
