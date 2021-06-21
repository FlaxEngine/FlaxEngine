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

bool NetworkMessage::IsValid() const
{
    return Buffer != nullptr && BufferSize > 0;
}
