// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Serialization.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Int2.h"
#include "Engine/Core/Math/Int3.h"
#include "Engine/Core/Math/Int4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Utilities/Encryption.h"

void ISerializable::DeserializeIfExists(DeserializeStream& stream, const char* memberName, ISerializeModifier* modifier)
{
    auto member = stream.FindMember(memberName);
    if (member != stream.MemberEnd())
        Deserialize(member->value, modifier);
}

#define DESERIALIZE_HELPER(stream, name, var, defaultValue) \
    { \
        const auto m = SERIALIZE_FIND_MEMBER(stream, name); \
        if (m != stream.MemberEnd()) \
            Deserialize(m->value, var, modifier); \
        else \
            var = defaultValue;\
    }

bool Serialization::ShouldSerialize(const VariantType& v, const void* otherObj)
{
    return !otherObj || v != *(VariantType*)otherObj;
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const VariantType& v, const void* otherObj)
{
    if (v.TypeName == nullptr)
    {
        stream.Int(static_cast<int32>(v.Type));
    }
    else
    {
        stream.StartObject();

        stream.JKEY("Type");
        stream.Int(static_cast<int32>(v.Type));

        stream.JKEY("TypeName");
        stream.String(v.TypeName);

        stream.EndObject();
    }
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, VariantType& v, ISerializeModifier* modifier)
{
    if (stream.IsObject())
    {
        const auto mType = SERIALIZE_FIND_MEMBER(stream, "Type");
        if (mType != stream.MemberEnd())
            v.Type = (VariantType::Types)mType->value.GetInt();
        else
            v.Type = VariantType::Null;
        const auto mTypeName = SERIALIZE_FIND_MEMBER(stream, "TypeName");
        if (mTypeName != stream.MemberEnd() && mTypeName->value.IsString())
            v.SetTypeName(StringAnsiView(mTypeName->value.GetStringAnsiView()));
    }
    else
    {
        v.Type = (VariantType::Types)stream.GetInt();
    }
}

