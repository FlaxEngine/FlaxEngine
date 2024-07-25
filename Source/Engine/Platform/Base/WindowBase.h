// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Platform/CreateWindowSettings.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Input/KeyboardKeys.h"
#include "Engine/Input/Enums.h"
#include "Enums.h"

class Input;
class Engine;
class RenderTask;
class SceneRenderTask;
class GPUSwapChain;
class TextureData;
class IGuiData;

API_INJECT_CODE(cpp, "#include \"Engine/Platform/Window.h\"");

/// <summary>
/// Native platform window object.
/// </summary>
API_CLASS(NoSpawn, NoConstructor, Sealed, Name="Window")
class FLAXENGINE_API WindowBase : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(WindowBase);
    friend GPUSwapChain;
protected:
    bool _visible, _minimized, _maximized, _isClosing, _showAfterFirstPaint, _focused;
    GPUSwapChain* _swapChain;
    CreateWindowSettings _settings;
    String _title;
    CursorType _cursor;
    Float2 _clientSize;
    int _dpi;
    float _dpiScale;

    Float2 _trackingMouseOffset;
    bool _isUsingMouseOffset = false;
    Rectangle _mouseOffsetScreenSize;
    bool _isTrackingMouse = false;
    bool _isHorizontalFlippingMouse = false;
    bool _isVerticalFlippingMouse = false;
    bool _isClippingCursor = false;

    explicit WindowBase(const CreateWindowSettings& settings);
    virtual ~WindowBase();

public:
    /// <summary>
    /// The rendering task for that window.
    /// </summary>
    RenderTask* RenderTask;

    /// <summary>
    /// Event fired when window gets shown.
    /// </summary>
    Action Shown;

    /// <summary>
    /// Event fired when window gets hidden.
    /// </summary>
    Action Hidden;

    /// <summary>
    /// Event fired when window gets closed.
    /// </summary>
    Action Closed;

    /// <summary>
    /// Event fired when window gets resized.
    /// </summary>
    Delegate<Float2> Resized;

    /// <summary>
    /// Event fired when window gets focused.
    /// </summary>
    Action GotFocus;

    /// <summary>
    /// Event fired when window lost focus.
    /// </summary>
    Action LostFocus;

    /// <summary>
    /// Event fired when window updates UI.
    /// </summary>
    Delegate<float> Update;

    /// <summary>
    /// Event fired when window draws UI.
    /// </summary>
    Action Draw;

public:
    // Returns true if that window is the main Engine window (works in both editor and game mode)
    bool IsMain() const;

    // Gets rendering output swap chain
    FORCE_INLINE GPUSwapChain* GetSwapChain() const
    {
        return _swapChain;
    }

    // Gets create window settings constant reference
    FORCE_INLINE const CreateWindowSettings& GetSettings() const
    {
        return _settings;
    }

    /// <summary>
    /// Gets a value that indicates whether a window is in a fullscreen mode.
    /// </summary>
    API_PROPERTY() bool IsFullscreen() const;

    /// <summary>
    /// Sets a value that indicates whether a window is in a fullscreen mode.
    /// </summary>
    /// <param name="isFullscreen">If set to <c>true</c> window will enter fullscreen mode, otherwise windowed mode.</param>
    API_PROPERTY() virtual void SetIsFullscreen(bool isFullscreen);

    /// <summary>
    /// Gets a value that indicates whether a window is not in a fullscreen mode.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsWindowed() const
    {
        return !IsFullscreen();
    }

    /// <summary>
    /// Gets a value that indicates whether a window is visible (hidden or shown).
    /// </summary>
    API_PROPERTY() bool IsVisible() const;

    /// <summary>
    /// Sets a value that indicates whether a window is visible (hidden or shown).
    /// </summary>
    /// <param name="isVisible">True if show window, otherwise false if hide it.</param>
    API_PROPERTY() void SetIsVisible(bool isVisible);

    /// <summary>
    /// Gets a value that indicates whether a window is minimized.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsMinimized() const
    {
        return _minimized;
    }

    /// <summary>
    /// Gets a value that indicates whether a window is maximized.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsMaximized() const
    {
        return _maximized;
    }

    /// <summary>
    /// Gets the native window handle.
    /// </summary>
    /// <returns>The native window object handle.</returns>
    API_PROPERTY() virtual void* GetNativePtr() const = 0;

