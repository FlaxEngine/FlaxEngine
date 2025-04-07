// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_LINUX

#include "../../PlatformTools.h"

/// <summary>
/// The Linux platform support tools.
/// </summary>
class LinuxPlatformTools : public PlatformTools
{
public:

    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
    bool UseSystemDotnet() const override;
    bool OnDeployBinaries(CookingData& data) override;
    void OnRun(CookingData& data, String& executableFile, String& commandLineFormat, String& workingDir) override;
};

#endif
