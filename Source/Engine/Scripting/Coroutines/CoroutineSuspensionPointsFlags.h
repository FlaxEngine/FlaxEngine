// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"


API_ENUM() enum class CoroutineSuspensionPointsFlags : uint8
{
    None = 0,

    Update = 1 << 0,
    LateUpdate = 1 << 1,
    FixedUpdate = 1 << 2,
    LateFixedUpdate = 1 << 3,
};

DECLARE_ENUM_OPERATORS(CoroutineSuspensionPointsFlags);
