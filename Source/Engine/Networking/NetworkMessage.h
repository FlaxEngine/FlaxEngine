// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Network message structure. Provides raw data writing and reading to the message buffer.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkMessage
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkMessage);
public:
    /// <summary>
    /// The raw message buffer.
    /// </summary>
    API_FIELD()
    uint8* Buffer = nullptr;

    /// <summary>
    /// The unique, internal message identifier.
    /// </summary>
    API_FIELD()
    uint32 MessageId = 0;

    /// <summary>
    /// The size in bytes of the buffer that this message has.
    /// </summary>
    API_FIELD()
    uint32 BufferSize = 0;

    /// <summary>
    /// The length in bytes of this message.
    /// </summary>
    API_FIELD()
    uint32 Length = 0;

    /// <summary>
    /// The position in bytes in buffer where the next read/write will occur.
    /// </summary>
    API_FIELD()
    uint32 Position = 0;

public:
    /// <summary>
    /// Initializes default values of the <seealso cref="NetworkMessage"/> structure.
    /// </summary>
    NetworkMessage() = default;

    /// <summary>
    /// Initializes values of the <seealso cref="NetworkMessage"/> structure.
    /// </summary>
    NetworkMessage(uint8* buffer, uint32 messageId, uint32 bufferSize, uint32 length, uint32 position) :
        Buffer(buffer), MessageId(messageId), BufferSize(bufferSize), Length(length), Position(position)
    { }

    ~NetworkMessage() = default;
    
public:
    /// <summary>
    /// Writes raw bytes into the message.
    /// </summary>
    /// <param name="bytes">The bytes that will be written.</param>
    /// <param name="numBytes">The amount of bytes to write from the bytes pointer.</param>
    FORCE_INLINE void WriteBytes(uint8* bytes, const int numBytes)
    {
        ASSERT(Position + numBytes < BufferSize);
        Platform::MemoryCopy(Buffer + Position, bytes, numBytes);
        Position += numBytes;
    }
    
    /// <summary>
    /// Reads raw bytes from the message into the given byte array.
    /// </summary>
    /// <param name="bytes">
    /// The buffer pointer that will be used to store the bytes.
    /// Should be of the same length as length or longer.
    /// </param>
    /// <param name="numBytes">The minimal amount of bytes that the buffer contains.</param>
    FORCE_INLINE void ReadBytes(uint8* bytes, const int numBytes)
    {
        ASSERT(Position + numBytes < BufferSize);
        Platform::MemoryCopy(bytes, Buffer + Position, numBytes);
        Position += numBytes;
    }

#define DECL_READWRITE(type, name) \
    FORCE_INLINE void Write##name(type value) { WriteBytes(reinterpret_cast<uint8*>(&value), sizeof(type)); } \
    FORCE_INLINE type Read##name() { type value = 0; ReadBytes(reinterpret_cast<uint8*>(&value), sizeof(type)); return value; }

public:
    DECL_READWRITE(int8, Int8)
    DECL_READWRITE(uint8, UInt8)
    DECL_READWRITE(int16, Int16)
    DECL_READWRITE(uint16, UInt16)
    DECL_READWRITE(int32, Int32)
    DECL_READWRITE(uint32, UInt32)
    DECL_READWRITE(int64, Int64)
    DECL_READWRITE(uint64, UInt64)
    DECL_READWRITE(float, Single)
    DECL_READWRITE(double, Double)
    DECL_READWRITE(bool, Boolean)

public:
    /// <summary>
    /// Writes data of type Vector2 into the message.
    /// </summary>
    FORCE_INLINE void WriteVector2(const Vector2& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
    }

    /// <summary>
    /// Reads and returns data of type Vector2 from the message.
    /// </summary>
    FORCE_INLINE Vector2 ReadVector2()
    {
        return Vector2(ReadSingle(), ReadSingle());
    }
    
    /// <summary>
    /// Writes data of type Vector3 into the message.
    /// </summary>
    FORCE_INLINE void WriteVector3(const Vector3& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
        WriteSingle(value.Z);
    }

    /// <summary>
    /// Reads and returns data of type Vector3 from the message.
    /// </summary>
    FORCE_INLINE Vector3 ReadVector3()
    {
        return Vector3(ReadSingle(), ReadSingle(), ReadSingle());
    }

    /// <summary>
    /// Writes data of type Vector4 into the message.
    /// </summary>
    FORCE_INLINE void WriteVector4(const Vector4& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
        WriteSingle(value.Z);
        WriteSingle(value.W);
    }

    /// <summary>
    /// Reads and returns data of type Vector4 from the message.
    /// </summary>
    FORCE_INLINE Vector4 ReadVector4()
    {
        return Vector4(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
    }
    
    /// <summary>
    /// Writes data of type Quaternion into the message.
    /// </summary>
    FORCE_INLINE void WriteQuaternion(const Quaternion& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
        WriteSingle(value.Z);
        WriteSingle(value.W);
    }

    /// <summary>
    /// Reads and returns data of type Quaternion from the message.
    /// </summary>
    FORCE_INLINE Quaternion ReadQuaternion()
    {
        return Quaternion(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
    }

public:
    /// <summary>
    /// Writes data of type String into the message. UTF-16 encoded.
    /// </summary>
    FORCE_INLINE void WriteString(const String& value)
    {
        WriteUInt16(value.Length()); // TODO: Use 1-byte length when possible
        WriteBytes((uint8*)value.Get(), value.Length() * sizeof(Char));
    }

    /// <summary>
    /// Reads and returns data of type String from the message. UTF-16 encoded.
    /// </summary>
    FORCE_INLINE String ReadString()
    {
        uint16 length = ReadUInt16();
        String value;
        value.Resize(length);
        ReadBytes((uint8*)value.Get(), length * sizeof(Char));
        return value;
    }
    
    /// <summary>
    /// Writes data of type Guid into the message.
    /// </summary>
    FORCE_INLINE void WriteGuid(const Guid& value)
    {
        WriteBytes((uint8*)&value, sizeof(Guid));
    }

    /// <summary>
    /// Reads and returns data of type Guid from the message.
    /// </summary>
    FORCE_INLINE Guid ReadGuid()
    {
        Guid value = Guid();
        ReadBytes((uint8*)&value, sizeof(Guid));
        return value;
    }

public:
    /// <summary>
    /// Returns true if the message is valid for reading or writing.
    /// </summary>
    bool IsValid() const
    {
        return Buffer != nullptr && BufferSize > 0;
    }
};
