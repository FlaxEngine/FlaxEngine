// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Message flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class NetworkMessageFlags : uint32
{
    // No flags.
    None = 0,
    // Indicates an error occurred during reading/writing to the message buffer.
    HasError = 1,
};

DECLARE_ENUM_OPERATORS(NetworkMessageFlags);

/// <summary>
/// Network message structure. Provides raw data writing and reading to the message buffer.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking", NoDefault) struct FLAXENGINE_API NetworkMessage
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkMessage);

public:
    /// <summary>
    /// The raw message buffer.
    /// </summary>
    API_FIELD() uint8* Buffer = nullptr;

    /// <summary>
    /// The size in bytes of the buffer that this message has.
    /// </summary>
    API_FIELD() uint32 BufferSize = 0;

    /// <summary>
    /// The position in bytes in buffer where the next read/write will occur.
    /// </summary>
    API_FIELD() uint32 Position = 0;

    /// <summary>
    /// The unique, internal message identifier.
    /// </summary>
    API_FIELD() uint32 MessageId = 0;

    /// <summary>
    /// Set of flags that describe the message state.
    /// </summary>
    API_FIELD() NetworkMessageFlags Flags = NetworkMessageFlags::None;

public:
    /// <summary>
    /// Initializes default values of the <seealso cref="NetworkMessage"/> structure.
    /// </summary>
    NetworkMessage() = default;

    /// <summary>
    /// Initializes values of the <seealso cref="NetworkMessage"/> structure.
    /// </summary>
    NetworkMessage(uint8* buffer, uint32 bufferSize, uint32 messageId = 0)
        : Buffer(buffer)
        , BufferSize(bufferSize)
        , MessageId(messageId)
    {
    }

    ~NetworkMessage() = default;

public:
    /// <summary>
    /// Writes raw bytes into the message.
    /// </summary>
    /// <param name="bytes">The bytes that will be written.</param>
    /// <param name="length">The amount of bytes to write from the pointer.</param>
    void WriteBytes(const void* bytes, const int32 length)
    {
        if (Position + length > BufferSize)
        {
            Flags |= NetworkMessageFlags::HasError;
            return;
        }
        Platform::MemoryCopy(Buffer + Position, bytes, length);
        Position += length;
    }

    /// <summary>
    /// Reads raw bytes from the message into the given byte array.
    /// </summary>
    /// <param name="bytes">The buffer pointer that will be used to store the bytes. Should be of the same length as length or longer.</param>
    /// <param name="length">The minimal amount of bytes that the buffer contains.</param>
    void ReadBytes(void* bytes, const int32 length)
    {
        if (Position + length > BufferSize)
        {
            Flags |= NetworkMessageFlags::HasError;
            return;
        }
        Platform::MemoryCopy(bytes, Buffer + Position, length);
        Position += length;
    }

    /// <summary>
    /// Skips bytes from the message.
    /// </summary>
    /// <param name="length">Amount of bytes to skip.</param>
    /// <returns>Pointer to skipped data beginning.</returns>
    void* SkipBytes(const int32 length)
    {
        if (Position + length > BufferSize)
        {
            Flags |= NetworkMessageFlags::HasError;
            return nullptr;
        }
        byte* result = Buffer + Position;
        Position += length;
        return result;
    }

    template<typename T>
    FORCE_INLINE void WriteStructure(const T& data)
    {
        WriteBytes(&data, sizeof(data));
    }

    template<typename T>
    FORCE_INLINE void ReadStructure(const T& data)
    {
        ReadBytes((T*)&data, sizeof(data));
    }

