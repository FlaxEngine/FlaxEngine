// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Physics/Types.h"
#include "IPhysicsActor.h"

class PhysicsScene;

/// <summary>
/// A base class for all physical actors.
/// </summary>
/// <seealso cref="Actor" />
API_CLASS(Abstract) class FLAXENGINE_API PhysicsActor : public Actor, public IPhysicsActor
{
DECLARE_SCENE_OBJECT_ABSTRACT(PhysicsActor);
protected:

    Vector3 _cachedScale;
    bool _isUpdatingTransform;

public:

    /// <summary>
    /// Gets the native PhysX actor object.
    /// </summary>
    /// <returns>The PhysX actor.</returns>
    virtual PxActor* GetPhysXActor() = 0;

    /// <summary>
    /// Updates the bounding box.
    /// </summary>
    void UpdateBounds();

public:

    // [Actor]
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;

    // [IPhysicsActor]
    void OnActiveTransformChanged(const PxTransform& transform) override;

protected:

    // [Actor]
    void OnTransformChanged() override;
};
