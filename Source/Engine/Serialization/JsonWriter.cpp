// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "JsonWriter.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Math/Int2.h"
#include "Engine/Core/Math/Int3.h"
#include "Engine/Core/Math/Int4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Plane.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Level/SceneObject.h"
#include "Engine/Utilities/Encryption.h"

void JsonWriter::Blob(const void* data, int32 length)
{
    ::Array<char> base64;
    base64.Resize(Encryption::Base64EncodeLength(length));
    Encryption::Base64Encode((byte*)data, length, base64.Get());
    String(base64.Get(), base64.Count());
}

void JsonWriter::DateTime(const ::DateTime& value)
{
    Int64(value.Ticks);
}

void JsonWriter::Vector2(const ::Vector2& value)
{
    StartObject();
    JKEY("X");
    Float(value.X);
    JKEY("Y");
    Float(value.Y);
    EndObject();
}

void JsonWriter::Vector3(const ::Vector3& value)
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

void JsonWriter::Vector4(const ::Vector4& value)
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

void JsonWriter::Int2(const ::Int2& value)
{
    StartObject();
    JKEY("X");
    Int(value.X);
    JKEY("Y");
    Int(value.Y);
    EndObject();
}

void JsonWriter::Int3(const ::Int3& value)
{
    StartObject();
    JKEY("X");
    Int(value.X);
    JKEY("Y");
    Int(value.Y);
    JKEY("Z");
    Int(value.Z);
    EndObject();
}

void JsonWriter::Int4(const ::Int4& value)
{
    StartObject();
    JKEY("X");
    Int(value.X);
    JKEY("Y");
    Int(value.Y);
    JKEY("Z");
    Int(value.Z);
    JKEY("W");
    Int(value.W);
    EndObject();
}

void JsonWriter::Color(const ::Color& value)
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

void JsonWriter::Quaternion(const ::Quaternion& value)
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

void JsonWriter::Ray(const ::Ray& value)
{
    StartObject();
    JKEY("Position");
    Vector3(value.Position);
    JKEY("Direction");
    Vector3(value.Direction);
    EndObject();
}

void JsonWriter::Matrix(const ::Matrix& value)
{
    StartObject();
    JKEY("M11");
    Float(value.M11);
    JKEY("M12");
    Float(value.M12);
    JKEY("M13");
    Float(value.M13);
    JKEY("M14");
    Float(value.M14);
    JKEY("M21");
    Float(value.M21);
    JKEY("M22");
    Float(value.M22);
    JKEY("M23");
    Float(value.M23);
    JKEY("M24");
    Float(value.M24);
    JKEY("M31");
    Float(value.M31);
    JKEY("M32");
    Float(value.M32);
    JKEY("M33");
    Float(value.M33);
    JKEY("M34");
    Float(value.M34);
    JKEY("M41");
    Float(value.M41);
    JKEY("M42");
    Float(value.M42);
    JKEY("M43");
    Float(value.M43);
    JKEY("M44");
    Float(value.M44);
    EndObject();
}

void JsonWriter::CommonValue(const ::CommonValue& value)
{
    StartObject();

    JKEY("Type");
    Int((int32)value.Type);

    JKEY("Value");
    switch (value.Type)
    {
    case CommonType::Bool:
        Bool(value.AsBool);
        break;
    case CommonType::Integer:
        Int(value.AsInteger);
        break;
    case CommonType::Float:
        Float(value.AsFloat);
        break;
    case CommonType::Vector2:
        Vector2(value.AsVector2);
        break;
    case CommonType::Vector3:
        Vector3(value.AsVector3);
        break;
    case CommonType::Vector4:
        Vector4(value.AsVector4);
        break;
    case CommonType::Color:
        Color(value.AsColor);
        break;
    case CommonType::Guid:
        Guid(value.AsGuid);
        break;
    case CommonType::String:
        String(value.AsString);
        break;
    case CommonType::Box:
        BoundingBox(value.AsBox);
        break;
    case CommonType::Rotation:
        Quaternion(value.AsRotation);
        break;
    case CommonType::Transform:
        Transform(value.AsTransform);
        break;
    case CommonType::Sphere:
        BoundingSphere(value.AsSphere);
        break;
    case CommonType::Rectangle:
        Rectangle(value.AsRectangle);
        break;
    case CommonType::Ray:
        Ray(value.AsRay);
        break;
    case CommonType::Pointer:
        Int64((int64)value.AsPointer);
        break;
    case CommonType::Matrix:
        Matrix(value.AsMatrix);
        break;
    case CommonType::Blob:
    {
        ::Array<char> base64;
        base64.Resize(Encryption::Base64EncodeLength(value.AsBlob.Length));
        Encryption::Base64Encode(value.AsBlob.Data, value.AsBlob.Length, base64.Get());
        String(base64.Get(), base64.Count());
        break;
    }
    case CommonType::Object:
        Guid(value.GetObjectId());
        break;
    default:
        CRASH;
        break;
    }

    EndObject();
}

