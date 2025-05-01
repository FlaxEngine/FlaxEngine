// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Templates.h"

// The utilities for CRC hash generation.
class Crc
{
public:
    // Helper lookup table with cached CRC values.
    static uint32 CachedCRCTablesSB8[8][256];

    // Initializes the CRC lookup table. Must be called before any of the CRC functions are used.
    static void Init();

    // Generates CRC hash of the memory area
    static uint32 MemCrc32(const void* data, int32 length, uint32 crc = 0);
};
