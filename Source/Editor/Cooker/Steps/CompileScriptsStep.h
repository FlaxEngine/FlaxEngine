// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"

/// <summary>
/// Game scripts compilation step. Outputs proper assemblies compiled to the target platform.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class CompileScriptsStep : public GameCooker::BuildStep
{
private:

    Array<String, FixedAllocation<8>> _extensionsToSkip;
    Array<String, InlinedAllocation<32>> _deployedBuilds;

    bool DeployBinaries(CookingData& data, const String& path, const String& projectFolderPath);

public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
