// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "InputDevice.h"

/// <summary>
/// Represents a single hardware keyboard device. Used by the Input to report raw keyboard input events.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Keyboard : public InputDevice
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Keyboard);
public:

    /// <summary>
    /// The mouse state.
    /// </summary>
    struct State
    {
        /// <summary>
        /// The input text length (characters count).
        /// </summary>
        uint16 InputTextLength;

        /// <summary>
        /// The input text.
        /// </summary>
        Char InputText[32];

        /// <summary>
        /// The keys.
        /// </summary>
        bool Keys[(int32)KeyboardKeys::MAX];

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

    explicit Keyboard()
        : InputDevice(SpawnParams(Guid::New(), TypeInitializer), TEXT("Keyboard"))
    {
        _state.Clear();
        _prevState.Clear();
    }

public:

    /// <summary>
    /// Gets the text entered during the current frame.
    /// </summary>
    /// <returns>The input text (Unicode).</returns>
    API_PROPERTY() StringView GetInputText() const
    {
        return StringView(_state.InputText, _state.InputTextLength);
    }

    /// <summary>
    /// Gets keyboard key state.
    /// </summary>
    /// <param name="key">Key ID to check.</param>
    /// <returns>True if user holds down the key identified by id, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool GetKey(KeyboardKeys key) const
    {
        return _state.Keys[static_cast<int32>(key)];
    }

    /// <summary>
    /// Gets keyboard key down state.
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True if user starts pressing down the key, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool GetKeyDown(KeyboardKeys key) const
    {
        return _state.Keys[static_cast<int32>(key)] && !_prevState.Keys[static_cast<int32>(key)];
    }

    /// <summary>
    /// Gets keyboard key up state.
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True if user releases the key, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool GetKeyUp(KeyboardKeys key) const
    {
        return !_state.Keys[static_cast<int32>(key)] && _prevState.Keys[static_cast<int32>(key)];
    }

    /// <summary>
    /// Called when keyboard enters input character.
    /// </summary>
    /// <param name="c">The Unicode character entered by the user.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnCharInput(const Char c, Window* target = nullptr)
    {
        // Skip control characters
        if (c < 32)
            return;

        Event& e = _queue.AddOne();
        e.Type = EventType::Char;
        e.Target = target;
        e.CharData.Char = c;
    }

    /// <summary>
    /// Called when key goes up.
    /// </summary>
    /// <param name="key">The keyboard key.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnKeyUp(const KeyboardKeys key, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::KeyUp;
        e.Target = target;
        e.KeyData.Key = key;
    }

    /// <summary>
    /// Called when key goes down.
    /// </summary>
    /// <param name="key">The keyboard key.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnKeyDown(const KeyboardKeys key, Window* target = nullptr)
    {
        Event& e = _queue.AddOne();
        e.Type = EventType::KeyDown;
        e.Target = target;
        e.KeyData.Key = key;
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
        for (int32 i = 0; i < _queue.Count(); i++)
        {
            const Event& e = _queue[i];
            switch (e.Type)
            {
            case EventType::Char:
            {
                if (_state.InputTextLength < ARRAY_COUNT(_state.InputText) - 1)
                    _state.InputText[_state.InputTextLength++] = e.CharData.Char;
                break;
            }
            case EventType::KeyDown:
            {
                _state.Keys[static_cast<int32>(e.KeyData.Key)] = true;
                break;
            }
            case EventType::KeyUp:
            {
                _state.Keys[static_cast<int32>(e.KeyData.Key)] = false;
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
