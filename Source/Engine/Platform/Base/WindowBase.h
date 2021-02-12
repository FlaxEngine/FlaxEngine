// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Platform/CreateWindowSettings.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Input/KeyboardKeys.h"
#include "Engine/Input/Enums.h"

class Input;
class Engine;
class RenderTask;
class SceneRenderTask;
class GPUSwapChain;
class TextureData;
class IGuiData;

/// <summary>
/// Window closing reasons.
/// </summary>
API_ENUM() enum class ClosingReason
{
    /// <summary>
    /// The unknown.
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// The user.
    /// </summary>
    User,

    /// <summary>
    /// The engine exit.
    /// </summary>
    EngineExit,

    /// <summary>
    /// The close event.
    /// </summary>
    CloseEvent,
};

/// <summary>
/// Types of default cursors.
/// </summary>
API_ENUM() enum class CursorType
{
    /// <summary>
    /// The default.
    /// </summary>
    Default = 0,

    /// <summary>
    /// The cross.
    /// </summary>
    Cross,

    /// <summary>
    /// The hand.
    /// </summary>
    Hand,

    /// <summary>
    /// The help icon
    /// </summary>
    Help,

    /// <summary>
    /// The I beam.
    /// </summary>
    IBeam,

    /// <summary>
    /// The blocking image.
    /// </summary>
    No,

    /// <summary>
    /// The wait.
    /// </summary>
    Wait,

    /// <summary>
    /// The size all sides.
    /// </summary>
    SizeAll,

    /// <summary>
    /// The size NE-SW.
    /// </summary>
    SizeNESW,

    /// <summary>
    /// The size NS.
    /// </summary>
    SizeNS,

    /// <summary>
    /// The size NW-SE.
    /// </summary>
    SizeNWSE,

    /// <summary>
    /// The size WE.
    /// </summary>
    SizeWE,

    /// <summary>
    /// The cursor is hidden.
    /// </summary>
    Hidden,

    MAX
};

/// <summary>
/// Data drag and drop effects.
/// </summary>
API_ENUM() enum class DragDropEffect
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The copy.
    /// </summary>
    Copy,

    /// <summary>
    /// The move.
    /// </summary>
    Move,

    /// <summary>
    /// The link.
    /// </summary>
    Link,
};

/// <summary>
/// Window hit test codes. Note: they are 1:1 mapping for Win32 values.
/// </summary>
API_ENUM() enum class WindowHitCodes
{
    /// <summary>
    /// The transparent area.
    /// </summary>
    Transparent = -1,

    /// <summary>
    /// The no hit.
    /// </summary>
    NoWhere = 0,

    /// <summary>
    /// The client area.
    /// </summary>
    Client = 1,

    /// <summary>
    /// The caption area.
    /// </summary>
    Caption = 2,

    /// <summary>
    /// The system menu.
    /// </summary>
    SystemMenu = 3,

    /// <summary>
    /// The grow box
    /// </summary>
    GrowBox = 4,

    /// <summary>
    /// The menu.
    /// </summary>
    Menu = 5,

    /// <summary>
    /// The horizontal scroll.
    /// </summary>
    HScroll = 6,

    /// <summary>
    /// The vertical scroll.
    /// </summary>
    VScroll = 7,

    /// <summary>
    /// The minimize button.
    /// </summary>
    MinButton = 8,

    /// <summary>
    /// The maximize button.
    /// </summary>
    MaxButton = 9,

    /// <summary>
    /// The left side;
    /// </summary>
    Left = 10,

    /// <summary>
    /// The right side.
    /// </summary>
    Right = 11,

    /// <summary>
    /// The top side.
    /// </summary>
    Top = 12,

    /// <summary>
    /// The top left corner.
    /// </summary>
    TopLeft = 13,

    /// <summary>
    /// The top right corner.
    /// </summary>
    TopRight = 14,

    /// <summary>
    /// The bottom side.
    /// </summary>
    Bottom = 15,

    /// <summary>
    /// The bottom left corner.
    /// </summary>
    BottomLeft = 16,

    /// <summary>
    /// The bottom right corner.
    /// </summary>
    BottomRight = 17,

    /// <summary>
    /// The border.
    /// </summary>
    Border = 18,

    /// <summary>
    /// The object.
    /// </summary>
    Object = 19,

    /// <summary>
    /// The close button.
    /// </summary>
    Close = 20,

    /// <summary>
    /// The help button.
    /// </summary>
    Help = 21,
};

API_INJECT_CPP_CODE("#include \"Engine/Platform/Window.h\"");

