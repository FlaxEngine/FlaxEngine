// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Implementation of the window class for Linux platform.
/// </summary>
class FLAXENGINE_API LinuxWindow : public WindowBase
{
	friend LinuxPlatform;
public:

	typedef unsigned long HandleType;

private:

	bool _resizeDisabled, _focusOnMapped = false, _dragOver = false;
	float _opacity = 1.0f;
	HandleType _window;

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="LinuxWindow"/> class.
	/// </summary>
	/// <param name="settings">The initial window settings.</param>
	LinuxWindow(const CreateWindowSettings& settings);

	/// <summary>
	/// Finalizes an instance of the <see cref="LinuxWindow"/> class.
	/// </summary>
	~LinuxWindow();

public:

	/// <summary>
	/// Gets information about screen which contains window
	/// </summary>
	/// <param name="x">X position</param>
	/// <param name="y">Y position</param>
	/// <param name="width">Width</param>
	/// <param name="height">Height</param>
	void GetScreenInfo(int32& x, int32& y, int32& width, int32& height) const;

	void CheckForWindowResize();

	void OnKeyPress(void* event);
	void OnKeyRelease(void* event);
	void OnButtonPress(void* event);
	void OnButtonRelease(void* event);
	void OnMotionNotify(void* event);
	void OnLeaveNotify(void* event);
	void OnConfigureNotify(void* event);

private:

	void Maximize(bool enable);
	void Minimize(bool enable);
	bool IsWindowMapped();

public:

	// [WindowBase]
	void* GetNativePtr() const override;
	void Show() override;
	void Hide() override;
	void Minimize() override;
	void Maximize() override;
	void Restore() override;
	bool IsClosed() const override;
    bool IsForegroundWindow() const override;
	void BringToFront(bool force = false) override;
	void SetClientBounds(const Rectangle& clientArea) override;
	void SetPosition(const Float2& position) override;
	void SetClientPosition(const Float2& position) override;
	Float2 GetPosition() const override;
	Float2 GetSize() const override;
	Float2 GetClientSize() const override;
	Float2 ScreenToClient(const Float2& screenPos) const override;
	Float2 ClientToScreen(const Float2& clientPos) const override;
	void FlashWindow() override;
	float GetOpacity() const override;
	void SetOpacity(float opacity) override;
	void Focus() override;
	void SetTitle(const StringView& title) override;
	DragDropEffect DoDragDrop(const StringView& data) override;
	void StartTrackingMouse(bool useMouseScreenOffset) override;
	void EndTrackingMouse() override;
	void SetCursor(CursorType type) override;
    void SetIcon(TextureData& icon) override;
};

#endif
