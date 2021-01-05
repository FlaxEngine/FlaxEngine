// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        Vector2 MousePosition;

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
    /// <returns>The mouse position</returns>
    API_PROPERTY() FORCE_INLINE Vector2 GetPosition() const
    {
        return _state.MousePosition;
    }

    /// <summary>
    /// Gets the delta position of the mouse in the screen-space coordinates.
    /// </summary>
    /// <returns>The mouse position delta</returns>
    API_PROPERTY() FORCE_INLINE Vector2 GetPositionDelta() const
    {
        return _state.MousePosition - _prevState.MousePosition;
    }

    /// <summary>
    /// Gets the mouse wheel change during the last frame.
    /// </summary>
    /// <returns>Mouse wheel value delta</returns>
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
    virtual void SetMousePosition(const Vector2& newPosition) = 0;

    /// <summary>
    /// Called when mouse cursor gets moved by the application. Invalidates the previous cached mouse position to prevent mouse jitter when locking the cursor programmatically.
    /// </summary>
    /// <param name="newPosition">The new mouse position.</param>
    void OnMouseMoved(const Vector2& newPosition)
    {
        _prevState.MousePosition = newPosition;
        _state.MousePosition = newPosition;
    }

    /// <summary>
    /// Called when mouse button goes down.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="button">The button.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseDown(const Vector2& position, const MouseButton button, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::MouseDown;
        e.Target = target;
        e.MouseData.Button = button;
        e.MouseData.Position = position;
    }

    /// <summary>
    /// Called when mouse button goes up.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="button">The button.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseUp(const Vector2& position, const MouseButton button, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::MouseUp;
        e.Target = target;
        e.MouseData.Button = button;
        e.MouseData.Position = position;
    }

    /// <summary>
    /// Called when mouse double clicks.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="button">The button.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseDoubleClick(const Vector2& position, const MouseButton button, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::MouseDoubleClick;
        e.Target = target;
        e.MouseData.Button = button;
        e.MouseData.Position = position;
    }

    /// <summary>
    /// Called when mouse moves.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseMove(const Vector2& position, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::MouseMove;
        e.Target = target;
        e.MouseData.Position = position;
    }

    /// <summary>
    /// Called when mouse leaves the input source area.
    /// </summary>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseLeave(Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::MouseLeave;
        e.Target = target;
    }

    /// <summary>
    /// Called when mouse wheel moves.
    /// </summary>
    /// <param name="position">The mouse position.</param>
    /// <param name="delta">The normalized delta (range [-1;1]).</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnMouseWheel(const Vector2& position, const float delta, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::MouseWheel;
        e.Target = target;
        e.MouseWheelData.WheelDelta = delta;
        e.MouseWheelData.Position = position;
    }

public:

    // [InputDevice]
    void ResetState() override
    {
        InputDevice::ResetState();

        _prevState.Clear();
        _state.Clear();
    }

    bool Update(EventQueue& queue) final override
    {
        // Move the current state to the previous
        Platform::MemoryCopy(&_prevState, &_state, sizeof(State));

        // Gather new events
        if (UpdateState())
            return true;

        // Handle events
        _state.MouseWheelDelta = 0;
        for (int32 i = 0; i < _queue.Count(); i++)
        {
            const Event& e = _queue[i];
            switch (e.Type)
            {
            case EventType::MouseDown:
            {
                _state.MouseButtons[static_cast<int32>(e.MouseData.Button)] = true;
                break;
            }
            case EventType::MouseUp:
            {
                _state.MouseButtons[static_cast<int32>(e.MouseData.Button)] = false;
                break;
            }
            case EventType::MouseDoubleClick:
            {
                break;
            }
            case EventType::MouseWheel:
            {
                _state.MouseWheelDelta += e.MouseWheelData.WheelDelta;
                break;
            }
            case EventType::MouseMove:
            {
                _state.MousePosition = e.MouseData.Position;
                break;
            }
            case EventType::MouseLeave:
            {
                break;
            }
            }
        }

        // Send events further
        queue.Add(_queue);
        _queue.Clear();
        return false;
    }
};