/// <summary>
/// Native platform window object.
/// </summary>
API_CLASS(NoSpawn, NoConstructor, Sealed, Name="Window")
class FLAXENGINE_API WindowBase : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(WindowBase);
    friend GPUSwapChain;
protected:

    bool _visible, _minimized, _maximized, _isClosing, _showAfterFirstPaint, _focused;
    GPUSwapChain* _swapChain;
    CreateWindowSettings _settings;
    String _title;
    CursorType _cursor;
    Vector2 _clientSize;

    Vector2 _trackingMouseOffset;
    bool _isUsingMouseOffset;
    Rectangle _mouseOffsetScreenSize;
    bool _isTrackingMouse;

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
    API_FUNCTION() virtual void Minimize() = 0;

    /// <summary>
    /// Maximizes the window.
    /// </summary>
    API_FUNCTION() virtual void Maximize() = 0;

    /// <summary>
    /// Restores the window state before minimizing or maximizing.
    /// </summary>
    API_FUNCTION() virtual void Restore() = 0;

    /// <summary>
    /// Closes the window.
    /// </summary>
    /// <param name="reason">The closing reason.</param>
    API_FUNCTION() virtual void Close(ClosingReason reason = ClosingReason::CloseEvent);

    /// <summary>
    /// Checks if window is closed.
    /// </summary>
    /// <returns>True if window is closed, otherwise false.</returns>
    API_PROPERTY() virtual bool IsClosed() const = 0;

    /// <summary>
    /// Checks if window is foreground (the window with which the user is currently working).
    /// </summary>
    /// <returns>True if window is foreground, otherwise false.</returns>
    API_PROPERTY() virtual bool IsForegroundWindow() const;