#define DECL_READWRITE(type, name) \
    void Write##name(type value) { WriteBytes(&value, sizeof(type)); } \
    type Read##name() { type value = 0; ReadBytes(&value, sizeof(type)); return value; }
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
#undef DECL_READWRITE

    /// <summary>
    /// Writes data of type Vector2 into the message.
    /// </summary>
    void WriteVector2(const Vector2& value)
    {
        WriteSingle((float)value.X);
        WriteSingle((float)value.Y);
    }

    /// <summary>
    /// Reads and returns data of type Vector2 from the message.
    /// </summary>
    Vector2 ReadVector2()
    {
        return Vector2(ReadSingle(), ReadSingle());
    }

    /// <summary>
    /// Writes data of type Vector3 into the message.
    /// </summary>
    void WriteVector3(const Vector3& value)
    {
        WriteSingle((float)value.X);
        WriteSingle((float)value.Y);
        WriteSingle((float)value.Z);
    }

    /// <summary>
    /// Reads and returns data of type Vector3 from the message.
    /// </summary>
    Vector3 ReadVector3()
    {
        return Vector3(ReadSingle(), ReadSingle(), ReadSingle());
    }

    /// <summary>
    /// Writes data of type Vector4 into the message.
    /// </summary>
    void WriteVector4(const Vector4& value)
    {
        WriteSingle((float)value.X);
        WriteSingle((float)value.Y);
        WriteSingle((float)value.Z);
        WriteSingle((float)value.W);
    }

    /// <summary>
    /// Reads and returns data of type Vector4 from the message.
    /// </summary>
    Vector4 ReadVector4()
    {
        return Vector4(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
    }

    /// <summary>
    /// Writes data of type Quaternion into the message.
    /// </summary>
    void WriteQuaternion(const Quaternion& value)
    {
        WriteBytes(&value, sizeof(Quaternion));
    }

    /// <summary>
    /// Reads and returns data of type Quaternion from the message.
    /// </summary>
    Quaternion ReadQuaternion()
    {
        Quaternion result = Quaternion::Identity;
        ReadBytes(&result, sizeof(Quaternion));
        return result;
    }

    /// <summary>
    /// Writes data of type String into the message. UTF-16 encoded.
    /// </summary>
    void WriteString(const StringView& value)
    {
        WriteUInt16(value.Length()); // TODO: Use 1-byte length when possible
        WriteBytes(value.Get(), value.Length() * sizeof(Char));
    }

    /// <summary>
    /// Writes data of type String into the message.
    /// </summary>
    void WriteStringAnsi(const StringAnsiView& value)
    {
        WriteUInt16(value.Length()); // TODO: Use 1-byte length when possible
        WriteBytes(value.Get(), value.Length());
    }

    /// <summary>
    /// Reads and returns data of type String from the message. UTF-16 encoded. Data valid within message lifetime.
    /// </summary>
    StringView ReadString()
    {
        const uint16 length = ReadUInt16();
        if (length)
        {
            auto str = SkipBytes(length * 2);
            if (str)
                return StringView((const Char*)str, length);
        }
        return StringView::Empty;
    }

    /// <summary>
    /// Reads and returns data of type String from the message. ANSI encoded. Data valid within message lifetime.
    /// </summary>
    StringAnsiView ReadStringAnsi()
    {
        const uint16 length = ReadUInt16();
        if (length)
        {
            auto str = SkipBytes(length);
            if (str)
                return StringAnsiView((const char*)str, length);
        }
        return StringAnsiView::Empty;
    }

    /// <summary>
    /// Writes data of type Guid into the message.
    /// </summary>
    void WriteGuid(const Guid& value)
    {
        WriteBytes((const uint8*)&value, sizeof(Guid));
    }

    /// <summary>
    /// Reads and returns data of type Guid from the message.
    /// </summary>
    Guid ReadGuid()
    {
        Guid value = Guid::Empty;
        ReadBytes((uint8*)&value, sizeof(Guid));
        return value;
    }

    /// <summary>
    /// Writes identifier into the stream that is networked-synced (by a server). If both peers acknowledge a specific id then the data transfer is optimized to 32 bits.
    /// </summary>
    /// <param name="id">Network-synced identifier.</param>
    void WriteNetworkId(const Guid& id);

    /// <summary>
    /// Reads identifier from the stream that is networked-synced (by a server). If both peers acknowledge a specific id then the data transfer is optimized to 32 bits.
    /// </summary>
    /// <param name="id">Network-synced identifier.</param>
    void ReadNetworkId(Guid& id);

    /// <summary>
    /// Writes name into the stream that is networked-synced (by a server). If both peers acknowledge a specific name then the data transfer is optimized to 32 bits.
    /// </summary>
    /// <param name="name">Network-synced name.</param>
    void WriteNetworkName(const StringAnsiView& name);

    /// <summary>
    /// Reads name from the stream that is networked-synced (by a server). If both peers acknowledge a specific name then the data transfer is optimized to 32 bits.
    /// </summary>
    /// <param name="name">Network-synced name.</param>
    void ReadNetworkName(StringAnsiView& name);

public:
    /// <summary>
    /// Returns true if the message is valid for reading or writing.
    /// </summary>
    bool IsValid() const
    {
        return Buffer != nullptr && BufferSize > 0;
    }
};

template<>
struct TIsPODType<NetworkMessage>
{
    enum { Value = true };
};
