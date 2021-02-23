// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"

/// <summary>
/// Layers and objects tags settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API LayersAndTagsSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(LayersAndTagsSettings);
public:

    /// <summary>
    /// The tag names.
    /// </summary>
    Array<String> Tags;

    /// <summary>
    /// The layer names.
    /// </summary>
    String Layers[32];

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static LayersAndTagsSettings* Get();

    // [SettingsBase]
    void Apply() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
