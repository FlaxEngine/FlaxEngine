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
    /// The keyboard state.
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

public:

    /// <summary>
    /// Called when keyboard enters input character.
    /// </summary>
    /// <param name="c">The Unicode character entered by the user.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnCharInput(Char c, Window* target = nullptr);

    /// <summary>
    /// Called when key goes up.
    /// </summary>
    /// <param name="key">The keyboard key.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnKeyUp(KeyboardKeys key, Window* target = nullptr);

    /// <summary>
    /// Called when key goes down.
    /// </summary>
    /// <param name="key">The keyboard key.</param>
    /// <param name="target">The target window to receive this event, otherwise input system will pick the window automatically.</param>
    void OnKeyDown(KeyboardKeys key, Window* target = nullptr);

public:

    // [InputDevice]
    void ResetState() override;;
    bool Update(EventQueue& queue) final override;
};
