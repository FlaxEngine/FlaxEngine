// Copyright (c) Wojciech Figat. All rights reserved.

#include "MemoryReadStream.h"
#include "Engine/Platform/Platform.h"

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

void* MemoryReadStream::Move(uint32 bytes)
{
    ASSERT(GetLength() - GetPosition() >= bytes);
    const auto result = (void*)_position;
    _position += bytes;
    return result;
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
        if (!data || GetLength() - GetPosition() < bytes)
        {
            _hasError = true;
            return;
        }
        Platform::MemoryCopy(data, _position, bytes);
        _position += bytes;
    }
}
