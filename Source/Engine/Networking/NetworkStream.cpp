// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "NetworkStream.h"
#include "INetworkSerializable.h"

NetworkStream::NetworkStream(const SpawnParams& params)
    : ScriptingObject(params)
    , ReadStream()
    , WriteStream()
{
}

NetworkStream::~NetworkStream()
{
    if (_allocated)
        Allocator::Free(_buffer);
}

void NetworkStream::Initialize(uint32 minCapacity)
{
    // Unlink buffer if was reading from memory
    if (!_allocated)
        _buffer = nullptr;

    // Allocate if buffer is missing or too small
    if (!_buffer || _length < minCapacity)
    {
        // Release previous
        if (_buffer)
            Allocator::Free(_buffer);

        // Allocate new one
        _buffer = (byte*)Allocator::Allocate(minCapacity);
        _length = minCapacity;
        _allocated = true;
    }

    // Reset pointer to the start
    _position = _buffer;
}

void NetworkStream::Initialize(byte* buffer, uint32 length)
{
    if (_allocated)
        Allocator::Free(_buffer);
    _position = _buffer = buffer;
    _length = length;
    _allocated = false;
}

void NetworkStream::Read(INetworkSerializable& obj)
{
    obj.Deserialize(this);
}

void NetworkStream::Read(INetworkSerializable* obj)
{
    obj->Deserialize(this);
}

void NetworkStream::Write(INetworkSerializable& obj)
{
    obj.Serialize(this);
}

void NetworkStream::Write(INetworkSerializable* obj)
{
    obj->Serialize(this);
}

void NetworkStream::Flush()
{
    // Nothing to do
}

void NetworkStream::Close()
{
    if (_allocated)
        Allocator::Free(_buffer);
    _position = _buffer = nullptr;
    _length = 0;
    _allocated = false;
}

uint32 NetworkStream::GetLength()
{
    return _length;
}

uint32 NetworkStream::GetPosition()
{
    return static_cast<uint32>(_position - _buffer);
}

void NetworkStream::SetPosition(uint32 seek)
{
    ASSERT(_length > 0);
    _position = _buffer + seek;
}

void NetworkStream::ReadBytes(void* data, uint32 bytes)
{
    if (bytes > 0)
    {
        ASSERT(data && GetLength() - GetPosition() >= bytes);
        Platform::MemoryCopy(data, _position, bytes);
        _position += bytes;
    }
}

void NetworkStream::WriteBytes(const void* data, uint32 bytes)
{
    // Calculate current position
    const uint32 position = GetPosition();

    // Check if there is need to update a buffer size
    if (_length - position < bytes)
    {
        // Perform reallocation
        uint32 newLength = _length != 0 ? _length * 2 : 256;
        while (newLength < position + bytes)
            newLength *= 2;
        byte* newBuf = (byte*)Allocator::Allocate(newLength);
        if (newBuf == nullptr)
        {
            OUT_OF_MEMORY;
        }
        if (_buffer && _length)
            Platform::MemoryCopy(newBuf, _buffer, _length);
        if (_allocated)
            Allocator::Free(_buffer);

        // Update state
        _buffer = newBuf;
        _length = newLength;
        _position = _buffer + position;
        _allocated = true;
    }

    // Copy data
    Platform::MemoryCopy(_position, data, bytes);
    _position += bytes;
}
