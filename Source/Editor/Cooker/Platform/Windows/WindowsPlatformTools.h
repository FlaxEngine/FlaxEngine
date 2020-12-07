// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_WINDOWS

#include "../../PlatformTools.h"

/// <summary>
/// The Windows platform support tools.
/// </summary>
class WindowsPlatformTools : public PlatformTools
{
private:

    ArchitectureType _arch;

public:

    WindowsPlatformTools(ArchitectureType arch)
        : _arch(arch)
    {
    }

public:

    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
    bool OnDeployBinaries(CookingData& data) override;
};

#endif
