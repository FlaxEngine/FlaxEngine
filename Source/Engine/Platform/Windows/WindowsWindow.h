// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS && !PLATFORM_SDL

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"
#include "WindowsInput.h"

/// <summary>
/// Implementation of the window class for Windows platform
/// </summary>
class FLAXENGINE_API WindowsWindow : public WindowBase
#if USE_EDITOR
                      , public Windows::IDropTarget
#endif
{
    friend WindowsPlatform;

private:

    Windows::HWND _handle;
#if USE_EDITOR
    Windows::ULONG _refCount;
#endif
    bool _isResizing = false;
    bool _isSwitchingFullScreen = false;
    bool _trackingMouse = false;
    bool _clipCursorSet = false;
    bool _lastCursorHidden = false;
    int _cursorHiddenSafetyCount = 0;
    bool _isDuringMaximize = false;
    Windows::HANDLE _monitor = nullptr;
    Windows::LONG _clipCursorRect[4];
    int32 _regionWidth = 0, _regionHeight = 0;
    Float2 _minimizedScreenPosition = Float2::Zero;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WindowsWindow"/> class.
    /// </summary>
    /// <param name="settings">The initial window settings.</param>
    WindowsWindow(const CreateWindowSettings& settings);

    /// <summary>
    /// Finalizes an instance of the <see cref="WindowsWindow"/> class.
    /// </summary>
    ~WindowsWindow();

public:

    /// <summary>
    /// Gets the window handle.
    /// </summary>
    /// <returns>The window handle.</returns>
    FORCE_INLINE Windows::HWND GetHWND() const
    {
        return _handle;
    }

    /// <summary>
    /// Checks if the window has valid handle created.
    /// </summary>
    /// <returns>True if has valid handle, otherwise false.</returns>
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
    void UpdateCursor();
    void UpdateRegion();

public:

    // [WindowBase]
    void* GetNativePtr() const override;
    void Show() override;
    void Hide() override;
    void Minimize() override;
    void Maximize() override;
    void SetBorderless(bool isBorderless, bool maximized = false) override;
    void Restore() override;
    bool IsClosed() const override;
    bool IsForegroundWindow() const override;
    void BringToFront(bool force = false) override;
    void SetClientBounds(const Rectangle& clientArea) override;
    void SetPosition(const Float2& position) override;
    void SetClientPosition(const Float2& position) override;
    void SetIsFullscreen(bool isFullscreen) override;
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
    void StartClippingCursor(const Rectangle& bounds) override;
    void EndClippingCursor() override;
    void SetCursor(CursorType type) override;

#if USE_EDITOR

    // [IUnknown]
    Windows::HRESULT __stdcall QueryInterface(const Windows::IID& id, void** ppvObject) override;
    Windows::ULONG __stdcall AddRef() override;
    Windows::ULONG __stdcall Release() override;

    // [Windows::IDropTarget]
    Windows::HRESULT __stdcall DragEnter(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect) override;
    Windows::HRESULT __stdcall DragOver(Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect) override;
    Windows::HRESULT __stdcall DragLeave() override;
    Windows::HRESULT __stdcall Drop(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect) override;

#endif
};

#endif
