// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/File.h"
#include "MemoryWriteStream.h"

MemoryWriteStream::MemoryWriteStream()
    : _buffer(nullptr)
    , _position(nullptr)
    , _capacity(0)
{
}

MemoryWriteStream::MemoryWriteStream(uint32 capacity)
    : _capacity(capacity)
{
    _buffer = capacity > 0 ? (byte*)Allocator::Allocate(capacity) : nullptr;
    _position = _buffer;
}

MemoryWriteStream::~MemoryWriteStream()
{
    Allocator::Free(_buffer);
}

void* MemoryWriteStream::Move(uint32 bytes)
{
    const uint32 position = GetPosition();

    // Check if there is need to update a buffer size
    if (_capacity - position < bytes)
    {
        // Perform reallocation
        uint32 newCapacity = _capacity != 0 ? _capacity * 2 : 256;
        while (newCapacity < position + bytes)
            newCapacity *= 2;
        byte* newBuf = (byte*)Allocator::Allocate(newCapacity);
        Platform::MemoryCopy(newBuf, _buffer, _capacity);
        Allocator::Free(_buffer);

        // Update state
        _buffer = newBuf;
        _capacity = newCapacity;
        _position = _buffer + position;
    }

    // Skip bytes
    _position += bytes;

    // Return pointer to begin
    return _buffer + position;
}

void MemoryWriteStream::Reset(uint32 capacity)
{
    // Check if resize
    if (capacity > _capacity)
    {
        Allocator::Free(_buffer);
        _buffer = (byte*)Allocator::Allocate(capacity);
        _capacity = capacity;
    }

    // Reset pointer
    _position = _buffer;
}

bool MemoryWriteStream::SaveToFile(const StringView& path) const
{
    // Open file for writing
    auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::Read);
    if (file == nullptr)
    {
        return true;
    }

    // Write data
    uint32 bytesWritten;
    file->Write(GetHandle(), GetPosition(), &bytesWritten);

    Delete(file);
    return false;
}

void MemoryWriteStream::Flush()
{
    // Nothing to do
}

void MemoryWriteStream::Close()
{
    Allocator::Free(_buffer);
    _buffer = nullptr;
    _position = nullptr;
    _capacity = 0;
}

uint32 MemoryWriteStream::GetLength()
{
    return _capacity;
}

uint32 MemoryWriteStream::GetPosition()
{
    return static_cast<uint32>(_position - _buffer);
}

void MemoryWriteStream::SetPosition(uint32 seek)
{
    _position = _buffer + seek;
}

void MemoryWriteStream::WriteBytes(const void* data, uint32 bytes)
{
    // Calculate current position
    const uint32 position = GetPosition();

    // Check if there is need to update a buffer size
    if (_capacity - position < bytes)
    {
        // Perform reallocation
        uint32 newCapacity = _capacity != 0 ? _capacity * 2 : 256;
        while (newCapacity < position + bytes)
            newCapacity *= 2;
        byte* newBuf = (byte*)Allocator::Allocate(newCapacity);
        Platform::MemoryCopy(newBuf, _buffer, _capacity);
        Allocator::Free(_buffer);

        // Update state
        _buffer = newBuf;
        _capacity = newCapacity;
        _position = _buffer + position;
    }

    // Copy data
    Platform::MemoryCopy(_position, data, bytes);
    _position += bytes;
}
