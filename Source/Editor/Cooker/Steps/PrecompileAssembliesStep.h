// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

/// <summary>
/// Optional step used only on selected platform that precompiles C# script assemblies. Uses Ahead of Time Compilation (AOT) feature.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class PrecompileAssembliesStep : public GameCooker::BuildStep
{
public:

    // [BuildStep]
    void OnBuildStarted(CookingData& data) override;
    bool Perform(CookingData& data) override;
};
