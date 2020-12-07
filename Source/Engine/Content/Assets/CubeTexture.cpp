// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "CubeTexture.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"

REGISTER_BINARY_ASSET(CubeTexture, "FlaxEngine.CubeTexture", ::New<TextureAssetUpgrader>(), true);

CubeTexture::CubeTexture(const SpawnParams& params, const AssetInfo* info)
    : TextureBase(params, info)
{
}
