// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// Graphics rendering settings.
/// </summary>
class GraphicsSettings : public SettingsBase<GraphicsSettings>
{
public:

    /// <summary>
    /// Enables rendering synchronization with the refresh rate of the display device to avoid "tearing" artifacts.
    /// </summary>
    bool UseVSync = false;

    /// <summary>
    /// Anti Aliasing quality setting.
    /// </summary>
    Quality AAQuality = Quality::Medium;

    /// <summary>
    /// Screen Space Reflections quality setting.
    /// </summary>
    Quality SSRQuality = Quality::Medium;

    /// <summary>
    /// Screen Space Ambient Occlusion quality setting.
    /// </summary>
    Quality SSAOQuality = Quality::Medium;

    /// <summary>
    /// Volumetric Fog quality setting.
    /// </summary>
    Quality VolumetricFogQuality = Quality::High;

    /// <summary>
    /// The shadows quality.
    /// </summary>
    Quality ShadowsQuality = Quality::Medium;

    /// <summary>
    /// The shadow maps quality (textures resolution).
    /// </summary>
    Quality ShadowMapsQuality = Quality::Medium;

    /// <summary>
    /// Enables cascades splits blending for directional light shadows.
    /// </summary>
    bool AllowCSMBlending = false;

public:

    // [SettingsBase]
    void Apply() override;

    void RestoreDefault() final override
    {
        UseVSync = false;
        AAQuality = Quality::Medium;
        SSRQuality = Quality::Medium;
        SSAOQuality = Quality::Medium;
        VolumetricFogQuality = Quality::High;
        ShadowsQuality = Quality::Medium;
        ShadowMapsQuality = Quality::Medium;
        AllowCSMBlending = false;
    }

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
