// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

/// <summary>
/// Final cooking step that can perform custom set of actions on generated game data.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class PostProcessStep : public GameCooker::BuildStep
{
public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
