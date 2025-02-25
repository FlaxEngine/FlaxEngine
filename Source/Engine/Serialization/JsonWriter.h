// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Utilities/StringConverter.h"

struct CommonValue;
class ISerializable;

// Helper macro for JSON serialization keys (reduces allocations count)
#define JKEY(keyname) Key(keyname, ARRAY_COUNT(keyname) - 1)

/// <summary>
/// Base class for Json writers.
/// </summary>
class FLAXENGINE_API JsonWriter
{
public:
    typedef char CharType;

    virtual ~JsonWriter() = default;

    virtual void Key(const char* str, int32 length) = 0;
    virtual void String(const char* str, int32 length) = 0;
    virtual void RawValue(const char* str, int32 length) = 0;
    virtual void Bool(bool d) = 0;
    virtual void Int(int32 d) = 0;
    virtual void Int64(int64 d) = 0;
    virtual void Uint(uint32 d) = 0;
    virtual void Uint64(uint64 d) = 0;
    virtual void Float(float d) = 0;
    virtual void Double(double d) = 0;

    virtual void StartObject() = 0;
    virtual void EndObject() = 0;
    virtual void StartArray() = 0;
    virtual void EndArray(int32 count = 0) = 0;

public:
    FORCE_INLINE void Key(const StringAnsiView& str)
    {
        Key(str.Get(), static_cast<unsigned>(str.Length()));
    }

    FORCE_INLINE void Key(const StringView& str)
    {
        const StringAsUTF8<256> buf(*str, str.Length());
        Key(buf.Get(), buf.Length());
    }

    FORCE_INLINE void String(const char* str)
    {
        String(str, StringUtils::Length(str));
    }

    FORCE_INLINE void String(const StringAnsiView& str)
    {
        String(str.Get(), static_cast<unsigned>(str.Length()));
    }

    void String(const Char* str)
    {
        const StringAsUTF8<256> buf(str);
        String(buf.Get());
    }

    void String(const Char* str, const int32 length)
    {
        const StringAsUTF8<256> buf(str, length);
        String(buf.Get());
    }

    void String(const ::String& value)
    {
        const StringAsUTF8<256> buf(*value, value.Length());
        String(buf.Get());
    }

    void String(const StringView& value)
    {
        const StringAsUTF8<256> buf(*value, value.Length());
        String(buf.Get());
    }

    void String(const StringAnsi& value)
    {
        String(value.Get(), value.Length());
    }

    FORCE_INLINE void RawValue(const StringAnsi& str)
    {
        RawValue(str.Get(), str.Length());
    }

    void RawValue(const StringView& str)
    {
        const StringAsUTF8<256> buf(*str, str.Length());
        RawValue(buf.Get(), buf.Length());
    }

    FORCE_INLINE void RawValue(const CharType* json)
    {
        RawValue(json, StringUtils::Length(json));
    }

    // Raw bytes blob serialized as base64 string
    void Blob(const void* data, int32 length);

    template<typename T>
    FORCE_INLINE void Enum(const T value)
    {
        Int(static_cast<int32>(value));
    }

    FORCE_INLINE void Real(Real d)
    {
#if USE_LARGE_WORLDS
        Double(d);
#else
        Float(d);
#endif
    }

    void DateTime(const DateTime& value);
    void Vector2(const Vector2& value);
    void Vector3(const Vector3& value);
    void Vector4(const Vector4& value);
    void Float2(const Float2& value);
    void Float3(const Float3& value);
    void Float4(const Float4& value);
    void Double2(const Double2& value);
    void Double3(const Double3& value);
    void Double4(const Double4& value);
    void Int2(const Int2& value);
    void Int3(const Int3& value);
    void Int4(const Int4& value);
    void Color(const Color& value);
    void Quaternion(const Quaternion& value);
    void Ray(const Ray& value);
    void Matrix(const Matrix& value);
    void CommonValue(const CommonValue& value);
    void Transform(const ::Transform& value);
    void Transform(const ::Transform& value, const ::Transform* other);
    void Plane(const Plane& value);
    void Rectangle(const Rectangle& value);
    void BoundingSphere(const BoundingSphere& value);
    void BoundingBox(const BoundingBox& value);
    void Guid(const Guid& value);
    void Object(ISerializable* value, const void* otherObj);

    // Serializes scene object (handles prefab with diff serialization)
    void SceneObject(class SceneObject* obj);
};
