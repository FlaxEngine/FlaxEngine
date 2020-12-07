// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Templates.h"

// The CRC hash generation for different types of input data.
class Crc
{
public:

    // Lookup table with pre-calculated CRC values - slicing by 8 implementation.
    static uint32 CRCTablesSB8[8][256];

    // Initializes the CRC lookup table. Must be called before any of the CRC functions are used.
    static void Init();

    // Generates CRC hash of the memory area
    static uint32 MemCrc32(const void* data, int32 length, uint32 crc = 0);

    // String CRC.
    template<typename CharType>
    static typename TEnableIf<sizeof(CharType) != 1, uint32>::Type StrCrc32(const CharType* data, uint32 crc = 0)
    {
        // We ensure that we never try to do a StrCrc32 with a CharType of more than 4 bytes.  This is because
        // we always want to treat every CRC as if it was based on 4 byte chars, even if it's less, because we
        // want consistency between equivalent strings with different character types.
        static_assert(sizeof(CharType) <= 4, "StrCrc32 only works with CharType up to 32 bits.");

        crc = ~crc;
        while (CharType c = *data++)
        {
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ c) & 0xFF];
            c >>= 8;
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ c) & 0xFF];
            c >>= 8;
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ c) & 0xFF];
            c >>= 8;
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ c) & 0xFF];
        }
        return ~crc;
    }

    template<typename CharType>
    static typename TEnableIf<sizeof(CharType) == 1, uint32>::Type StrCrc32(const CharType* data, uint32 crc = 0)
    {
        // Overload for when CharType is a byte, which causes warnings when right-shifting by 8
        crc = ~crc;
        while (CharType Ch = *data++)
        {
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ Ch) & 0xFF];
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc) & 0xFF];
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc) & 0xFF];
            crc = (crc >> 8) ^ CRCTablesSB8[0][(crc) & 0xFF];
        }
        return ~crc;
    }
};
