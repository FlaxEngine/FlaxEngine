// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// Graphics rendering settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API GraphicsSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(GraphicsSettings);
public:

    /// <summary>
    /// Enables rendering synchronization with the refresh rate of the display device to avoid "tearing" artifacts.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(false), EditorDisplay(\"General\", \"Use V-Sync\")")
    bool UseVSync = false;

    /// <summary>
    /// Anti Aliasing quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\", \"AA Quality\")")
    Quality AAQuality = Quality::Medium;

    /// <summary>
    /// Screen Space Reflections quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1100), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\", \"SSR Quality\")")
    Quality SSRQuality = Quality::Medium;

    /// <summary>
    /// Screen Space Ambient Occlusion quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1200), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\", \"SSAO Quality\")")
    Quality SSAOQuality = Quality::Medium;

    /// <summary>
    /// Volumetric Fog quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1250), DefaultValue(Quality.High), EditorDisplay(\"Quality\")")
    Quality VolumetricFogQuality = Quality::High;

    /// <summary>
    /// The shadows quality.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1300), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\")")
    Quality ShadowsQuality = Quality::Medium;

    /// <summary>
    /// The shadow maps quality (textures resolution).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1310), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\")")
    Quality ShadowMapsQuality = Quality::Medium;

    /// <summary>
    /// Enables cascades splits blending for directional light shadows.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1320), DefaultValue(false), EditorDisplay(\"Quality\", \"Allow CSM Blending\")")
    bool AllowCSMBlending = false;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static GraphicsSettings* Get();

    // [SettingsBase]
    void Apply() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(UseVSync);
        DESERIALIZE(AAQuality);
        DESERIALIZE(SSRQuality);
        DESERIALIZE(SSAOQuality);
        DESERIALIZE(VolumetricFogQuality);
        DESERIALIZE(ShadowsQuality);
        DESERIALIZE(ShadowMapsQuality);
        DESERIALIZE(AllowCSMBlending);
    }
};
