// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

/// <summary>
/// A base interface for all physical actors types/owners that can responds on transformation changed event.
/// </summary>
class FLAXENGINE_API IPhysicsActor
{
public:
    /// <summary>
    /// Finalizes an instance of the <see cref="IPhysicsActor"/> class.
    /// </summary>
    virtual ~IPhysicsActor() = default;

    /// <summary>
    /// Gets the native physics backend object.
    /// </summary>
    virtual void* GetPhysicsActor() const = 0;

    /// <summary>
    /// Called when actor's active transformation gets changed after the physics simulation step during.
    /// </summary>
    /// <remarks>
    /// This event is called internally by the Physics service and should not be used by the others.
    /// </remarks>
    virtual void OnActiveTransformChanged() = 0;
};
