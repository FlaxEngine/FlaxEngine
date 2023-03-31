// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_GDK

#include "Editor/Cooker/PlatformTools.h"

class GDKPlatformSettings;

/// <summary>
/// The GDK platform support tools.
/// </summary>
class GDKPlatformTools : public PlatformTools
{
protected:

    String _gdkPath;

public:

    GDKPlatformTools();

    bool OnPostProcess(CookingData& data, GDKPlatformSettings* platformSettings);

public:

    // [PlatformTools]
    DotNetAOTModes UseAOT() const override;
    bool OnScriptsStepDone(CookingData& data) override;
    bool OnDeployBinaries(CookingData& data) override;
    void OnConfigureAOT(CookingData& data, AotConfig& config) override;
    bool OnPerformAOT(CookingData& data, AotConfig& config, const String& assemblyPath) override;
};

#endif
