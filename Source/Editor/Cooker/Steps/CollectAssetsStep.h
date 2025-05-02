// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

class Asset;

/// <summary>
/// Cooking step that uses the root assets collection to find all dependant assets to include in the build.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class CollectAssetsStep : public GameCooker::BuildStep
{
public:
    // [BuildStep]
    bool Perform(CookingData& data) override;
};
