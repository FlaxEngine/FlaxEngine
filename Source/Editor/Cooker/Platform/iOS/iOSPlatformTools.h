// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_IOS

#include "../../PlatformTools.h"

/// <summary>
/// The iOS platform support tools.
/// </summary>
class iOSPlatformTools : public PlatformTools
{
public:
    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
    bool IsNativeCodeFile(CookingData& data, const String& file) override;
    void OnBuildStarted(CookingData& data) override;
    bool OnPostProcess(CookingData& data) override;
};

#endif
