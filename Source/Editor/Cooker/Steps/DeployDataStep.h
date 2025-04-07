// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

/// <summary>
/// Engine and game content and data files deployment step.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class DeployDataStep : public GameCooker::BuildStep
{
public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
