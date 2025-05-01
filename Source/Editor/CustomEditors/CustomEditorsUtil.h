// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ManagedCLR/MTypes.h"

/// <summary>
/// Helper utility class to quickly scan assemblies to gather metadata for custom editor feature.
/// </summary>
class CustomEditorsUtil
{
public:

#if USE_CSHARP
    static MTypeObject* GetCustomEditor(MTypeObject* refType);
#endif
};
