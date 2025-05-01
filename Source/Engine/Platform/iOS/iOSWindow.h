// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS

#include "Engine/Platform/Base/WindowBase.h"

/// <summary>
/// Implementation of the window class for iOS platform.
/// </summary>
class FLAXENGINE_API iOSWindow : public WindowBase
{
public:
	iOSWindow(const CreateWindowSettings& settings);
	~iOSWindow();

    void CheckForResize(float width, float height);

public:
	// [WindowBase]
    void* GetNativePtr() const override;
    void Show() override;
    bool IsClosed() const override;
    bool IsForegroundWindow() const override;
    void BringToFront(bool force = false) override;
    void SetIsFullscreen(bool isFullscreen) override;
};

#endif
