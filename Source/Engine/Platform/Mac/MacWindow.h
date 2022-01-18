// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Implementation of the window class for Mac platform.
/// </summary>
class MacWindow : public WindowBase
{
private:

    Vector2 _clientSize;
    void* _window;

public:

	MacWindow(const CreateWindowSettings& settings);
	~MacWindow();

    void CheckForResize(float width, float height);

public:

	// [Window]
    void* GetNativePtr() const override;
    void Show() override;
    void Hide() override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    bool IsClosed() const override;
    bool IsForegroundWindow() const override;
    void BringToFront(bool force) override;
    void SetIsFullscreen(bool isFullscreen) override;
    void SetOpacity(float opacity) override;
    void Focus() override;
    void SetTitle(const StringView& title) override;
};

#endif
