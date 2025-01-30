// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Tells if the object is occupied, and if not, if the bucket is a subject of compaction.
/// </summary>
enum class BucketState : byte
{
    Empty = 0,
    Deleted = 1,
    Occupied = 2,
};
