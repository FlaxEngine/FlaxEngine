// Copyright (c) Wojciech Figat. All rights reserved.

#include "EmptyActor.h"

EmptyActor::EmptyActor(const SpawnParams& params)
    : Actor(params)
{
}

void EmptyActor::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
