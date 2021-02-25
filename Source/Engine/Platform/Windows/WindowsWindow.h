// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"
#include "WindowsInput.h"

/// <summary>
/// Implementation of the window class for Windows platform
/// </summary>
class WindowsWindow : public WindowBase
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
    bool _isDuringMaximize = false;
    Windows::HANDLE _monitor = nullptr;
    Vector2 _clientSize;
    int32 _regionWidth = 0, _regionHeight = 0;

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
    void UpdateCursor() const;
    void UpdateRegion();

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
    void BringToFront(bool force = false) override;
    void SetClientBounds(const Rectangle& clientArea) override;
    void SetPosition(const Vector2& position) override;
    void SetClientPosition(const Vector2& position) override;
    void SetIsFullscreen(bool isFullscreen) override;
    Vector2 GetPosition() const override;
    Vector2 GetSize() const override;
    Vector2 GetClientSize() const override;
    Vector2 ScreenToClient(const Vector2& screenPos) const override;
    Vector2 ClientToScreen(const Vector2& clientPos) const override;
    void FlashWindow() override;
    float GetOpacity() const override;
    void SetOpacity(float opacity) override;
    void Focus() override;
    void SetTitle(const StringView& title) override;
    DragDropEffect DoDragDrop(const StringView& data) override;
    void StartTrackingMouse(bool useMouseScreenOffset) override;
    void EndTrackingMouse() override;
    void SetCursor(CursorType type) override;

#if USE_EDITOR

    // [IUnknown]
    Windows::HRESULT __stdcall QueryInterface(const Windows::IID& id, void** ppvObject) override;
    Windows::ULONG __stdcall AddRef() override;
    Windows::ULONG __stdcall Release() override;

    // [IDropTarget]
    Windows::HRESULT __stdcall DragEnter(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect) override;
    Windows::HRESULT __stdcall DragOver(Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect) override;
    Windows::HRESULT __stdcall DragLeave() override;
    Windows::HRESULT __stdcall Drop(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect) override;

#endif
};

#endif
