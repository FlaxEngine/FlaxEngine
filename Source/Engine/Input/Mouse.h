// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "InputDevice.h"

/// <summary>
/// Represents a single hardware mouse device. Used by the Input to report raw mouse input events.
/// </summary>
/// <remarks>
/// The mouse device position is in screen-space (not game client window space).
/// </remarks>
API_CLASS(NoSpawn) class FLAXENGINE_API Mouse : public InputDevice
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Mouse);
public:
    /// <summary>
    /// The mouse state.
    /// </summary>
    struct State
    {
        /// <summary>
        /// The mouse position.
        /// </summary>
        Float2 MousePosition;

        /// <summary>
        /// The mouse wheel delta.
        /// </summary>
        float MouseWheelDelta;

        /// <summary>
        /// The mouse buttons state.
        /// </summary>
        bool MouseButtons[(int32)MouseButton::MAX];

        /// <summary>
        /// Clears the state.
        /// </summary>
        void Clear()
        {
            Platform::MemoryClear(this, sizeof(State));
        }
    };

protected:
    State _state;
    State _prevState;

    explicit Mouse()
        : InputDevice(SpawnParams(Guid::New(), TypeInitializer), TEXT("Mouse"))
    {
        _state.Clear();
        _prevState.Clear();
    }

public:
    /// <summary>
    /// Gets the position of the mouse in the screen-space coordinates.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Float2 GetPosition() const
    {
        return _state.MousePosition;
    }

    /// <summary>
    /// Checks if any mouse button is currently pressed.
    /// </summary>
    API_PROPERTY() bool IsAnyButtonDown() const;

    /// <summary>
    /// Gets the delta position of the mouse in the screen-space coordinates.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Float2 GetPositionDelta() const
    {
        return _state.MousePosition - _prevState.MousePosition;
    }

    /// <summary>
    /// Gets the mouse wheel change during the last frame.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetScrollDelta() const
    {
        return _state.MouseWheelDelta;
    }

    /// <summary>
    /// Gets the mouse button state (true if being pressed during the current frame).
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True if user holds down the button, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool GetButton(MouseButton button) const
    {
        return _state.MouseButtons[static_cast<int32>(button)];
    }

    /// <summary>
    /// Gets the mouse button down state (true if was pressed during the current frame).
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True if user starts pressing down the button, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool GetButtonDown(MouseButton button) const
    {
        return _state.MouseButtons[static_cast<int32>(button)] && !_prevState.MouseButtons[static_cast<int32>(button)];
    }

    /// <summary>
    /// Gets the mouse button up state (true if was released during the current frame).
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True if user releases the button, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool GetButtonUp(MouseButton button) const
    {
        return !_state.MouseButtons[static_cast<int32>(button)] && _prevState.MouseButtons[static_cast<int32>(button)];
    }

public:
    /// <summary>
    /// Sets the mouse position.
    /// </summary>
    /// <param name="newPosition">The new position.</param>
    virtual void SetMousePosition(const Float2& newPosition) = 0;

    /// <summary>
    /// Called when mouse cursor gets moved by the application. Invalidates the previous cached mouse position to prevent mouse jitter when locking the cursor programmatically.
    /// </summary>
    /// <param name="newPosition">The new mouse position.</param>
    void OnMouseMoved(const Float2& newPosition);

    /// <summary>
    /// Called when mouse button goes down.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="button">The button.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseDown(const Float2& position, const MouseButton button, Window* target = nullptr);

    /// <summary>
    /// Called when mouse button goes up.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="button">The button.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseUp(const Float2& position, const MouseButton button, Window* target = nullptr);

    /// <summary>
    /// Called when mouse double clicks.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="button">The button.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseDoubleClick(const Float2& position, const MouseButton button, Window* target = nullptr);

    /// <summary>
    /// Called when mouse moves.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseMove(const Float2& position, Window* target = nullptr);

    /// <summary>
    /// Called when mouse leaves the input source area.
    /// </summary>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseLeave(Window* target = nullptr);

    /// <summary>
    /// Called when mouse wheel moves.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="delta">The normalized delta (range [-1;1]).</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseWheel(const Float2& position, float delta, Window* target = nullptr);

public:
    // [InputDevice]
    void ResetState() override;
    bool Update(EventQueue& queue) final override;
};
