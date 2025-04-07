// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "TextureGroup.h"

/// <summary>
/// Content streaming settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings", NoConstructor) class FLAXENGINE_API StreamingSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(StreamingSettings);
    API_AUTO_SERIALIZATION();

public:
    /// <summary>
    /// Textures streaming configuration (per-group).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Textures\")")
    Array<TextureGroup, InlinedAllocation<32>> TextureGroups;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static StreamingSettings* Get();

    // [SettingsBase]
    void Apply() override;
};
