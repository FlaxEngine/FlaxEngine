// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"


/// <summary>
/// Stage of the game loop at which the coroutine may be suspended.
/// </summary>
/// <remarks>
/// If loop stages are not present in this enum, the coroutines may not be suspended at these points.
/// </remarks>
API_ENUM() enum class CoroutineSuspendPoint : uint8
{
    /// <summary>
    /// Suspension point during or after the OnUpdate event.
    /// </summary>
	Update          = 0,

    /// <summary>
    /// Suspension point during or after the OnLateUpdate event.
    /// </summary>
	LateUpdate      = 1,

    /// <summary>
    /// Suspension point during or after the OnFixedUpdate event.
    /// </summary>
	FixedUpdate     = 2,

    /// <summary>
    /// Suspension point during or after the OnLateFixedUpdate event.
    /// </summary>
	LateFixedUpdate = 3,
};