public:

    /// <summary>
    /// Gets the client bounds of the window (client area not including border).
    /// </summary>
    /// <returns>Client bounds.</returns>
    API_PROPERTY() FORCE_INLINE Rectangle GetClientBounds() const
    {
        return Rectangle(GetClientPosition(), GetClientSize());
    }

    /// <summary>
    /// Sets the client bounds of the window (client area not including border).
    /// </summary>
    /// <param name="clientArea">The client area.</param>
    API_PROPERTY() virtual void SetClientBounds(const Rectangle& clientArea) = 0;

    /// <summary>
    /// Gets the window position (in screen coordinates).
    /// </summary>
    /// <returns>Window position.</returns>
    API_PROPERTY() virtual Vector2 GetPosition() const = 0;

    /// <summary>
    /// Sets the window position (in screen coordinates).
    /// </summary>
    /// <param name="position">The position.</param>
    API_PROPERTY() virtual void SetPosition(const Vector2& position) = 0;

    /// <summary>
    /// Gets the client position of the window (client area not including border).
    /// </summary>
    /// <returns>The client area position.</returns>
    API_PROPERTY() FORCE_INLINE Vector2 GetClientPosition() const
    {
        return ClientToScreen(Vector2::Zero);
    }

    /// <summary>
    /// Sets the client position of the window (client area not including border)
    /// </summary>
    /// <param name="position">The client area position.</param>
    API_PROPERTY() virtual void SetClientPosition(const Vector2& position) = 0;

    /// <summary>
    /// Gets the window size (including border).
    /// </summary>
    /// <returns>The window size</returns>
    API_PROPERTY() virtual Vector2 GetSize() const = 0;

    /// <summary>
    /// Gets the size of the client area of the window (not including border).
    /// </summary>
    /// <returns>The window client area size.</returns>
    API_PROPERTY() virtual Vector2 GetClientSize() const = 0;

    /// <summary>
    /// Sets the size of the client area of the window (not including border).
    /// </summary>
    /// <param name="size">The window client area size.</param>
    API_PROPERTY() void SetClientSize(const Vector2& size)
    {
        SetClientBounds(Rectangle(GetClientPosition(), size));
    }

    /// <summary>
    /// Converts screen space location into window space coordinates.
    /// </summary>
    /// <param name="screenPos">The screen position.</param>
    /// <returns>The client space position.</returns>
    API_FUNCTION() virtual Vector2 ScreenToClient(const Vector2& screenPos) const = 0;

    /// <summary>
    /// Converts window space location into screen space coordinates.
    /// </summary>
    /// <param name="clientPos">The client position.</param>
    /// <returns>The screen space position.</returns>
    API_FUNCTION() virtual Vector2 ClientToScreen(const Vector2& clientPos) const = 0;

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
    /// <returns>Window opacity.</returns>
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
    /// <returns><c>true</c> if this window is focused; otherwise, <c>false</c>.</returns>
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
    /// <returns>The mouse screen offset.</returns>
    API_PROPERTY() Vector2 GetTrackingMouseOffset() const
    {
        return _trackingMouseOffset;
    }

    /// <summary>
    /// Ends the mouse tracking.
    /// </summary>
    API_FUNCTION() virtual void EndTrackingMouse()
    {
    }

    /// <summary>
    /// Gets the mouse cursor.
    /// </summary>
    /// <returns>The cursor type</returns>
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
    typedef Delegate<const Vector2&> MouseDelegate;
    typedef Delegate<const Vector2&, MouseButton> MouseButtonDelegate;
    typedef Delegate<const Vector2&, float> MouseWheelDelegate;
    typedef Delegate<const Vector2&, int32> TouchDelegate;
    typedef Delegate<IGuiData*, const Vector2&, DragDropEffect&> DragDelegate;
    typedef Delegate<const Vector2&, WindowHitCodes&, bool&> HitTestDelegate;
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
    void OnMouseDown(const Vector2& mousePosition, MouseButton button);

    /// <summary>
    /// Event fired when mouse button goes up.
    /// </summary>
    MouseButtonDelegate MouseUp;
    void OnMouseUp(const Vector2& mousePosition, MouseButton button);

    /// <summary>
    /// Event fired when mouse button double clicks.
    /// </summary>
    MouseButtonDelegate MouseDoubleClick;
    void OnMouseDoubleClick(const Vector2& mousePosition, MouseButton button);

    /// <summary>
    /// Event fired when mouse wheel is scrolling (wheel delta is normalized).
    /// </summary>
    MouseWheelDelegate MouseWheel;
    void OnMouseWheel(const Vector2& mousePosition, float delta);

    /// <summary>
    /// Event fired when mouse moves.
    /// </summary>
    MouseDelegate MouseMove;
    void OnMouseMove(const Vector2& mousePosition);

    /// <summary>
    /// Event fired when mouse leaves window.
    /// </summary>
    Action MouseLeave;
    void OnMouseLeave();

    /// <summary>
    /// Event fired when touch action begins.
    /// </summary>
    TouchDelegate TouchDown;
    void OnTouchDown(const Vector2& pointerPosition, int32 pointerIndex);

    /// <summary>
    /// Event fired when touch action moves.
    /// </summary>
    TouchDelegate TouchMove;
    void OnTouchMove(const Vector2& pointerPosition, int32 pointerIndex);

    /// <summary>
    /// Event fired when touch action ends.
    /// </summary>
    TouchDelegate TouchUp;
    void OnTouchUp(const Vector2& pointerPosition, int32 pointerIndex);

    /// <summary>
    /// Event fired when drag&drop enters window.
    /// </summary>
    DragDelegate DragEnter;
    void OnDragEnter(IGuiData* data, const Vector2& mousePosition, DragDropEffect& result);

    /// <summary>
    /// Event fired when drag&drop moves over window.
    /// </summary>
    DragDelegate DragOver;
    void OnDragOver(IGuiData* data, const Vector2& mousePosition, DragDropEffect& result);

    /// <summary>
    /// Event fired when drag&drop ends over window with drop.
    /// </summary>
    DragDelegate DragDrop;
    void OnDragDrop(IGuiData* data, const Vector2& mousePosition, DragDropEffect& result);

    /// <summary>
    /// Event fired when drag&drop leaves window.
    /// </summary>
    Action DragLeave;
    void OnDragLeave();

    /// <summary>
    /// Event fired when system tests if the specified location is part of the window.
    /// </summary>
    HitTestDelegate HitTest;
    void OnHitTest(const Vector2& mousePosition, WindowHitCodes& result, bool& handled);

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
    /// <returns>Mouse cursor coordinates</returns>
    API_PROPERTY() Vector2 GetMousePosition() const;

    /// <summary>
    /// Sets the mouse position in window coordinates.
    /// </summary>
    /// <param name="position">Mouse position to set on</param>
    API_PROPERTY() void SetMousePosition(const Vector2& position) const;

    /// <summary>
    /// Gets the mouse position change during the last frame.
    /// </summary>
    /// <returns>Mouse cursor position delta</returns>
    API_PROPERTY() Vector2 GetMousePositionDelta() const;

    /// <summary>
    /// Gets the mouse wheel change during the last frame.
    /// </summary>
    /// <returns>Mouse wheel value delta</returns>
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

    // [PersistentScriptingObject]
    String ToString() const override;
    void OnDeleteObject() override;
};