bool Serialization::ShouldSerialize(const Variant& v, const void* otherObj)
{
    return !otherObj || v != *(Variant*)otherObj;
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const Variant& v, const void* otherObj)
{
    stream.StartObject();

    stream.JKEY("Type");
    Serialize(stream, v.Type, nullptr);

    stream.JKEY("Value");
    switch (v.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::ManagedObject:
        stream.StartObject();
        stream.EndObject();
        break;
    case VariantType::Bool:
        stream.Bool(v.AsBool);
        break;
    case VariantType::Int:
        stream.Int(v.AsInt);
        break;
    case VariantType::Uint:
        stream.Uint(v.AsUint);
        break;
    case VariantType::Int64:
        stream.Int64(v.AsInt64);
        break;
    case VariantType::Uint64:
    case VariantType::Enum:
        stream.Uint64(v.AsUint64);
        break;
    case VariantType::Float:
        stream.Float(v.AsFloat);
        break;
    case VariantType::Double:
        stream.Double(v.AsDouble);
        break;
    case VariantType::Pointer:
        stream.Uint64((uint64)(uintptr)v.AsPointer);
        break;
    case VariantType::String:
        if (v.AsBlob.Data)
            stream.String((const Char*)v.AsBlob.Data, v.AsBlob.Length / sizeof(Char) - 1);
        else
            stream.String("", 0);
        break;
    case VariantType::Structure:
    case VariantType::Blob:
        stream.Blob(v.AsBlob.Data, v.AsBlob.Length);
        break;
    case VariantType::Object:
        stream.Guid(v.AsObject ? v.AsObject->GetID() : Guid::Empty);
        break;
    case VariantType::Asset:
        stream.Guid(v.AsAsset ? v.AsAsset->GetID() : Guid::Empty);
        break;
    case VariantType::Vector2:
        stream.Vector2(*(Vector2*)v.AsData);
        break;
    case VariantType::Vector3:
        stream.Vector3(*(Vector3*)v.AsData);
        break;
    case VariantType::Vector4:
        stream.Vector4(*(Vector4*)v.AsData);
        break;
    case VariantType::Int2:
        stream.Int2(*(Int2*)v.AsData);
        break;
    case VariantType::Int3:
        stream.Int3(*(Int3*)v.AsData);
        break;
    case VariantType::Int4:
        stream.Int4(*(Int4*)v.AsData);
        break;
    case VariantType::Color:
        stream.Color(*(Color*)v.AsData);
        break;
    case VariantType::Guid:
        stream.Guid(*(Guid*)v.AsData);
        break;
    case VariantType::BoundingSphere:
        stream.BoundingSphere(*(BoundingSphere*)v.AsData);
        break;
    case VariantType::Quaternion:
        stream.Quaternion(*(Quaternion*)v.AsData);
        break;
    case VariantType::Rectangle:
        stream.Rectangle(*(Rectangle*)v.AsData);
        break;
    case VariantType::BoundingBox:
        stream.BoundingBox(*(BoundingBox*)v.AsBlob.Data);
        break;
    case VariantType::Transform:
        stream.Transform(*(Transform*)v.AsBlob.Data);
        break;
    case VariantType::Ray:
        stream.Ray(*(Ray*)v.AsBlob.Data);
        break;
    case VariantType::Matrix:
        stream.Matrix(*(Matrix*)v.AsBlob.Data);
        break;
    case VariantType::Array:
        Serialize(stream, *(Array<Variant, HeapAllocation>*)v.AsData, nullptr);
        break;
    case VariantType::Dictionary:
        Serialize(stream, *v.AsDictionary, nullptr);
        break;
    case VariantType::Typename:
        if (v.AsBlob.Data)
            stream.String((const char*)v.AsBlob.Data, v.AsBlob.Length - 1);
        else
            stream.String("", 0);
        break;
    default:
        Platform::CheckFailed("", __FILE__, __LINE__);
        stream.StartObject();
        stream.EndObject();
    }

    stream.EndObject();
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Variant& v, ISerializeModifier* modifier)
{
    const auto mType = SERIALIZE_FIND_MEMBER(stream, "Type");
    if (mType == stream.MemberEnd())
        return;
    VariantType type;
    Deserialize(mType->value, type, modifier);
    v.SetType(type);

    const auto mValue = SERIALIZE_FIND_MEMBER(stream, "Value");
    if (mValue == stream.MemberEnd())
        return;
    auto& value = mValue->value;
    Guid id;
    switch (v.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::ManagedObject:
        break;
    case VariantType::Bool:
        v.AsBool = value.GetBool();
        break;
    case VariantType::Int:
        v.AsInt = value.GetInt();
        break;
    case VariantType::Uint:
        v.AsUint = value.GetUint();
        break;
    case VariantType::Int64:
        v.AsInt64 = value.GetInt64();
        break;
    case VariantType::Uint64:
    case VariantType::Enum:
        v.AsUint64 = value.GetUint64();
        break;
    case VariantType::Float:
        v.AsFloat = value.GetFloat();
        break;
    case VariantType::Double:
        v.AsDouble = value.GetDouble();
        break;
    case VariantType::Pointer:
        v.AsPointer = (void*)(uintptr)value.GetUint64();
        break;
    case VariantType::String:
        CHECK(value.IsString());
        v.SetString(value.GetStringAnsiView());
        break;
    case VariantType::Object:
        Deserialize(value, id, modifier);
        modifier->IdsMapping.TryGet(id, id);
        v.SetObject(FindObject(id, ScriptingObject::GetStaticClass()));
        break;
    case VariantType::Asset:
        Deserialize(value, id, modifier);
        v.SetAsset(LoadAsset(id, Asset::TypeInitializer));
        break;
    case VariantType::Structure:
    case VariantType::Blob:
        CHECK(value.IsString());
        id.A = value.GetStringLength();
        v.SetBlob(id.A);
        Encryption::Base64Decode(value.GetString(), id.A, (byte*)v.AsBlob.Data);
        break;
    case VariantType::Vector2:
        Deserialize(value, *(Vector2*)v.AsData, modifier);
        break;
    case VariantType::Vector3:
        Deserialize(value, *(Vector3*)v.AsData, modifier);
        break;
    case VariantType::Vector4:
        Deserialize(value, *(Vector4*)v.AsData, modifier);
        break;
    case VariantType::Int2:
        Deserialize(value, *(Int2*)v.AsData, modifier);
        break;
    case VariantType::Int3:
        Deserialize(value, *(Int3*)v.AsData, modifier);
        break;
    case VariantType::Int4:
        Deserialize(value, *(Int4*)v.AsData, modifier);
        break;
    case VariantType::Color:
        Deserialize(value, *(Color*)v.AsData, modifier);
        break;
    case VariantType::Guid:
        Deserialize(value, *(Guid*)v.AsData, modifier);
        break;
    case VariantType::BoundingSphere:
        Deserialize(value, *(BoundingSphere*)v.AsData, modifier);
        break;
    case VariantType::Quaternion:
        Deserialize(value, *(Quaternion*)v.AsData, modifier);
        break;
    case VariantType::Rectangle:
        Deserialize(value, *(Rectangle*)v.AsData, modifier);
        break;
    case VariantType::BoundingBox:
        Deserialize(value, *(BoundingBox*)v.AsBlob.Data, modifier);
        break;
    case VariantType::Transform:
        Deserialize(value, *(Transform*)v.AsBlob.Data, modifier);
        break;
    case VariantType::Ray:
        Deserialize(value, *(Ray*)v.AsBlob.Data, modifier);
        break;
    case VariantType::Matrix:
        Deserialize(value, *(Matrix*)v.AsBlob.Data, modifier);
        break;
    case VariantType::Array:
        Deserialize(value, *(Array<Variant, HeapAllocation>*)v.AsData, modifier);
        break;
    case VariantType::Dictionary:
        Deserialize(value, *v.AsDictionary, modifier);
        break;
    case VariantType::Typename:
        CHECK(value.IsString());
        v.SetTypename(value.GetStringAnsiView());
        break;
    default:
        Platform::CheckFailed("", __FILE__, __LINE__);
    }
}

