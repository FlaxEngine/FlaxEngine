// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

API_STRUCT() struct FLAXENGINE_API NetworkMessage
{
public:
    API_FIELD()
    uint8* Buffer;
    
    API_FIELD()
    uint32 MessageId; // TODO: Make it read-only
    
    API_FIELD()
    uint32 BufferSize;
    
    API_FIELD()
    uint32 Length;
    
    API_FIELD()
    uint32 Position;
    
public:
    API_FUNCTION() void WriteBytes(uint8* bytes, int numBytes);
    API_FUNCTION() void ReadBytes(uint8* bytes, int numBytes);

public:
    API_FUNCTION() void WriteUInt32(uint32 value); // TODO: Macro the shit out of this
    API_FUNCTION() uint32 ReadUInt32();

public:
    API_FUNCTION() bool IsValid() const
    {
        return Buffer != nullptr && BufferSize > 0;
    }
};
