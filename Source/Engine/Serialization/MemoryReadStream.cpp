// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "MemoryReadStream.h"

MemoryReadStream::MemoryReadStream()
    : _buffer(nullptr)
    , _position(nullptr)
    , _length(0)
{
}

MemoryReadStream::MemoryReadStream(const byte* bytes, uint32 length)
    : _buffer(bytes)
    , _position(bytes)
    , _length(length)
{
}

void MemoryReadStream::Init(const byte* bytes, uint32 length)
{
    _buffer = _position = bytes;
    _length = length;
}

void MemoryReadStream::Flush()
{
}

void MemoryReadStream::Close()
{
    _position = _buffer = nullptr;
    _length = 0;
}

uint32 MemoryReadStream::GetLength()
{
    return _length;
}

uint32 MemoryReadStream::GetPosition()
{
    return static_cast<uint32>(_position - _buffer);
}

void MemoryReadStream::SetPosition(uint32 seek)
{
    ASSERT(_length > 0);
    _position = _buffer + seek;
}

void MemoryReadStream::ReadBytes(void* data, uint32 bytes)
{
    if (bytes > 0)
    {
        ASSERT(data && GetLength() - GetPosition() >= bytes);
        Platform::MemoryCopy(data, _position, bytes);
        _position += bytes;
    }
}