public:
    /// <summary>
    /// Performs the UI update.
    /// </summary>
    /// <param name="dt">The delta time (in seconds).</param>
    virtual void OnUpdate(float dt);

    /// <summary>
    /// Performs the window UI rendering using Render2D.
    /// </summary>
    virtual void OnDraw();

    /// <summary>
    /// Initializes the swap chain and the rendering task.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    virtual bool InitSwapChain();

    /// <summary>
    /// Shows the window.
    /// </summary>
    API_FUNCTION() virtual void Show();

    /// <summary>
    /// Hides the window.
    /// </summary>
    API_FUNCTION() virtual void Hide();

    /// <summary>
    /// Minimizes the window.
    /// </summary>
    API_FUNCTION() virtual void Minimize()
    {
    }

    /// <summary>
    /// Maximizes the window.
    /// </summary>
    API_FUNCTION() virtual void Maximize()
    {
    }

    /// <summary>
    /// Sets the window to be borderless or not and to be fullscreen.
    /// </summary>
    /// <param name="isBorderless">Whether or not to have borders on window.</param>
    /// <param name="maximized">Whether or not to make the borderless window fullscreen (maximize to cover whole screen).</param>
    API_FUNCTION() virtual void SetBorderless(bool isBorderless, bool maximized = false)
    {
    }

    /// <summary>
    /// Restores the window state before minimizing or maximizing.
    /// </summary>
    API_FUNCTION() virtual void Restore()
    {
    }

    /// <summary>
    /// Closes the window.
    /// </summary>
    /// <param name="reason">The closing reason.</param>
    API_FUNCTION() virtual void Close(ClosingReason reason = ClosingReason::CloseEvent);

    /// <summary>
    /// Checks if window is closed.
    /// </summary>
    API_PROPERTY() virtual bool IsClosed() const
    {
        return _isClosing;
    }

    /// <summary>
    /// Checks if window is foreground (the window with which the user is currently working).
    /// </summary>
    API_PROPERTY() virtual bool IsForegroundWindow() const;

public:
    /// <summary>
    /// Gets the client bounds of the window (client area not including border).
    /// </summary>
    API_PROPERTY() FORCE_INLINE Rectangle GetClientBounds() const
    {
        return Rectangle(GetClientPosition(), GetClientSize());
    }

    /// <summary>
    /// Sets the client bounds of the window (client area not including border).
    /// </summary>
    /// <param name="clientArea">The client area.</param>
    API_PROPERTY() virtual void SetClientBounds(const Rectangle& clientArea)
    {
    }

    /// <summary>
    /// Gets the window position (in screen coordinates).
    /// </summary>
    API_PROPERTY() virtual Float2 GetPosition() const
    {
        return Float2::Zero;
    }

    /// <summary>
    /// Sets the window position (in screen coordinates).
    /// </summary>
    /// <param name="position">The position.</param>
    API_PROPERTY() virtual void SetPosition(const Float2& position)
    {
    }

    /// <summary>
    /// Gets the client position of the window (client area not including border).
    /// </summary>
    API_PROPERTY() FORCE_INLINE Float2 GetClientPosition() const
    {
        return ClientToScreen(Float2::Zero);
    }

    /// <summary>
    /// Sets the client position of the window (client area not including border)
    /// </summary>
    /// <param name="position">The client area position.</param>
    API_PROPERTY() virtual void SetClientPosition(const Float2& position)
    {
        SetClientBounds(Rectangle(position, GetClientSize()));
    }

    /// <summary>
    /// Gets the window size (including border).
    /// </summary>
    API_PROPERTY() virtual Float2 GetSize() const
    {
        return _clientSize;
    }

    /// <summary>
    /// Gets the size of the client area of the window (not including border).
    /// </summary>
    API_PROPERTY() virtual Float2 GetClientSize() const
    {
        return _clientSize;
    }

    /// <summary>
    /// Sets the size of the client area of the window (not including border).
    /// </summary>
    /// <param name="size">The window client area size.</param>
    API_PROPERTY() void SetClientSize(const Float2& size)
    {
        SetClientBounds(Rectangle(GetClientPosition(), size));
    }

    /// <summary>
    /// Converts screen space location into window space coordinates.
    /// </summary>
    /// <param name="screenPos">The screen position.</param>
    /// <returns>The client space position.</returns>
    API_FUNCTION() virtual Float2 ScreenToClient(const Float2& screenPos) const
    {
        return screenPos;
    }

    /// <summary>
    /// Converts window space location into screen space coordinates.
    /// </summary>
    /// <param name="clientPos">The client position.</param>
    /// <returns>The screen space position.</returns>
    API_FUNCTION() virtual Float2 ClientToScreen(const Float2& clientPos) const
    {
        return clientPos;
    }

    /// <summary>
    /// Gets the window DPI setting.
    /// </summary>
    API_PROPERTY() int GetDpi() const
    {
        return _dpi;
    }

    /// <summary>
    /// Gets the window DPI scale factor (1 is default). Includes custom DPI scale
    /// </summary>
    API_PROPERTY() float GetDpiScale() const
    {
        return Platform::CustomDpiScale * _dpiScale;
    }

