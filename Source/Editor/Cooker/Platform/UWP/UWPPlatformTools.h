// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_UWP

#include "../../PlatformTools.h"

/// <summary>
/// The Universal Windows Platform (UWP) platform support tools.
/// </summary>
class UWPPlatformTools : public PlatformTools
{
private:

    ArchitectureType _arch;

public:

    UWPPlatformTools(ArchitectureType arch)
        : _arch(arch)
    {
    }

public:

    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
    DotNetAOTModes UseAOT() const override;
    bool OnDeployBinaries(CookingData& data) override;
    bool OnPostProcess(CookingData& data) override;
};

#endif
