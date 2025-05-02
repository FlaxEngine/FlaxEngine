// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_MAC

#include "../../PlatformTools.h"

/// <summary>
/// The Mac platform support tools.
/// </summary>
class MacPlatformTools : public PlatformTools
{
private:
    ArchitectureType _arch;

public:

    MacPlatformTools(ArchitectureType arch);

    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
    bool UseSystemDotnet() const override;
    bool IsNativeCodeFile(CookingData& data, const String& file) override;
    void OnBuildStarted(CookingData& data) override;
    bool OnPostProcess(CookingData& data) override;
    void OnRun(CookingData& data, String& executableFile, String& commandLineFormat, String& workingDir) override;
};

#endif
