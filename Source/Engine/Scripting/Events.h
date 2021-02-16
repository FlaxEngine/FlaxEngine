// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ScriptingType.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// The helper utility for binding and invoking scripting events (eg. used by Visual Scripting).
/// </summary>
class FLAXENGINE_API ScriptingEvents
{
public:

    /// <summary>
    /// Global table for registered even binder methods (key is pair of type and event name, value is method that takes instance with event, object to bind and flag to bind or unbind).
    /// </summary>
    /// <remarks>
    /// Key: pair of event type name (full), event name.
    /// Value: event binder function with parameters: event caller instance (null for static events), object to bind, true to bind/false to unbind.
    /// </remarks>
    static Dictionary<Pair<ScriptingTypeHandle, StringView>, void(*)(ScriptingObject*, void*, bool)> EventsTable;

    /// <summary>
    /// The action called when any scripting event occurs. Can be used to invoke scripting code that binded to this particular event.
    /// </summary>
    /// <remarks>
    /// Delegate parameters: event caller instance (null for static events), event invocation parameters list, event type name (full), event name.
    /// </remarks>
    static Delegate<ScriptingObject*, Span<Variant>, ScriptingTypeHandle, StringView> Event;
};