void JsonWriter::Transform(const ::Transform& value)
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

void JsonWriter::Transform(const ::Transform& value, const ::Transform* other)
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

void JsonWriter::Plane(const ::Plane& value)
{
    StartObject();
    JKEY("Normal");
    Vector3(value.Normal);
    JKEY("D");
    Float(value.D);
    EndObject();
}

void JsonWriter::Rectangle(const ::Rectangle& value)
{
    StartObject();
    JKEY("Location");
    Vector2(value.Location);
    JKEY("Size");
    Vector2(value.Size);
    EndObject();
}

void JsonWriter::BoundingSphere(const ::BoundingSphere& value)
{
    StartObject();
    JKEY("Center");
    Vector3(value.Center);
    JKEY("Radius");
    Float(value.Radius);
    EndObject();
}

void JsonWriter::BoundingBox(const ::BoundingBox& value)
{
    StartObject();
    JKEY("Minimum");
    Vector3(value.Minimum);
    JKEY("Maximum");
    Vector3(value.Maximum);
    EndObject();
}

void JsonWriter::Guid(const ::Guid& value)
{
    // Unoptimized version:
    //Text(value.ToString(Guid::FormatType::N));

    // Optimized version:
	// @formatter:off
	char buffer[32] =
	{
		'0','0', '0', '0', '0', '0', '0', '0',
		'0','0', '0', '0', '0', '0', '0', '0',
		'0','0', '0', '0', '0', '0', '0', '0',
		'0','0', '0', '0', '0', '0', '0', '0'
	};
    // @formatter:on
    static const char* digits = "0123456789abcdef";
    uint32 n = value.A;
    char* p = buffer + 7;
    do
    {
        *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    n = value.B;
    p = buffer + 15;
    do
    {
        *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    n = value.C;
    p = buffer + 23;
    do
    {
        *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    n = value.D;
    p = buffer + 31;
    do
    {
        *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    String(buffer, 32);
}

void JsonWriter::Object(ISerializable* value, const void* otherObj)
{
    StartObject();
    value->Serialize(*this, otherObj);
    EndObject();
}

void JsonWriter::SceneObject(::SceneObject* obj)
{
    StartObject();

    // Serialize prefab instances with diff detection
    if (obj->HasPrefabLink())
    {
        // Load prefab
        auto prefab = Content::Load<Prefab>(obj->GetPrefabID());
        if (prefab)
        {
            // Request the prefab to be deserialized to the default instance (used for comparison to generate a diff)
            prefab->GetDefaultInstance();

            // Get prefab object instance from the prefab
            const void* prefabObject;
            if (prefab->ObjectsCache.TryGet(obj->GetPrefabObjectID(), prefabObject))
            {
                // Serialize modified properties compared with the default object from prefab
                obj->Serialize(*this, prefabObject);
                EndObject();
                return;
            }
            else
            {
                LOG(Warning, "Missing object {1} in prefab {0}.", prefab->ToString(), obj->GetPrefabObjectID());
            }
        }
        else
        {
            if (prefab)
                LOG(Warning, "Failed to load prefab {0}.", prefab->ToString());
            else
                LOG(Warning, "Missing prefab with id={0}.", obj->GetPrefabID());
        }
    }

    // Serialize modified properties compared with the default class object
    const void* defaultInstance = obj->GetType().GetDefaultInstance();
    obj->Serialize(*this, defaultInstance);

    EndObject();
}
