// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"


API_ENUM() enum class CoroutineSuspensionPointIndex : uint8
{
	Update = 0,
	LateUpdate = 1,
	FixedUpdate = 2,
	LateFixedUpdate = 3,
};
