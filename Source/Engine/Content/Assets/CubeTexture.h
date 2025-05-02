// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/TextureBase.h"

/// <summary>
/// Cube texture asset contains 6 images that is usually stored on a GPU as a cube map (one slice per each axis direction).
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API CubeTexture : public TextureBase
{
    DECLARE_BINARY_ASSET_HEADER(CubeTexture, TexturesSerializedVersion);
};
