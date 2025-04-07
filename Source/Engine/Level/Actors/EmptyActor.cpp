// Copyright (c) Wojciech Figat. All rights reserved.

#include "EmptyActor.h"

EmptyActor::EmptyActor(const SpawnParams& params)
    : Actor(params)
{
}

#if USE_EDITOR

BoundingBox EmptyActor::GetEditorBox() const
{
    const Vector3 size(50);
    return BoundingBox(_transform.Translation - size, _transform.Translation + size);
}

#endif

void EmptyActor::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