public:
    /// <summary>
    /// Gets the window title.
    /// </summary>
    /// <returns>The window title.</returns>
    API_PROPERTY() virtual String GetTitle() const
    {
        return _title;
    }

    /// <summary>
    /// Sets the window title.
    /// </summary>
    /// <param name="title">The title.</param>
    API_PROPERTY() virtual void SetTitle(const StringView& title)
    {
        _title = title;
    }

    /// <summary>
    /// Gets window opacity value (valid only for windows created with SupportsTransparency flag). Opacity values are normalized to range [0;1].
    /// </summary>
    API_PROPERTY() virtual float GetOpacity() const
    {
        return 1.0f;
    }

    /// <summary>
    /// Sets window opacity value (valid only for windows created with SupportsTransparency flag). Opacity values are normalized to range [0;1].
    /// </summary>
    /// <param name="opacity">The opacity.</param>
    API_PROPERTY() virtual void SetOpacity(float opacity)
    {
    }

    /// <summary>
    /// Determines whether this window is focused.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsFocused() const
    {
        return _focused;
    }

    /// <summary>
    /// Focuses this window.
    /// </summary>
    API_FUNCTION() virtual void Focus()
    {
    }

    /// <summary>
    /// Brings window to the front of the Z order.
    /// </summary>
    /// <param name="force">True if move to the front by force, otherwise false.</param>
    API_FUNCTION() virtual void BringToFront(bool force = false)
    {
    }

    /// <summary>
    /// Flashes the window to bring use attention.
    /// </summary>
    API_FUNCTION() virtual void FlashWindow()
    {
    }

