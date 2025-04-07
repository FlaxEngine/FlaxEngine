// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Settings.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// The main game engine configuration service. Loads and applies game configuration.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API GameSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(GameSettings);

public:
    /// <summary>
    /// The product full name.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    String ProductName;

    /// <summary>
    /// The company full name.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"General\")")
    String CompanyName;

    /// <summary>
    /// The copyright note used for content signing (eg. source code header).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(15), EditorDisplay(\"General\")")
    String CopyrightNotice;

    /// <summary>
    /// The default application icon.
    /// </summary>
    Guid Icon = Guid::Empty;

    /// <summary>
    /// Reference to the first scene to load on a game startup.
    /// </summary>
    Guid FirstScene = Guid::Empty;

    /// <summary>
    /// True if skip showing splash screen image on the game startup.
    /// </summary>
    bool NoSplashScreen = false;

    /// <summary>
    /// Reference to the splash screen image to show on a game startup.
    /// </summary>
    Guid SplashScreen = Guid::Empty;

    /// <summary>
    /// The custom settings to use with a game. Can be specified by the user to define game-specific options and be used by the external plugins (used as key-value pair).
    /// </summary>
    Dictionary<String, Guid> CustomSettings;

public:
    // Settings containers
    Guid Time;
    Guid Audio;
    Guid LayersAndTags;
    Guid Physics;
    Guid Input;
    Guid Graphics;
    Guid Network;
    Guid Navigation;
    Guid Localization;
    Guid GameCooking;
    Guid Streaming;

    // Per-platform settings containers
    Guid WindowsPlatform;
    Guid UWPPlatform;
    Guid LinuxPlatform;
    Guid PS4Platform;
    Guid XboxOnePlatform;
    Guid XboxScarlettPlatform;
    Guid AndroidPlatform;
    Guid SwitchPlatform;
    Guid PS5Platform;
    Guid MacPlatform;
    Guid iOSPlatform;

public:
    /// <summary>
    /// Gets the instance of the game settings asset (null if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static GameSettings* Get();

    /// <summary>
    /// Loads the game settings (including other settings such as Physics, Input, etc.).
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Load();

    // [SettingsBase]
    void Apply() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
