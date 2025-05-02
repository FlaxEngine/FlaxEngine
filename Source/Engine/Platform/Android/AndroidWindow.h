// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Implementation of the window class for Android platform.
/// </summary>
class FLAXENGINE_API AndroidWindow : public WindowBase
{
    friend AndroidPlatform;
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="AndroidWindow"/> class.
    /// </summary>
    /// <param name="settings">The initial window settings.</param>
    AndroidWindow(const CreateWindowSettings& settings);

public:

    // [WindowBase]
    void* GetNativePtr() const override;
    void Show() override;
    void Hide() override;
    void SetClientBounds(const Rectangle& clientArea) override;
};

#endif