bool Serialization::ShouldSerialize(const Guid& v, const void* otherObj)
{
    return v.IsValid();
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const Guid& v, const void* otherObj)
{
    stream.Guid(v);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Guid& v, ISerializeModifier* modifier)
{
    if (!stream.IsString() || stream.GetStringLength() != 32)
    {
        v = Guid::Empty;
        return;
    }
    const char* a = stream.GetString();
    const char* b = a + 8;
    const char* c = b + 8;
    const char* d = c + 8;
    StringUtils::ParseHex(a, 8, &v.A);
    StringUtils::ParseHex(b, 8, &v.B);
    StringUtils::ParseHex(c, 8, &v.C);
    StringUtils::ParseHex(d, 8, &v.D);
}

bool Serialization::ShouldSerialize(const DateTime& v, const void* otherObj)
{
    return !otherObj || v != *(DateTime*)otherObj;
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const DateTime& v, const void* otherObj)
{
    stream.DateTime(v);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, DateTime& v, ISerializeModifier* modifier)
{
    v.Ticks = stream.GetInt64();
}

bool Serialization::ShouldSerialize(const TimeSpan& v, const void* otherObj)
{
    return !otherObj || v != *(TimeSpan*)otherObj;
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const TimeSpan& v, const void* otherObj)
{
    stream.Int64(v.Ticks);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, TimeSpan& v, ISerializeModifier* modifier)
{
    v.Ticks = stream.GetInt64();
}

bool Serialization::ShouldSerialize(const Version& v, const void* otherObj)
{
    return !otherObj || v != *(Version*)otherObj;
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const Version& v, const void* otherObj)
{
    stream.String(v.ToString());
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Version& v, ISerializeModifier* modifier)
{
    if (stream.IsString())
    {
        Version::Parse(stream.GetText(), &v);
    }
    else if (stream.IsObject())
    {
        const auto mMajor = SERIALIZE_FIND_MEMBER(stream, "Major");
        if (mMajor != stream.MemberEnd())
        {
            const auto mMinor = SERIALIZE_FIND_MEMBER(stream, "Minor");
            if (mMinor != stream.MemberEnd())
            {
                const auto mBuild = SERIALIZE_FIND_MEMBER(stream, "Build");
                if (mBuild != stream.MemberEnd())
                {
                    const auto mRevision = SERIALIZE_FIND_MEMBER(stream, "mRevision");
                    if (mRevision != stream.MemberEnd())
                        v = Version(mMajor->value.GetInt(), mMinor->value.GetInt(), mBuild->value.GetInt(), mRevision->value.GetInt());
                    else
                        v = Version(mMajor->value.GetInt(), mMinor->value.GetInt(), mBuild->value.GetInt());
                }
                else
                    v = Version(mMajor->value.GetInt(), mMinor->value.GetInt());
            }
            else
                v = Version(mMajor->value.GetInt(), 0);
        }
        else
            v = Version();
    }
}

