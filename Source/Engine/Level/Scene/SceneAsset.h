// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/JsonAsset.h"

/// <summary>
/// The scene asset.
/// </summary>
API_CLASS(NoSpawn) class SceneAsset : public JsonAsset
{
    DECLARE_ASSET_HEADER(SceneAsset);
protected:
    bool IsInternalType() const override;
};
