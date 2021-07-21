// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "JsonTools.h"
#include "ISerializable.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Utilities/Encryption.h"

void ChangeIds(rapidjson_flax::Value& obj, rapidjson_flax::Document& document, const Dictionary<Guid, Guid>& mapping)
{
    if (obj.IsObject())
    {
        for (rapidjson_flax::Value::MemberIterator i = obj.MemberBegin(); i != obj.MemberEnd(); ++i)
        {
            ChangeIds(i->value, document, mapping);
        }
    }
    else if (obj.IsArray())
    {
        for (rapidjson::SizeType i = 0; i < obj.Size(); i++)
        {
            ChangeIds(obj[i], document, mapping);
        }
    }
    else if (obj.IsString() && obj.GetStringLength() == 32)
    {
        auto value = JsonTools::GetGuid(obj);
        if (mapping.TryGet(value, value))
        {
            // Unoptimized version:
            //obj.SetString(value.ToString(Guid::FormatType::N).ToSTD().c_str(), 32, document.GetAllocator());

            // Optimized version:
            char buffer[32] =
            {
            // @formatter:off
                '0','0','0','0','0','0','0','0','0','0',
                '0','0','0','0','0','0','0','0','0','0',
                '0','0','0','0','0','0','0','0','0','0',
                '0','0'
            // @formatter:on
            };
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
            obj.SetString(buffer, 32, document.GetAllocator());
        }
    }
}

void JsonTools::ChangeIds(Document& doc, const Dictionary<Guid, Guid>& mapping)
{
    if (mapping.IsEmpty())
        return;
    PROFILE_CPU();
    ::ChangeIds(doc, doc, mapping);
}

Matrix JsonTools::GetMatrix(const Value& value)
{
    Matrix result;
    GetFloat(result.M11, value, "M11");
    GetFloat(result.M12, value, "M12");
    GetFloat(result.M13, value, "M13");
    GetFloat(result.M14, value, "M14");
    GetFloat(result.M21, value, "M21");
    GetFloat(result.M22, value, "M22");
    GetFloat(result.M23, value, "M23");
    GetFloat(result.M24, value, "M24");
    GetFloat(result.M31, value, "M31");
    GetFloat(result.M32, value, "M32");
    GetFloat(result.M33, value, "M33");
    GetFloat(result.M34, value, "M34");
    GetFloat(result.M41, value, "M41");
    GetFloat(result.M42, value, "M42");
    GetFloat(result.M43, value, "M43");
    GetFloat(result.M44, value, "M44");
    return result;
}

DateTime JsonTools::GetDate(const Value& value)
{
    return DateTime(value.GetInt64());
}

DateTime JsonTools::GetDateTime(const Value& value)
{
    return DateTime(value.GetInt64());
}

CommonValue JsonTools::GetCommonValue(const Value& value)
{
    CommonValue result;
    const auto typeMember = value.FindMember("Type");
    const auto valueMember = value.FindMember("Value");
    if (typeMember != value.MemberEnd() && typeMember->value.IsInt() && valueMember != value.MemberEnd())
    {
        const auto& v = valueMember->value;
        switch ((CommonType)typeMember->value.GetInt())
        {
        case CommonType::Bool:
            result = v.GetBool();
            break;
        case CommonType::Integer:
            result = v.GetInt();
            break;
        case CommonType::Float:
            result = v.GetFloat();
            break;
        case CommonType::Vector2:
            result = GetVector2(v);
            break;
        case CommonType::Vector3:
            result = GetVector3(v);
            break;
        case CommonType::Vector4:
            result = GetVector4(v);
            break;
        case CommonType::Color:
            result = GetColor(v);
            break;
        case CommonType::Guid:
            result = GetGuid(v);
            break;
        case CommonType::String:
            result = v.GetText();
            break;
        case CommonType::Box:
            result = GetBoundingBox(v);
            break;
        case CommonType::Rotation:
            result = GetQuaternion(v);
            break;
        case CommonType::Transform:
            result = GetTransform(v);
            break;
        case CommonType::Sphere:
            result = GetBoundingSphere(v);
            break;
        case CommonType::Rectangle:
            result = GetRectangle(v);
            break;
        case CommonType::Ray:
            result = GetRay(v);
            break;
        case CommonType::Pointer:
            result = (void*)v.GetInt64();
            break;
        case CommonType::Matrix:
            result = GetMatrix(v);
            break;
        case CommonType::Blob:
        {
            const int32 length = v.GetStringLength();
            result.SetBlob(length);
            Encryption::Base64Decode(v.GetString(), length, result.AsBlob.Data);
            break;
        }
        case CommonType::Object:
            result = FindObject(GetGuid(v), ScriptingObject::GetStaticClass());
            break;
        default:
        CRASH;
            break;
        }
    }
    return result;
}