public:
    /// <summary>
    /// Starts drag and drop operation
    /// </summary>
    /// <param name="data">The data.</param>
    /// <returns>The result.</returns>
    API_FUNCTION() virtual DragDropEffect DoDragDrop(const StringView& data)
    {
        return DragDropEffect::None;
    }

    /// <summary>
    /// Starts the mouse tracking.
    /// </summary>
    /// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
    API_FUNCTION() virtual void StartTrackingMouse(bool useMouseScreenOffset)
    {
    }

    /// <summary>
    /// Gets the mouse tracking offset.
    /// </summary>
    API_PROPERTY() Float2 GetTrackingMouseOffset() const
    {
        return _trackingMouseOffset;
    }

    /// <summary>
    /// Gets the value indicating whenever mouse input is tracked by this window.
    /// </summary>
    API_PROPERTY() bool IsMouseTracking() const
    {
        return _isTrackingMouse;
    }

    /// <summary>
    /// Gets the value indicating if the mouse flipped to the other screen edge horizontally
    /// </summary>
    API_PROPERTY() bool IsMouseFlippingHorizontally() const
    {
        return _isHorizontalFlippingMouse;
    }

    /// <summary>
    /// Gets the value indicating if the mouse flipped to the other screen edge vertically
    /// </summary>
    API_PROPERTY() bool IsMouseFlippingVertically() const
    {
        return _isVerticalFlippingMouse;
    }

    /// <summary>
    /// Ends the mouse tracking.
    /// </summary>
    API_FUNCTION() virtual void EndTrackingMouse()
    {
    }

    /// <summary>
    /// Starts the cursor clipping.
    /// </summary>
    /// <param name="bounds">The screen-space bounds that the cursor will be confined to.</param>
    API_FUNCTION() virtual void StartClippingCursor(const Rectangle& bounds)
    {
    }

    /// <summary>
    /// Gets the value indicating whenever the cursor is being clipped.
    /// </summary>
    API_PROPERTY() bool IsCursorClipping() const
    {
        return _isClippingCursor;
    }

    /// <summary>
    /// Ends the cursor clipping.
    /// </summary>
    API_FUNCTION() virtual void EndClippingCursor()
    {
    }

    /// <summary>
    /// Gets the mouse cursor.
    /// </summary>
    API_PROPERTY() FORCE_INLINE CursorType GetCursor() const
    {
        return _cursor;
    }

    /// <summary>
    /// Sets the mouse cursor.
    /// </summary>
    /// <param name="type">The cursor type.</param>
    API_PROPERTY() virtual void SetCursor(CursorType type)
    {
        _cursor = type;
    }

    /// <summary>
    /// Sets the window icon.
    /// </summary>
    /// <param name="icon">The icon.</param>
    virtual void SetIcon(TextureData& icon)
    {
    }

    /// <summary>
    /// Gets the value indicating whenever rendering to this window enabled.
    /// </summary>
    API_PROPERTY() bool GetRenderingEnabled() const;

    /// <summary>
    /// Sets the value indicating whenever rendering to this window enabled.
    /// </summary>
    API_PROPERTY() void SetRenderingEnabled(bool value);

public:
    typedef Delegate<Char> CharDelegate;
    typedef Delegate<KeyboardKeys> KeyboardDelegate;
    typedef Delegate<const Float2&> MouseDelegate;
    typedef Delegate<const Float2&, MouseButton> MouseButtonDelegate;
    typedef Delegate<const Float2&, float> MouseWheelDelegate;
    typedef Delegate<const Float2&, int32> TouchDelegate;
    typedef Delegate<IGuiData*, const Float2&, DragDropEffect&> DragDelegate;
    typedef Delegate<const Float2&, WindowHitCodes&, bool&> HitTestDelegate;
    typedef Delegate<WindowHitCodes, bool&> ButtonHitDelegate;
    typedef Delegate<ClosingReason, bool&> ClosingDelegate;

    /// <summary>
    /// Event fired on character input.
    /// </summary>
    CharDelegate CharInput;
    void OnCharInput(Char c);

    /// <summary>
    /// Event fired on key pressed.
    /// </summary>
    KeyboardDelegate KeyDown;
    void OnKeyDown(KeyboardKeys key);

    /// <summary>
    /// Event fired on key released.
    /// </summary>
    KeyboardDelegate KeyUp;
    void OnKeyUp(KeyboardKeys key);

    /// <summary>
    /// Event fired when mouse button goes down.
    /// </summary>
    MouseButtonDelegate MouseDown;
    void OnMouseDown(const Float2& mousePosition, MouseButton button);

    /// <summary>
    /// Event fired when mouse button goes up.
    /// </summary>
    MouseButtonDelegate MouseUp;
    void OnMouseUp(const Float2& mousePosition, MouseButton button);

    /// <summary>
    /// Event fired when mouse button double clicks.
    /// </summary>
    MouseButtonDelegate MouseDoubleClick;
    void OnMouseDoubleClick(const Float2& mousePosition, MouseButton button);

    /// <summary>
    /// Event fired when mouse wheel is scrolling (wheel delta is normalized).
    /// </summary>
    MouseWheelDelegate MouseWheel;
    void OnMouseWheel(const Float2& mousePosition, float delta);

    /// <summary>
    /// Event fired when mouse moves.
    /// </summary>
    MouseDelegate MouseMove;
    void OnMouseMove(const Float2& mousePosition);

    /// <summary>
    /// Event fired when mouse leaves window.
    /// </summary>
    Action MouseLeave;
    void OnMouseLeave();

    /// <summary>
    /// Event fired when touch action begins.
    /// </summary>
    TouchDelegate TouchDown;
    void OnTouchDown(const Float2& pointerPosition, int32 pointerIndex);

    /// <summary>
    /// Event fired when touch action moves.
    /// </summary>
    TouchDelegate TouchMove;
    void OnTouchMove(const Float2& pointerPosition, int32 pointerIndex);

    /// <summary>
    /// Event fired when touch action ends.
    /// </summary>
    TouchDelegate TouchUp;
    void OnTouchUp(const Float2& pointerPosition, int32 pointerIndex);

    /// <summary>
    /// Event fired when drag&drop enters window.
    /// </summary>
    DragDelegate DragEnter;
    void OnDragEnter(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);

    /// <summary>
    /// Event fired when drag&drop moves over window.
    /// </summary>
    DragDelegate DragOver;
    void OnDragOver(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);

    /// <summary>
    /// Event fired when drag&drop ends over window with drop.
    /// </summary>
    DragDelegate DragDrop;
    void OnDragDrop(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);

    /// <summary>
    /// Event fired when drag&drop leaves window.
    /// </summary>
    Action DragLeave;
    void OnDragLeave();

    /// <summary>
    /// Event fired when system tests if the specified location is part of the window.
    /// </summary>
    HitTestDelegate HitTest;
    void OnHitTest(const Float2& mousePosition, WindowHitCodes& result, bool& handled);

    /// <summary>
    /// Event fired when system tests if the left button hit the window for the given hit code.
    /// </summary>
    ButtonHitDelegate LeftButtonHit;
    void OnLeftButtonHit(WindowHitCodes hit, bool& result);

    /// <summary>
    /// Event fired when window is closing. Can be used to cancel the operation.
    /// </summary>
    ClosingDelegate Closing;
    void OnClosing(ClosingReason reason, bool& cancel);

