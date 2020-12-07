// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ManagedCLR/MTypes.h"

/// <summary>
/// Helper utility class to quickly scan assemblies to gather metadata for custom editor feature.
/// </summary>
class CustomEditorsUtil
{
public:

    static MonoReflectionType* GetCustomEditor(MonoReflectionType* refType);
};
