// Copyright (c) Wojciech Figat. All rights reserved.

#include "NetworkStream.h"
#include "INetworkSerializable.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Transform.h"

// Quaternion quantized for optimized network data size.
struct NetworkQuaternion
{
    enum Flag : uint8
    {
        None = 0,
        HasX = 1,
        HasY = 2,
        HasZ = 4,
        NegativeX = 8,
        NegativeY = 16,
        NegativeZ = 32,
        NegativeW = 64,
    };

    FORCE_INLINE static void Read(NetworkStream* stream, Quaternion& data)
    {
        uint8 flags;
        stream->Read(flags);
        if (flags == None)
        {
            // Early out on default value
            data = Quaternion::Identity;
            return;
        }

        Quaternion raw = Quaternion::Identity;
#define READ_COMPONENT(comp, hasFlag, negativeFlag) \
    if (flags & hasFlag) \
    { \
        uint16 packed; \
        stream->Read(packed); \
        const float norm = (float)packed / (float)MAX_uint16; \
        raw.comp = norm; \
        if (flags & negativeFlag) \
            raw.comp = -raw.comp; \
    }
        READ_COMPONENT(X, HasX, NegativeX);
        READ_COMPONENT(Y, HasY, NegativeY);
        READ_COMPONENT(Z, HasZ, NegativeZ);
#define READ_COMPONENT

        // Calculate W
        raw.W = Math::Sqrt(Math::Max(1.0f - raw.X * raw.X - raw.Y * raw.Y - raw.Z * raw.Z, 0.0f));
        if (flags & NegativeW)
            raw.W = -raw.W;

        raw.Normalize();
        data = raw;
    }

    FORCE_INLINE static void Write(NetworkStream* stream, const Quaternion& data)
    {
        // Assumes rotation is normalized so W can be recalculated
        Quaternion raw = data;
        raw.Normalize();

        // Compose flags that describe the data
        uint8 flags = HasX | HasY | HasZ;
#define QUANTIZE_COMPONENT(comp, hasFlag, negativeFlag) \
    if (Math::IsZero(raw.comp)) \
        flags &= ~hasFlag; \
    else if (raw.comp < 0.0f) \
        flags |= negativeFlag
        QUANTIZE_COMPONENT(X, HasX, NegativeX);
        QUANTIZE_COMPONENT(Y, HasY, NegativeY);
        QUANTIZE_COMPONENT(Z, HasZ, NegativeZ);
        if (raw.W < 0.0f)
            flags |= NegativeW;
#undef QUANTIZE_COMPONENT

        // Write data
        stream->Write(flags);
#define WRITE_COMPONENT(comp, hasFlag)  \
    if (flags & hasFlag) \
    { \
        const float norm = Math::Abs(raw.comp); \
        const uint16 packed = (uint16)(norm * MAX_uint16); \
        stream->Write(packed); \
    }
        WRITE_COMPONENT(X, HasX);
        WRITE_COMPONENT(Y, HasY);
        WRITE_COMPONENT(Z, HasZ);
#undef WRITE_COMPONENT
    }
};

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

void NetworkStream::Read(Quaternion& data)
{
    NetworkQuaternion::Read(this, data);
}

void NetworkStream::Read(Transform& data, bool useDouble)
{
    struct NonQuantized
    {
        Vector3 Pos;
        Float3 Scale;
    };
    NonQuantized nonQuantized;
    ReadBytes(&nonQuantized, sizeof(nonQuantized));
    NetworkQuaternion::Read(this, data.Orientation);
    data.Translation = nonQuantized.Pos;
    data.Scale = nonQuantized.Scale;
}

void NetworkStream::Write(INetworkSerializable& obj)
{
    obj.Serialize(this);
}

void NetworkStream::Write(INetworkSerializable* obj)
{
    obj->Serialize(this);
}

void NetworkStream::Write(const Quaternion& data)
{
    NetworkQuaternion::Write(this, data);
}

void NetworkStream::Write(const Transform& data, bool useDouble)
{
    struct NonQuantized
    {
        Vector3 Pos;
        Float3 Scale;
    };
    // TODO: quantize transform (at least scale)
    NonQuantized nonQuantized = { data.Translation, data.Scale };
    WriteBytes(&nonQuantized, sizeof(nonQuantized));
    NetworkQuaternion::Write(this, data.Orientation);
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
