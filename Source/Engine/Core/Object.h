// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Memory/Memory.h"

/// <summary>
/// Engine objects flags used in various aspects but packed into a single flags container.
/// </summary>
enum class ObjectFlags : uint32
{
    None = 0,
    WasMarkedToDelete = 1 << 0,
    UseGameTimeForDelete = 1 << 1,
    IsRegistered = 1 << 2,
    IsManagedType = 1 << 3,
    IsDuringPlay = 1 << 4,
    IsCustomScriptingType = 1 << 5,
};

DECLARE_ENUM_OPERATORS(ObjectFlags);

/// <summary>
/// Interface for Engine objects.
/// </summary>
class FLAXENGINE_API Object
{
public:

    /// <summary>
    /// The object flags.
    /// </summary>
    ObjectFlags Flags = ObjectFlags::None;

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="Object"/> class.
    /// </summary>
    virtual ~Object()
    {
    }

    /// <summary>
    /// Gets the string representation of this object.
    /// </summary>
    /// <returns>The string.</returns>
    virtual String ToString() const = 0;
};

/// <summary>
/// Interface for removable Engine objects.
/// </summary>
class FLAXENGINE_API RemovableObject : public Object
{
public:

    /// <summary>
    /// Virtual destructor but protected. Removable objects should be deleted using `DeleteObject` which supports deferred delete.
    /// Note: it's unsafe to delete object using destructor it it has been marked for deferred delete.
    /// </summary>
    virtual ~RemovableObject();

    /// <summary>
    /// Deletes the object without queueing it to the ObjectsRemovalService.
    /// </summary>
    void DeleteObjectNow();

    /// <summary>
    /// Deletes the object.
    /// </summary>
    /// <param name="timeToLive">The time to live (in seconds). Use zero to kill it now.</param>
    /// <param name="useGameTime">True if unscaled game time for the object life timeout, otherwise false to use absolute time.</param>
    void DeleteObject(float timeToLive = 0.0f, bool useGameTime = false);

    /// <summary>
    /// Deletes the object. Called by the ObjectsRemovalService. Can be overriden to provide custom logic per object (cleanup, etc.).
    /// </summary>
    virtual void OnDeleteObject()
    {
        Delete(this);
    }
};
