// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Serialization.h"
#include "Engine/Core/Log.h"
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
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Scripting/Internal/ManagedSerialization.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Content/Asset.h"
#include "Engine/Level/SceneObject.h"
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
    case VariantType::Blob:
        stream.Blob(v.AsBlob.Data, v.AsBlob.Length);
        break;
    case VariantType::Object:
        stream.Guid(v.AsObject ? v.AsObject->GetID() : Guid::Empty);
        break;
    case VariantType::Asset:
        stream.Guid(v.AsAsset ? v.AsAsset->GetID() : Guid::Empty);
        break;
    case VariantType::Float2:
        stream.Float2(*(Float2*)v.AsData);
        break;
    case VariantType::Float3:
        stream.Float3(*(Float3*)v.AsData);
        break;
    case VariantType::Float4:
        stream.Float4(*(Float4*)v.AsData);
        break;
    case VariantType::Double2:
        stream.Double2(v.AsDouble2());
        break;
    case VariantType::Double3:
        stream.Double3(v.AsDouble3());
        break;
    case VariantType::Double4:
        stream.Double4(v.AsDouble4());
        break;
    case VariantType::Int2:
        stream.Int2(v.AsInt2());
        break;
    case VariantType::Int3:
        stream.Int3(v.AsInt3());
        break;
    case VariantType::Int4:
        stream.Int4(v.AsInt4());
        break;
    case VariantType::Color:
        stream.Color(v.AsColor());
        break;
    case VariantType::Guid:
        stream.Guid(v.AsGuid());
        break;
    case VariantType::BoundingSphere:
        stream.BoundingSphere(v.AsBoundingSphere());
        break;
    case VariantType::Quaternion:
        stream.Quaternion(v.AsQuaternion());
        break;
    case VariantType::Rectangle:
        stream.Rectangle(v.AsRectangle());
        break;
    case VariantType::BoundingBox:
        stream.BoundingBox(v.AsBoundingBox());
        break;
    case VariantType::Transform:
        stream.Transform(v.AsTransform());
        break;
    case VariantType::Ray:
        stream.Ray(v.AsRay());
        break;
    case VariantType::Matrix:
        stream.Matrix(v.AsMatrix());
        break;
    case VariantType::Array:
        Serialize(stream, v.AsArray(), nullptr);
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
    case VariantType::ManagedObject:
    case VariantType::Structure:
    {
#if USE_CSHARP
        MObject* obj;
        if (v.Type.Type == VariantType::Structure)
            obj = MUtils::BoxVariant(v);
        else
            obj = (MObject*)v;
        ManagedSerialization::Serialize(stream, obj);
#endif
        break;
    }
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
    v.SetType(MoveTemp(type));

    const auto mValue = SERIALIZE_FIND_MEMBER(stream, "Value");
    if (mValue == stream.MemberEnd())
        return;
    auto& value = mValue->value;
    Guid id;
    switch (v.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
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
    case VariantType::Blob:
        CHECK(value.IsString());
        id.A = value.GetStringLength();
        v.SetBlob(id.A);
        Encryption::Base64Decode(value.GetString(), id.A, (byte*)v.AsBlob.Data);
        break;
    case VariantType::Float2:
        Deserialize(value, *(Float2*)v.AsData, modifier);
        break;
    case VariantType::Float3:
        Deserialize(value, *(Float3*)v.AsData, modifier);
        break;
    case VariantType::Float4:
        Deserialize(value, *(Float4*)v.AsData, modifier);
        break;
    case VariantType::Double2:
        Deserialize(value, *(Double2*)v.AsData, modifier);
        break;
    case VariantType::Double3:
        Deserialize(value, *(Double3*)v.AsData, modifier);
        break;
    case VariantType::Double4:
        Deserialize(value, *(Double4*)v.AsBlob.Data, modifier);
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
        Deserialize(value, v.AsBoundingSphere(), modifier);
        break;
    case VariantType::Quaternion:
        Deserialize(value, *(Quaternion*)v.AsData, modifier);
        break;
    case VariantType::Rectangle:
        Deserialize(value, *(Rectangle*)v.AsData, modifier);
        break;
    case VariantType::BoundingBox:
        Deserialize(value, v.AsBoundingBox(), modifier);
        break;
    case VariantType::Transform:
        Deserialize(value, v.AsTransform(), modifier);
        break;
    case VariantType::Ray:
        Deserialize(value, v.AsRay(), modifier);
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
    case VariantType::ManagedObject:
    case VariantType::Structure:
    {
#if USE_CSHARP
        auto obj = (MObject*)v;
        if (!obj && v.Type.TypeName)
        {
            MClass* klass = MUtils::GetClass(v.Type);
            if (!klass)
            {
                LOG(Error, "Invalid variant type {0}", v.Type);
                return;
            }
            obj = MCore::Object::New(klass);
            if (!obj)
            {
                LOG(Error, "Failed to managed instance of the variant type {0}", v.Type);
                return;
            }
            if (!klass->IsValueType())
                MCore::Object::Init(obj);
            if (v.Type.Type == VariantType::ManagedObject)
                v.SetManagedObject(obj);
        }
        ManagedSerialization::Deserialize(value, obj);
        if (v.Type.Type == VariantType::Structure)
            v = MUtils::UnboxVariant(obj);
#endif
        break;
    }
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
                    const auto mRevision = SERIALIZE_FIND_MEMBER(stream, "Revision");
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

bool Serialization::ShouldSerialize(const Float2& v, const void* otherObj)
{
    return !otherObj || !Float2::NearEqual(v, *(Float2*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Float2& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Float3& v, const void* otherObj)
{
    return !otherObj || !Float3::NearEqual(v, *(Float3*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Float3& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Float4& v, const void* otherObj)
{
    return !otherObj || !Float4::NearEqual(v, *(Float4*)otherObj, SERIALIZE_EPSILON);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Float4& v, ISerializeModifier* modifier)
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

bool Serialization::ShouldSerialize(const Double2& v, const void* otherObj)
{
    return !otherObj || !Double2::NearEqual(v, *(Double2*)otherObj, SERIALIZE_EPSILON_DOUBLE);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Double2& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Double3& v, const void* otherObj)
{
    return !otherObj || !Double3::NearEqual(v, *(Double3*)otherObj, SERIALIZE_EPSILON_DOUBLE);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Double3& v, ISerializeModifier* modifier)
{
    const auto mX = SERIALIZE_FIND_MEMBER(stream, "X");
    const auto mY = SERIALIZE_FIND_MEMBER(stream, "Y");
    const auto mZ = SERIALIZE_FIND_MEMBER(stream, "Z");
    v.X = mX != stream.MemberEnd() ? mX->value.GetFloat() : 0.0f;
    v.Y = mY != stream.MemberEnd() ? mY->value.GetFloat() : 0.0f;
    v.Z = mZ != stream.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
}

bool Serialization::ShouldSerialize(const Double4& v, const void* otherObj)
{
    return !otherObj || !Double4::NearEqual(v, *(Double4*)otherObj, SERIALIZE_EPSILON_DOUBLE);
}

void Serialization::Deserialize(ISerializable::DeserializeStream& stream, Double4& v, ISerializeModifier* modifier)
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
    {
        const auto m = SERIALIZE_FIND_MEMBER(stream, "Translation");
        if (m != stream.MemberEnd())
            Deserialize(m->value, v.Translation, modifier);
    }
    {
        const auto m = SERIALIZE_FIND_MEMBER(stream, "Scale");
        if (m != stream.MemberEnd())
            Deserialize(m->value, v.Scale, modifier);
    }
    {
        const auto m = SERIALIZE_FIND_MEMBER(stream, "Orientation");
        if (m != stream.MemberEnd())
            Deserialize(m->value, v.Orientation, modifier);
    }
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

bool Serialization::ShouldSerialize(const SceneObject* v, const SceneObject* other)
{
    bool result = v != other;
    if (result && v && other && v->HasPrefabLink() && other->HasPrefabLink())
    {
        // Special case when saving reference to prefab object and the objects are different but the point to the same prefab object
        // In that case, skip saving reference as it's defined in prefab (will be populated via IdsMapping during deserialization)
        result &= v->GetPrefabObjectID() != other->GetPrefabObjectID();
    }
    return result;
}

#undef DESERIALIZE_HELPER
