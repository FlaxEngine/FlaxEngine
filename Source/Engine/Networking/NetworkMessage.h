// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/ScriptingType.h"

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkMessage
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkMessage);
public:
    API_FIELD()
    uint8* Buffer = nullptr;
    
    API_FIELD()
    uint32 MessageId = 0;
    
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
    FORCE_INLINE void WriteBytes(uint8* bytes, int numBytes);
    FORCE_INLINE void ReadBytes(uint8* bytes, int numBytes);

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
    FORCE_INLINE void WriteVector2(const Vector2& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
    }

    FORCE_INLINE Vector2 ReadVector2()
    {
        return Vector2(ReadSingle(), ReadSingle());
    }
    
    FORCE_INLINE void WriteVector3(const Vector3& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
        WriteSingle(value.Z);
    }

    FORCE_INLINE Vector3 ReadVector3()
    {
        return Vector3(ReadSingle(), ReadSingle(), ReadSingle());
    }
    
    FORCE_INLINE void WriteVector4(const Vector4& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
        WriteSingle(value.Z);
        WriteSingle(value.W);
    }

    FORCE_INLINE Vector4 ReadVector4()
    {
        return Vector4(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
    }
    
    FORCE_INLINE void WriteQuaternion(const Quaternion& value)
    {
        WriteSingle(value.X);
        WriteSingle(value.Y);
        WriteSingle(value.Z);
        WriteSingle(value.W);
    }

    FORCE_INLINE Quaternion ReadQuaternion()
    {
        return Quaternion(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
    }

public:
    FORCE_INLINE void WriteQuaternion(const String& value)
    {
        WriteUInt16(value.Length()); // TODO: Use 1-byte length when possible
        WriteBytes((uint8*)value.Get(), value.Length() * 2);
    }

    FORCE_INLINE String ReadString()
    {
        uint16 length = ReadUInt16();
        String value;
        value.Resize(length);
        ReadBytes((uint8*)value.Get(), value.Length() * 2);
        return value;
    }

public:
    bool IsValid() const
    {
        return Buffer != nullptr && BufferSize > 0;
    }
};
