// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Plane.h"
#include "Engine/Utilities/StringConverter.h"
#include "ISerializable.h"

struct CommonValue;
struct Matrix;
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

    FORCE_INLINE void RawValue(const StringAnsi& str)
    {
        RawValue(str.Get(), static_cast<int32>(str.Length()));
    }

    FORCE_INLINE void RawValue(const CharType* json)
    {
        RawValue(json, StringUtils::Length(json));
    }

    FORCE_INLINE void DateTime(const DateTime& value)
    {
        Int64(value.Ticks);
    }

    // Raw bytes blob serialized as base64 string
    void Blob(const void* data, int32 length);

    template<typename T>
    FORCE_INLINE void Enum(const T value)
    {
        Int(static_cast<int32>(value));
    }

    void Vector2(const Vector2& value)
    {
        StartObject();
        JKEY("X");
        Float(value.X);
        JKEY("Y");
        Float(value.Y);
        EndObject();
    }

    void Vector3(const Vector3& value)
    {
        StartObject();
        JKEY("X");
        Float(value.X);
        JKEY("Y");
        Float(value.Y);
        JKEY("Z");
        Float(value.Z);
        EndObject();
    }

    void Vector4(const Vector4& value)
    {
        StartObject();
        JKEY("X");
        Float(value.X);
        JKEY("Y");
        Float(value.Y);
        JKEY("Z");
        Float(value.Z);
        JKEY("W");
        Float(value.W);
        EndObject();
    }

    void Color(const Color& value)
    {
        StartObject();
        JKEY("R");
        Float(value.R);
        JKEY("G");
        Float(value.G);
        JKEY("B");
        Float(value.B);
        JKEY("A");
        Float(value.A);
        EndObject();
    }

    void Quaternion(const Quaternion& value)
    {
        StartObject();
        JKEY("X");
        Float(value.X);
        JKEY("Y");
        Float(value.Y);
        JKEY("Z");
        Float(value.Z);
        JKEY("W");
        Float(value.W);
        EndObject();
    }

    void Ray(const Ray& value)
    {
        StartObject();
        JKEY("Position");
        Vector3(value.Position);
        JKEY("Direction");
        Vector3(value.Direction);
        EndObject();
    }

    void Matrix(const Matrix& value);
    void CommonValue(const CommonValue& value);

    void Transform(const ::Transform& value)
    {
        StartObject();
        if (!value.Translation.IsZero())
        {
            JKEY("Translation");
            Vector3(value.Translation);
        }
        if (!value.Orientation.IsIdentity())
        {
            JKEY("Orientation");
            Quaternion(value.Orientation);
        }
        if (!value.Scale.IsOne())
        {
            JKEY("Scale");
            Vector3(value.Scale);
        }
        EndObject();
    }

    void Transform(const ::Transform& value, const ::Transform* other)
    {
        StartObject();
        if (!other || !Vector3::NearEqual(value.Translation, other->Translation))
        {
            JKEY("Translation");
            Vector3(value.Translation);
        }
        if (!other || !Quaternion::NearEqual(value.Orientation, other->Orientation))
        {
            JKEY("Orientation");
            Quaternion(value.Orientation);
        }
        if (!other || !Vector3::NearEqual(value.Scale, other->Scale))
        {
            JKEY("Scale");
            Vector3(value.Scale);
        }
        EndObject();
    }

    void Plane(const Plane& value)
    {
        StartObject();
        JKEY("Normal");
        Vector3(value.Normal);
        JKEY("D");
        Float(value.D);
        EndObject();
    }

    void Rectangle(const Rectangle& value)
    {
        StartObject();
        JKEY("Location");
        Vector2(value.Location);
        JKEY("Size");
        Vector2(value.Size);
        EndObject();
    }

    void BoundingSphere(const BoundingSphere& value)
    {
        StartObject();
        JKEY("Center");
        Vector3(value.Center);
        JKEY("Radius");
        Float(value.Radius);
        EndObject();
    }

    void BoundingBox(const BoundingBox& value)
    {
        StartObject();
        JKEY("Minimum");
        Vector3(value.Minimum);
        JKEY("Maximum");
        Vector3(value.Maximum);
        EndObject();
    }

    void Guid(const Guid& value);

    void Object(ISerializable* value, const void* otherObj);

    // Serializes scene object (handles prefab with diff serialization)
    void SceneObject(class SceneObject* obj);

public:

    void Array(const ::Guid* value, int32 count)
    {
        StartArray();
        for (int32 i = 0; i < count; i++)
        {
            Guid(value[i]);
        }
        EndArray(count);
    }
};
