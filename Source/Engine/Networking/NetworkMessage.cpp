// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkMessage.h"

void NetworkMessage::WriteBytes(uint8* bytes, const int numBytes)
{
    ASSERT(Position + numBytes < BufferSize);
    Platform::MemoryCopy(Buffer + Position, bytes, numBytes);
    Position += numBytes;
}

void NetworkMessage::ReadBytes(uint8* bytes, const int numBytes)
{
    ASSERT(Position + numBytes < BufferSize);
    Platform::MemoryCopy(bytes, Buffer + Position, numBytes);
    Position += numBytes;
}

void NetworkMessage::WriteUInt32(uint32 value)
{
    WriteBytes(reinterpret_cast<uint8*>(&value), sizeof(uint32));
}

uint32 NetworkMessage::ReadUInt32()
{
    uint32 value = 0;
    ReadBytes(reinterpret_cast<uint8*>(&value), sizeof(uint32));
    return value;
}
