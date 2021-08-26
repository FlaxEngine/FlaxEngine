// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_UWP

#include "../../PlatformTools.h"

/// <summary>
/// The Universal Windows Platform (UWP) platform support tools.
/// </summary>
class UWPPlatformTools : public PlatformTools
{
public:

    // [PlatformTools]
    bool UseAOT() const override;
    bool OnScriptsStepDone(CookingData& data) override;
    bool OnDeployBinaries(CookingData& data) override;
    void OnConfigureAOT(CookingData& data, AotConfig& config) override;
    bool OnPerformAOT(CookingData& data, AotConfig& config, const String& assemblyPath) override;
    bool OnPostProcess(CookingData& data) override;
};

/// <summary>
/// The Windows Store platform support tools.
/// </summary>
class WSAPlatformTools : public UWPPlatformTools
{
private:

    ArchitectureType _arch;

public:

    WSAPlatformTools(ArchitectureType arch)
        : _arch(arch)
    {
    }

public:

    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
};


#endif
