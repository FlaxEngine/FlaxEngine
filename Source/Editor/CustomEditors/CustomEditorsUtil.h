// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ManagedCLR/MTypes.h"

/// <summary>
/// Helper utility class to quickly scan assemblies to gather metadata for custom editor feature.
/// </summary>
class CustomEditorsUtil
{
public:

#if USE_MONO
    static MonoReflectionType* GetCustomEditor(MonoReflectionType* refType);
#endif
};
