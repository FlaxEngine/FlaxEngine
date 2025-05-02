// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_TOOLS_ANDROID

#include "../../PlatformTools.h"

/// <summary>
/// The Android platform support tools.
/// </summary>
class AndroidPlatformTools : public PlatformTools
{
private:

    ArchitectureType _arch;

public:

    AndroidPlatformTools(ArchitectureType arch)
        : _arch(arch)
    {
    }

public:

    // [PlatformTools]
    const Char* GetDisplayName() const override;
    const Char* GetName() const override;
    PlatformType GetPlatform() const override;
    ArchitectureType GetArchitecture() const override;
    PixelFormat GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format) override;
    void LoadCache(CookingData& data, IBuildCache* cache, const Span<byte>& bytes) override;
    Array<byte> SaveCache(CookingData& data, IBuildCache* cache) override;
    void OnBuildStarted(CookingData& data) override;
    bool OnPostProcess(CookingData& data) override;
};

#endif
