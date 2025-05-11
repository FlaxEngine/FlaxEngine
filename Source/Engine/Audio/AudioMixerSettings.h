// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"

/// <summary>
/// Audio Mixer settings container
/// </summary>
API_CLASS(sealed, Namespace = "") class FLAXENGINE_API AudioMixerSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AudioMixerSettings);
    API_AUTO_SERIALIZATION();




    public:
        /// <summary>
        /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
        /// </summary>
        static AudioMixerSettings* Get();

        // [SettingsBase]
        void Apply() override;
};
