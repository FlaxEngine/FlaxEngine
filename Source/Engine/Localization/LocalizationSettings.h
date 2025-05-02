// Copyright (c) Wojciech Figat. All rights reserved.

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
    API_FIELD(Attributes="Collection(Display = CollectionAttribute.DisplayType.Inline)")
    Array<AssetReference<LocalizedStringTable>> LocalizedStringTables;

    /// <summary>
    /// The default fallback language to use if localization system fails to pick the system locale language (eg. en-GB).
    /// </summary>
    API_FIELD()
    String DefaultFallbackLanguage;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static LocalizationSettings* Get();

    // [SettingsBase]
    void Apply() override;
#if USE_EDITOR
    void Serialize(SerializeStream& stream, const void* otherObj) override;
#endif
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
