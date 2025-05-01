// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Helper type used to synchronize CPU and GPU work
/// </summary>
typedef uint64 GPUSyncPoint;

/// <summary>
/// Default latency (in frames) between CPU and GPU used for async gpu jobs
/// </summary>
#define GPU_ASYNC_LATENCY 2
