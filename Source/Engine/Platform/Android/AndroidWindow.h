// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Implementation of the window class for Android platform.
/// </summary>
class AndroidWindow : public WindowBase
{
    friend AndroidPlatform;
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="AndroidWindow"/> class.
    /// </summary>
    /// <param name="settings">The initial window settings.</param>
    AndroidWindow(const CreateWindowSettings& settings);

    /// <summary>
    /// Finalizes an instance of the <see cref="AndroidWindow"/> class.
    /// </summary>
    ~AndroidWindow();

public:

    // [Window]
    void* GetNativePtr() const override;
    void Show() override;
    void Hide() override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    bool IsClosed() const override;
    void BringToFront(bool force = false) override;
    void SetClientBounds(const Rectangle& clientArea) override;
    void SetPosition(const Vector2& position) override;
    void SetClientPosition(const Vector2& position) override;
    Vector2 GetPosition() const override;
    Vector2 GetSize() const override;
    Vector2 GetClientSize() const override;
    Vector2 ScreenToClient(const Vector2& screenPos) const override;
    Vector2 ClientToScreen(const Vector2& clientPos) const override;
    void SetTitle(const StringView& title) override;
    DragDropEffect DoDragDrop(const StringView& data) override;
    void StartTrackingMouse(bool useMouseScreenOffset) override;
    void EndTrackingMouse() override;
};

#endif
