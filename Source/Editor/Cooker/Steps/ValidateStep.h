// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

/// <summary>
/// Project data validation step. Ensures that game cooking can be started.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class ValidateStep : public GameCooker::BuildStep
{
public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
