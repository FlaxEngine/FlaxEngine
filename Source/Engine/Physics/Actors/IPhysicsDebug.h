// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

#if USE_EDITOR
class FLAXENGINE_API IPhysicsDebug
{
public:
    virtual void DrawPhysicsDebug(struct RenderView& view)
    {
    }
};
#define ImplementPhysicsDebug void DrawPhysicsDebug(RenderView& view)
#else
#define ImplementPhysicsDebug
#endif
