// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Object.h"
#include "ObjectsRemovalService.h"

RemovableObject::~RemovableObject()
{
#if BUILD_DEBUG
    // Prevent removing object that is still reverenced by the removal service
    ASSERT(!ObjectsRemovalService::IsInPool(this));
#endif
}

void RemovableObject::DeleteObjectNow()
{
    ObjectsRemovalService::Dereference(this);

    OnDeleteObject();
}

void RemovableObject::DeleteObject(float timeToLive, bool useGameTime)
{
    // Add to deferred remove (or just update timeout but don't remove object here)
    ObjectsRemovalService::Add(this, timeToLive, useGameTime);
}
