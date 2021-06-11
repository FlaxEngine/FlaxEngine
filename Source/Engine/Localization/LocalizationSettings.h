// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Content/AssetReference.h"
#include "LocalizedStringTable.h"

/// <summary>
/// Game localization and internalization settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API LocalizationSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(LocalizationSettings);
public:
    /// <summary>
    /// The list of the string localization tables used by the game.
    /// </summary>
    API_FIELD()
    Array<AssetReference<LocalizedStringTable>> LocalizedStringTables;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static LocalizationSettings* Get();

    // [SettingsBase]
    void Apply() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
