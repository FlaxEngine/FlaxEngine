// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "JsonWriter.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Content/Content.h"
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
