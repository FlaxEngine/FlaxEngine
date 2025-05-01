// Copyright (c) Wojciech Figat. All rights reserved.

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
    DotNetAOTModes UseAOT() const override;
    PixelFormat GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format) override;
    void LoadCache(CookingData& data, IBuildCache* cache, const Span<byte>& bytes) override;
    Array<byte> SaveCache(CookingData& data, IBuildCache* cache) override;
    bool IsNativeCodeFile(CookingData& data, const String& file) override;
    void OnBuildStarted(CookingData& data) override;
    bool OnPostProcess(CookingData& data) override;
};

#endif
