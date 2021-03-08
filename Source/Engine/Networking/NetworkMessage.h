// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkMessage
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkMessage);
public:
    API_FIELD()
    uint8* Buffer = nullptr;
    
    API_FIELD()
    uint32 MessageId = 0; // TODO: Make it read-only
    
    API_FIELD()
    uint32 BufferSize = 0;
    
    API_FIELD()
    uint32 Length = 0;
    
    API_FIELD()
    uint32 Position = 0;

public:
    NetworkMessage() = default;
    
    NetworkMessage(uint8* buffer, uint32 messageId, uint32 bufferSize, uint32 length, uint32 position) :
        Buffer(buffer), MessageId(messageId), BufferSize(bufferSize), Length(length), Position(position)
    { }

    ~NetworkMessage() = default;
    
public:
    void WriteBytes(uint8* bytes, int numBytes);
    void ReadBytes(uint8* bytes, int numBytes);

public:
    void WriteUInt32(uint32 value); // TODO: Macro the shit out of this
    uint32 ReadUInt32();

public:
    bool IsValid() const
    {
        return Buffer != nullptr && BufferSize > 0;
    }
};
