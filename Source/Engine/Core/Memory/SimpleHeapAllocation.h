// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Types/BaseTypes.h"

// Base class for custom heap-based allocators (eg. with local pooling/paging). Expects only Allocate/Free methods to be provided.
template<typename This, uint32 InitialCapacity = 8>
using SimpleHeapAllocation = HeapAllocation;
