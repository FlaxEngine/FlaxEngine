// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

/// <summary>
/// Optional step used only on selected platform that precompiles C# script assemblies.
/// Uses Mono Ahead of Time Compilation (AOT) feature.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class PrecompileAssembliesStep : public GameCooker::BuildStep
{
public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