public:
    /// <summary>
    /// Gets the text entered during the current frame (Unicode).
    /// </summary>
    /// <returns>The input text (Unicode).</returns>
    API_PROPERTY() StringView GetInputText() const;

    /// <summary>
    /// Gets the key state (true if key is being pressed during this frame).
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True while the user holds down the key identified by id</returns>
    API_FUNCTION() bool GetKey(KeyboardKeys key) const;

    /// <summary>
    /// Gets the key 'down' state (true if key was pressed in this frame).
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True during the frame the user starts pressing down the key</returns>
    API_FUNCTION() bool GetKeyDown(KeyboardKeys key) const;

    /// <summary>
    /// Gets the key 'up' state (true if key was released in this frame).
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True during the frame the user releases the key</returns>
    API_FUNCTION() bool GetKeyUp(KeyboardKeys key) const;

public:
    /// <summary>
    /// Gets the mouse position in window coordinates.
    /// </summary>
    API_PROPERTY() Float2 GetMousePosition() const;

    /// <summary>
    /// Sets the mouse position in window coordinates.
    /// </summary>
    /// <param name="position">Mouse position to set on</param>
    API_PROPERTY() void SetMousePosition(const Float2& position) const;

    /// <summary>
    /// Gets the mouse position change during the last frame.
    /// </summary>
    /// <returns>Mouse cursor position delta</returns>
    API_PROPERTY() Float2 GetMousePositionDelta() const;

    /// <summary>
    /// Gets the mouse wheel change during the last frame.
    /// </summary>
    API_PROPERTY() float GetMouseScrollDelta() const;

    /// <summary>
    /// Gets the mouse button state.
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True while the user holds down the button</returns>
    API_FUNCTION() bool GetMouseButton(MouseButton button) const;

    /// <summary>
    /// Gets the mouse button down state.
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True during the frame the user starts pressing down the button</returns>
    API_FUNCTION() bool GetMouseButtonDown(MouseButton button) const;

    /// <summary>
    /// Gets the mouse button up state.
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True during the frame the user releases the button</returns>
    API_FUNCTION() bool GetMouseButtonUp(MouseButton button) const;

public:
    void OnShow();
    void OnResize(int32 width, int32 height);
    void OnClosed();
    void OnGotFocus();
    void OnLostFocus();

private:
    void OnMainRenderTaskDelete(class ScriptingObject* obj)
    {
        RenderTask = nullptr;
    }

public:
    // [ScriptingObject]
    String ToString() const override;
    void OnDeleteObject() override;
};
