// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Implementation of the window class for iOS platform.
/// </summary>
class FLAXENGINE_API iOSWindow : public WindowBase
{
private:

    Float2 _clientSize;
    void* _window;
    void* _layer;
    void* _view;
    void* _viewController;

public:

	iOSWindow(const CreateWindowSettings& settings);
	~iOSWindow();

    void CheckForResize(float width, float height);
    void* GetViewController() const { return _viewController; }

public:

	// [Window]
    void* GetNativePtr() const override;
    void Show() override;
    bool IsClosed() const override;
    bool IsForegroundWindow() const override;
    void BringToFront(bool force = false) override;
	void SetClientBounds(const Rectangle& clientArea) override;
	void SetPosition(const Float2& position) override;
	Float2 GetPosition() const override;
	Float2 GetSize() const override;
	Float2 GetClientSize() const override;
	Float2 ScreenToClient(const Float2& screenPos) const override;
	Float2 ClientToScreen(const Float2& clientPos) const override;
    void SetIsFullscreen(bool isFullscreen) override;
    void SetTitle(const StringView& title) override;
};

#endif
