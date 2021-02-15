// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

namespace physx
{
    class PxRigidActor;
    class PxTransform;
}

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
    /// Gets the rigid actor (PhysX object) may be null.
    /// </summary>
    /// <returns>PhysX rigid actor or null if not using</returns>
    virtual physx::PxRigidActor* GetRigidActor() = 0;

    /// <summary>
    /// Called when actor's active transformation gets changed after the physics simulation step.
    /// </summary>
    /// <remarks>
    /// This event is called internally by the Physics service and should not be used by the others.
    /// </remarks>
    /// <param name="transform">The current transformation.</param>
    virtual void OnActiveTransformChanged(const physx::PxTransform& transform) = 0;
};
