// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_GDK

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/Win32/WindowsMinimal.h"

/// <summary>
/// Implementation of the window class for GDK platform.
/// </summary>
class FLAXENGINE_API GDKWindow : public WindowBase
{
    friend GDKPlatform;
private:

    Windows::HWND _handle;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GDKWindow"/> class.
    /// </summary>
    /// <param name="settings">The initial window settings.</param>
    GDKWindow(const CreateWindowSettings& settings);

    /// <summary>
    /// Finalizes an instance of the <see cref="GDKWindow"/> class.
    /// </summary>
    ~GDKWindow();

public:

    /// <summary>
    /// Gets the window handle.
    /// </summary>
    FORCE_INLINE Windows::HWND GetHWND() const
    {
        return _handle;
    }

    /// <summary>
    /// Checks if the window has valid handle created.
    /// </summary>
    FORCE_INLINE bool HasHWND() const
    {
        return _handle != nullptr;
    }

    /// <summary>
    /// Gets the information about screen which contains window.
    /// </summary>
    /// <param name="x">The x position.</param>
    /// <param name="y">The y position.</param>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    void GetScreenInfo(int32& x, int32& y, int32& width, int32& height) const;

public:

    /// <summary>
    /// The Windows messages procedure.
    /// </summary>
    /// <param name="msg">The mMessage.</param>
    /// <param name="wParam">The first parameter.</param>
    /// <param name="lParam">The second parameter.</param>
    /// <returns>The result.</returns>
    Windows::LRESULT WndProc(Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);

private:

    void CheckForWindowResize();
    void UpdateCursor() const;

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
    void SetIsFullscreen(bool isFullscreen) override;
    void SetCursor(CursorType type) override;
};

#endif
