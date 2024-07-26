// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_SDL

#include "Engine/Platform/Base/WindowBase.h"

struct SDL_Window;
union SDL_Event;
#if PLATFORM_LINUX
class MessageBox;
#endif

/// <summary>
/// Implementation of the window class for SDL platform
/// </summary>
class FLAXENGINE_API SDLWindow : public WindowBase
#if USE_EDITOR && PLATFORM_WINDOWS
    , public Windows::IDropTarget
#endif
{
    friend SDLPlatform;
#if PLATFORM_LINUX
    friend LinuxPlatform;
    friend MessageBox;
#endif
    
private:
    void* _handle; // Opaque, platform specific window handle
#if USE_EDITOR && PLATFORM_WINDOWS
    Windows::ULONG _refCount;
#endif
#if PLATFORM_LINUX
    bool _resizeDisabled, _focusOnMapped = false, _dragOver = false;
#endif
    SDL_Window* _window;
    uint32 _windowId;
    Rectangle _clipCursorRect;
    Rectangle _cachedClientRectangle;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SDLWindow"/> class.
    /// </summary>
    /// <param name="settings">The initial window settings.</param>
    SDLWindow(const CreateWindowSettings& settings);

    /// <summary>
    /// Finalizes an instance of the <see cref="SDLWindow"/> class.
    /// </summary>
    ~SDLWindow();

private:

    static SDLWindow* GetWindowFromEvent(const SDL_Event& event);
    static SDLWindow* GetWindowWithId(uint32 windowId);
    void HandleEvent(SDL_Event& event);
    void CheckForWindowResize();
    void UpdateCursor() const;

#if PLATFORM_LINUX
    DragDropEffect DoDragDropWayland(const StringView& data);
    DragDropEffect DoDragDropX11(const StringView& data);
#endif

public:

#if PLATFORM_LINUX
    void* GetWaylandSurfacePtr() const;
    void* GetWaylandDisplay() const;
    uintptr GetX11WindowHandle() const;
    void* GetX11Display() const;
#endif

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
    String GetTitle() const override;
    void SetTitle(const StringView& title) override;
    DragDropEffect DoDragDrop(const StringView& data) override;
    void StartTrackingMouse(bool useMouseScreenOffset) override;
    void EndTrackingMouse() override;
    void StartClippingCursor(const Rectangle& bounds) override;
    void EndClippingCursor() override;
    void SetCursor(CursorType type) override;

#if USE_EDITOR && PLATFORM_WINDOWS
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
