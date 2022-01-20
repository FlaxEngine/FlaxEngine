// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Physics/Types.h"
#include "IPhysicsActor.h"

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
    /// Updates the bounding box.
    /// </summary>
    void UpdateBounds();

public:

    // [Actor]
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;

    // [IPhysicsActor]
    void OnActiveTransformChanged() override;

protected:

    // [Actor]
    void OnTransformChanged() override;
};
