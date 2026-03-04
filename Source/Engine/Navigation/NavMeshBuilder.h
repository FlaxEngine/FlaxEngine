// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_NAV_MESH_BUILDER

#include "Engine/Core/Types/BaseTypes.h"

class Scene;

/// <summary>
/// The navigation mesh building utility.
/// </summary>
class FLAXENGINE_API NavMeshBuilder
{
public:
    static void Init();
    static void Update();
};

#endif