bool Serialization::ShouldSerialize(const Vector2& v, const void* otherObj)
{
    return !otherObj || !Vector2::NearEqual(v, *(Vector2*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Vector2& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Vector3& v, const void* otherObj)
{
    return !otherObj || !Vector3::NearEqual(v, *(Vector3*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Vector3& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Vector4& v, const void* otherObj)
{
    return !otherObj || !Vector4::NearEqual(v, *(Vector4*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Vector4& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    const auto mW = SERIALIZE_FIND_MEMBER(stream, "W");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
    v.W = mW != stream.MemberEnd() ? mW->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Int2& v, const void* otherObj)
{
    return !otherObj || !(v == *(Int2*)otherObj);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Int2& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    v.X = mX != stream.MemberEnd() ? mX->value.GetInt() : 0;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetInt() : 0;
}

bool Serialization::ShouldSerialize(const Int3& v, const void* otherObj)
{
    return !otherObj || !(v == *(Int3*)otherObj);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Int3& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    v.X = mX != stream.MemberEnd() ? mX->value.GetInt() : 0;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetInt() : 0;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetInt() : 0;
}

bool Serialization::ShouldSerialize(const Int4& v, const void* otherObj)
{
    return !otherObj || !(v == *(Int4*)otherObj);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Int4& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    const auto mW = SERIALIZE_FIND_MEMBER(stream, "W");
    v.X = mX != stream.MemberEnd() ? mX->value.GetInt() : 0;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetInt() : 0;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetInt() : 0;
    v.W = mW != stream.MemberEnd() ? mW->value.GetInt() : 0;
}

bool Serialization::ShouldSerialize(const Quaternion& v, const void* otherObj)
{
    return !otherObj || !Quaternion::NearEqual(v, *(Quaternion*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Quaternion& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    const auto mW = SERIALIZE_FIND_MEMBER(stream, "W");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
    v.W = mW != stream.MemberEnd() ? mW->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Color& v, const void* otherObj)
{
    return !otherObj || !Color::NearEqual(v, *(Color*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Color& v, ISerializeModifier* modifier)
{
    const auto mR = SERIALIZE_FIND_MEMBER(stream, "R");
    const auto mG = SERIALIZE_FIND_MEMBER(stream, "G");
    const auto mB = SERIALIZE_FIND_MEMBER(stream, "B");
    const auto mA = SERIALIZE_FIND_MEMBER(stream, "A");
    v.R = mR != stream.MemberEnd() ? mR->value.GetFloat() : 0.0f;
    v.G = mG != stream.MemberEnd() ? mG->value.GetFloat() : 0.0f;
    v.B = mB != stream.MemberEnd() ? mB->value.GetFloat() : 0.0f;
    v.A = mA != stream.MemberEnd() ? mA->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Color32& v, const void* otherObj)
{
    return !otherObj || v != *(Color32*)otherObj;
}

void Serialization::Serialize(ISerializable::SerializeStream& stream, const Color32& v, const void* otherObj)
{
    stream.StartObject();
    stream.JKEY("R");
    stream.Int(v.R);
    stream.JKEY("G");
    stream.Int(v.G);
    stream.JKEY("B");
    stream.Int(v.B);
    stream.JKEY("A");
    stream.Int(v.A);
    stream.EndObject();
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Color32& v, ISerializeModifier* modifier)
{
    const auto mR = SERIALIZE_FIND_MEMBER(stream, "R");
    const auto mG = SERIALIZE_FIND_MEMBER(stream, "G");
    const auto mB = SERIALIZE_FIND_MEMBER(stream, "B");
    const auto mA = SERIALIZE_FIND_MEMBER(stream, "A");
    v.R = mR != stream.MemberEnd() ? mR->value.GetInt() : 0;
    v.G = mG != stream.MemberEnd() ? mG->value.GetInt() : 0;
    v.B = mB != stream.MemberEnd() ? mB->value.GetInt() : 0;
    v.A = mA != stream.MemberEnd() ? mA->value.GetInt() : 0;
}

bool Serialization::ShouldSerialize(const BoundingBox& v, const void* otherObj)
{
    return !otherObj || !BoundingBox::NearEqual(v, *(BoundingBox*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, BoundingBox& v, ISerializeModifier* modifier)
{
    DESERIALIZE_HELPER(stream, "Minimum", v.Minimum, Vector3::Zero);
    DESERIALIZE_HELPER(stream, "Maximum", v.Maximum, Vector3::Zero);
}

bool Serialization::ShouldSerialize(const BoundingSphere& v, const void* otherObj)
{
    return !otherObj || !BoundingSphere::NearEqual(v, *(BoundingSphere*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, BoundingSphere& v, ISerializeModifier* modifier)
{
    DESERIALIZE_HELPER(stream, "Center", v.Center, Vector3::Zero);
    DESERIALIZE_HELPER(stream, "Radius", v.Radius, 0.0f);
}

bool Serialization::ShouldSerialize(const Ray& v, const void* otherObj)
{
    return !otherObj || !Ray::NearEqual(v, *(Ray*)otherObj);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Ray& v, ISerializeModifier* modifier)
{
    DESERIALIZE_HELPER(stream, "Position", v.Position, Vector3::Zero);
    DESERIALIZE_HELPER(stream, "Direction", v.Direction, Vector3::Zero);
}

bool Serialization::ShouldSerialize(const Rectangle& v, const void* otherObj)
{
    return !otherObj || !Rectangle::NearEqual(v, *(Rectangle*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Rectangle& v, ISerializeModifier* modifier)
{
    DESERIALIZE_HELPER(stream, "Location", v.Location, Vector2::Zero);
    DESERIALIZE_HELPER(stream, "Size", v.Size, Vector2::Zero);
}

bool Serialization::ShouldSerialize(const Transform& v, const void* otherObj)
{
    return !otherObj || !Transform::NearEqual(v, *(Transform*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Transform& v, ISerializeModifier* modifier)
{
    DESERIALIZE_HELPER(stream, "Translation", v.Translation, Vector3::Zero);
    DESERIALIZE_HELPER(stream, "Scale", v.Scale, Vector3::One);
    DESERIALIZE_HELPER(stream, "Orientation", v.Orientation, Quaternion::Identity);
}

bool Serialization::ShouldSerialize(const Matrix& v, const void* otherObj)
{
    return !otherObj || v != *(Matrix*)otherObj;
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Matrix& v, ISerializeModifier* modifier)
{
    DESERIALIZE_HELPER(stream, "M11", v.M11, 0);
    DESERIALIZE_HELPER(stream, "M12", v.M12, 0);
    DESERIALIZE_HELPER(stream, "M13", v.M13, 0);
    DESERIALIZE_HELPER(stream, "M14", v.M14, 0);
    DESERIALIZE_HELPER(stream, "M21", v.M21, 0);
    DESERIALIZE_HELPER(stream, "M22", v.M22, 0);
    DESERIALIZE_HELPER(stream, "M23", v.M23, 0);
    DESERIALIZE_HELPER(stream, "M24", v.M24, 0);
    DESERIALIZE_HELPER(stream, "M31", v.M31, 0);
    DESERIALIZE_HELPER(stream, "M32", v.M32, 0);
    DESERIALIZE_HELPER(stream, "M33", v.M33, 0);
    DESERIALIZE_HELPER(stream, "M34", v.M34, 0);
    DESERIALIZE_HELPER(stream, "M41", v.M41, 0);
    DESERIALIZE_HELPER(stream, "M42", v.M42, 0);
    DESERIALIZE_HELPER(stream, "M43", v.M43, 0);
    DESERIALIZE_HELPER(stream, "M44", v.M44, 0);
}

#undef DESERIALIZE_HELPER
