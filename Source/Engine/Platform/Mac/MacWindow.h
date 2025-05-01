// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Math/Vector2.h"

/// <summary>
/// Implementation of the window class for Mac platform.
/// </summary>
class FLAXENGINE_API MacWindow : public WindowBase
{
private:
    void* _window = nullptr;
    void* _view = nullptr;
    bool _isMouseOver = false;
    Float2 _mouseTrackPos = Float2::Minimum;
    String _dragText;

public:
	MacWindow(const CreateWindowSettings& settings);
	~MacWindow();

    void CheckForResize(float width, float height);
    void SetIsMouseOver(bool value);
    const String& GetDragText() const
    {
        return _dragText;
    }

public:
	// [WindowBase]
    void* GetNativePtr() const override;
    void OnUpdate(float dt) override;
    void Show() override;
    void Hide() override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    bool IsForegroundWindow() const override;
    void BringToFront(bool force = false) override;
	void SetClientBounds(const Rectangle& clientArea) override;
	void SetPosition(const Float2& position) override;
	Float2 GetPosition() const override;
	Float2 GetSize() const override;
	Float2 GetClientSize() const override;
	Float2 ScreenToClient(const Float2& screenPos) const override;
	Float2 ClientToScreen(const Float2& clientPos) const override;
    void FlashWindow() override;
    void SetIsFullscreen(bool isFullscreen) override;
    void SetOpacity(float opacity) override;
    void Focus() override;
    void SetTitle(const StringView& title) override;
	DragDropEffect DoDragDrop(const StringView& data) override;
    void StartTrackingMouse(bool useMouseScreenOffset) override;
    void EndTrackingMouse() override;
	void SetCursor(CursorType type) override;
};

#endif
