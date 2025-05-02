// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Enums.h"
#include "KeyboardKeys.h"

/// <summary>
/// Base class for all input device objects.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API InputDevice : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(InputDevice);
public:
    enum class EventType
    {
        Char,
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseDoubleClick,
        MouseWheel,
        MouseMove,
        MouseLeave,
        TouchDown,
        TouchMove,
        TouchUp,
    };

    struct Event
    {
        EventType Type;
        Window* Target;

        union
        {
            struct
            {
                Char Char;
            } CharData;

            struct
            {
                KeyboardKeys Key;
            } KeyData;

            struct
            {
                MouseButton Button;
                Float2 Position;
            } MouseData;

            struct
            {
                float WheelDelta;
                Float2 Position;
            } MouseWheelData;

            struct
            {
                Float2 Position;
                int32 PointerId;
            } TouchData;
        };

        Event()
        {
        }

        Event(const Event& e)
        {
            Platform::MemoryCopy(this, &e, sizeof(Event));
        }
    };

    typedef Array<Event, InlinedAllocation<32>> EventQueue;

protected:
    String _name;
    EventQueue _queue;

    explicit InputDevice(const SpawnParams& params, const StringView& name)
        : ScriptingObject(params)
        , _name(name)
    {
    }

public:
    /// <summary>
    /// Gets the name.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const String& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Resets the input device state. Called when application looses focus.
    /// </summary>
    virtual void ResetState()
    {
        _queue.Clear();
    }

    /// <summary>
    /// Captures the input since the last call and triggers the input events.
    /// </summary>
    /// <param name="queue">The input events queue.</param>
    /// <returns>True if device has been disconnected, otherwise false.</returns>
    virtual bool Update(EventQueue& queue)
    {
        if (UpdateState())
            return true;
        queue.Add(_queue);
        _queue.Clear();
        return false;
    }

    /// <summary>
    /// Updates only the current state of the device.
    /// </summary>
    /// <returns>True if device has been disconnected, otherwise false.</returns>
    virtual bool UpdateState()
    {
        return false;
    }
};
