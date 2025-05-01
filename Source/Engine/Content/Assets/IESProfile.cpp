// Copyright (c) Wojciech Figat. All rights reserved.

#include "IESProfile.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"

REGISTER_BINARY_ASSET_WITH_UPGRADER(IESProfile, "FlaxEngine.IESProfile", TextureAssetUpgrader, false);

IESProfile::IESProfile(const SpawnParams& params, const AssetInfo* info)
    : TextureBase(params, info)
    , Brightness(0)
    , TextureMultiplier(1)
{
}

bool IESProfile::init(AssetInitData& initData)
{
    // Base
    if (TextureBase::init(initData))
        return true;

    // Get settings from texture header mini-storage
    auto data = (CustomDataLayout*)_texture.GetHeader()->CustomData;
    Brightness = data->Brightness;
    TextureMultiplier = data->TextureMultiplier;
    return false;
}
