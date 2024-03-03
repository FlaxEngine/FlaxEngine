// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

class Asset;

/// <summary>
/// Cooking step that uses the root assets collection to find all dependant assets to include in the build.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class CollectAssetsStep : public GameCooker::BuildStep
{
private:

    Array<Guid> _assetsQueue;
    Array<Guid> _references;

    bool Process(CookingData& data, Asset* asset);

public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
