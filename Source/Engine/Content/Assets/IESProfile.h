// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/TextureBase.h"

/// <summary>
/// Contains IES profile texture used by the lights to simulate real life bulb light emission.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API IESProfile : public TextureBase
{
    DECLARE_BINARY_ASSET_HEADER(IESProfile, TexturesSerializedVersion);

public:
    struct CustomDataLayout
    {
        float Brightness;
        float TextureMultiplier;
    };

public:
    /// <summary>
    /// The light brightness in Lumens, imported from IES profile.
    /// </summary>
    API_FIELD() float Brightness;

    /// <summary>
    /// The multiplier to map texture value to result to integrate over the sphere to 1.
    /// </summary>
    API_FIELD() float TextureMultiplier;

protected:
    // [BinaryAsset]
    bool init(AssetInitData& initData) override;
};
