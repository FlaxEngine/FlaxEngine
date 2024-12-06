// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ShaderGraphValue.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Types/StringView.h"

const Char* ShaderGraphValue::_subs[] =
{
    TEXT(".x"),
    TEXT(".y"),
    TEXT(".z"),
    TEXT(".w")
};

const ShaderGraphValue ShaderGraphValue::Zero(VariantType::Types::Float, TEXT("0.0"));
const ShaderGraphValue ShaderGraphValue::Half(VariantType::Types::Float, TEXT("0.5"));
const ShaderGraphValue ShaderGraphValue::One(VariantType::Types::Float, TEXT("1.0"));
const ShaderGraphValue ShaderGraphValue::True(VariantType::Types::Bool, TEXT("true"));
const ShaderGraphValue ShaderGraphValue::False(VariantType::Types::Bool, TEXT("false"));

ShaderGraphValue::ShaderGraphValue(const Variant& v)
{
    switch (v.Type.Type)
    {
    case VariantType::Bool:
        Type = VariantType::Types::Bool;
        Value = v.AsBool ? TEXT('1') : TEXT('0');
        break;
    case VariantType::Int:
        Type = VariantType::Types::Int;
        Value = StringUtils::ToString(v.AsInt);
        break;
    case VariantType::Uint:
        Type = VariantType::Types::Uint;
        Value = StringUtils::ToString(v.AsUint);
        break;
    case VariantType::Float:
        Type = VariantType::Types::Float;
        Value = String::Format(TEXT("{}"), v.AsFloat);
        if (Value.Find('.') == -1)
            Value = String::Format(TEXT("{:.1f}"), v.AsFloat);
        break;
    case VariantType::Double:
        Type = VariantType::Types::Float;
        Value = String::Format(TEXT("{}"), (float)v.AsDouble);
        if (Value.Find('.') == -1)
            Value = String::Format(TEXT("{:.1f}"), (float)v.AsDouble);
        break;
    case VariantType::Float2:
    {
        const auto vv = v.AsFloat2();
        Type = VariantType::Types::Float2;
        Value = String::Format(TEXT("float2({0}, {1})"), vv.X, vv.Y);
        break;
    }
    case VariantType::Float3:
    {
        const auto vv = v.AsFloat3();
        Type = VariantType::Types::Float3;
        Value = String::Format(TEXT("float3({0}, {1}, {2})"), vv.X, vv.Y, vv.Z);
        break;
    }
    case VariantType::Float4:
    case VariantType::Color:
    {
        const auto vv = v.AsFloat4();
        Type = VariantType::Types::Float4;
        Value = String::Format(TEXT("float4({0}, {1}, {2}, {3})"), vv.X, vv.Y, vv.Z, vv.W);
        break;
    }
    case VariantType::Double2:
    {
        const auto vv = (::Float2)v.AsDouble2();
        Type = VariantType::Types::Float2;
        Value = String::Format(TEXT("float2({0}, {1})"), vv.X, vv.Y);
        break;
    }
    case VariantType::Double3:
    {
        const auto vv = (::Float3)v.AsDouble3();
        Type = VariantType::Types::Float3;
        Value = String::Format(TEXT("float3({0}, {1}, {2})"), vv.X, vv.Y, vv.Z);
        break;
    }
    case VariantType::Double4:
    {
        const auto vv = (::Float4)v.AsDouble4();
        Type = VariantType::Types::Float4;
        Value = String::Format(TEXT("float4({0}, {1}, {2}, {3})"), vv.X, vv.Y, vv.Z, vv.W);
        break;
    }
    case VariantType::Quaternion:
    {
        const auto vv = v.AsQuaternion();
        Type = VariantType::Types::Quaternion;
        Value = String::Format(TEXT("float4({0}, {1}, {2}, {3})"), vv.X, vv.Y, vv.Z, vv.W);
        break;
    }
    case VariantType::String:
        Type = VariantType::Types::String;
        Value = (StringView)v;
        break;
    default:
        Type = VariantType::Types::Null;
        break;
    }
}

bool ShaderGraphValue::IsZero() const
{
    switch (Type)
    {
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
    case VariantType::Types::Float:
        return Value == TEXT("0") || Value == TEXT("0.0");
    default:
        return false;
    }
}

bool ShaderGraphValue::IsOne() const
{
    switch (Type)
    {
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
    case VariantType::Types::Float:
        return Value == TEXT("1") || Value == TEXT("1.0");
    default:
        return false;
    }
}

bool ShaderGraphValue::IsLiteral() const
{
    switch (Type)
    {
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
    case VariantType::Types::Float:
        if (Value.HasChars())
        {
            for (int32 i = 0; i < Value.Length(); i++)
            {
                const Char c = Value[i];
                if (!StringUtils::IsDigit(c) && c != '.')
                    return false;
            }
            return true;
        }
    default:
        return false;
    }
}

ShaderGraphValue ShaderGraphValue::InitForZero(VariantType::Types type)
{
    const Char* v;
    switch (type)
    {
    case VariantType::Types::Float:
    case VariantType::Types::Double:
        v = TEXT("0.0");
        break;
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
        v = TEXT("0");
        break;
    case VariantType::Types::Float2:
    case VariantType::Types::Double2:
        v = TEXT("float2(0, 0)");
        break;
    case VariantType::Types::Float3:
    case VariantType::Types::Double3:
        v = TEXT("float3(0, 0, 0)");
        break;
    case VariantType::Types::Float4:
    case VariantType::Types::Double4:
    case VariantType::Types::Color:
        v = TEXT("float4(0, 0, 0, 0)");
        break;
    case VariantType::Types::Quaternion:
        v = TEXT("float4(0, 0, 0, 1)");
        break;
    case VariantType::Types::Void:
        v = TEXT("((Material)0)");
        break;
    default:
        CRASH;
        v = nullptr;
    }
    return ShaderGraphValue(type, v);
}

ShaderGraphValue ShaderGraphValue::InitForHalf(VariantType::Types type)
{
    const Char* v;
    switch (type)
    {
    case VariantType::Types::Float:
    case VariantType::Types::Double:
        v = TEXT("0.5");
        break;
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
        v = TEXT("0");
        break;
    case VariantType::Types::Float2:
    case VariantType::Types::Double2:
        v = TEXT("float2(0.5, 0.5)");
        break;
    case VariantType::Types::Float3:
    case VariantType::Types::Double3:
        v = TEXT("float3(0.5, 0.5, 0.5)");
        break;
    case VariantType::Types::Float4:
    case VariantType::Types::Double4:
    case VariantType::Types::Quaternion:
    case VariantType::Types::Color:
        v = TEXT("float4(0.5, 0.5, 0.5, 0.5)");
        break;
    default:
        CRASH;
        v = nullptr;
    }
    return ShaderGraphValue(type, String(v));
}

ShaderGraphValue ShaderGraphValue::InitForOne(VariantType::Types type)
{
    const Char* v;
    switch (type)
    {
    case VariantType::Types::Float:
    case VariantType::Types::Double:
        v = TEXT("1.0");
        break;
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
        v = TEXT("1");
        break;
    case VariantType::Types::Float2:
    case VariantType::Types::Double2:
        v = TEXT("float2(1, 1)");
        break;
    case VariantType::Types::Float3:
    case VariantType::Types::Double3:
        v = TEXT("float3(1, 1, 1)");
        break;
    case VariantType::Types::Float4:
    case VariantType::Types::Double4:
    case VariantType::Types::Quaternion:
    case VariantType::Types::Color:
        v = TEXT("float4(1, 1, 1, 1)");
        break;
    default:
        CRASH;
        v = nullptr;
    }
    return ShaderGraphValue(type, String(v));
}

ShaderGraphValue ShaderGraphValue::GetY() const
{
    switch (Type)
    {
    case VariantType::Float2:
    case VariantType::Float3:
    case VariantType::Float4:
    case VariantType::Double2:
    case VariantType::Double3:
    case VariantType::Double4:
        return ShaderGraphValue(VariantType::Types::Float, Value + _subs[1]);
    default:
        return Zero;
    }
}

ShaderGraphValue ShaderGraphValue::GetZ() const
{
    switch (Type)
    {
    case VariantType::Float3:
    case VariantType::Float4:
    case VariantType::Double3:
    case VariantType::Double4:
        return ShaderGraphValue(VariantType::Types::Float, Value + _subs[2]);
    default:
        return Zero;
    }
}

ShaderGraphValue ShaderGraphValue::GetW() const
{
    switch (Type)
    {
    case VariantType::Float4:
    case VariantType::Double4:
        return ShaderGraphValue(VariantType::Types::Float, Value + _subs[3]);
    default:
        return One;
    }
}

ShaderGraphValue ShaderGraphValue::Cast(const ShaderGraphValue& v, VariantType::Types to)
{
    // If they are the same types or input value is empty, then just return value
    if (v.Type == to || v.Value.IsEmpty())
    {
        return v;
    }

    // Select format string
    const Char* format = nullptr;
    switch (to)
    {
    case VariantType::Types::Bool:
        switch (v.Type)
        {
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
        case VariantType::Types::Double:
            format = TEXT("((bool){0})");
            break;
        case VariantType::Types::Float2:
        case VariantType::Types::Float3:
        case VariantType::Types::Float4:
        case VariantType::Types::Double2:
        case VariantType::Types::Double3:
        case VariantType::Types::Double4:
        case VariantType::Types::Quaternion:
        case VariantType::Types::Color:
            format = TEXT("((bool){0}.x)");
            break;
        }
        break;
    case VariantType::Types::Int:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
        case VariantType::Types::Double:
            format = TEXT("((int){0})");
            break;
        case VariantType::Types::Float2:
        case VariantType::Types::Float3:
        case VariantType::Types::Float4:
        case VariantType::Types::Double2:
        case VariantType::Types::Double3:
        case VariantType::Types::Double4:
        case VariantType::Types::Quaternion:
        case VariantType::Types::Color:
            format = TEXT("((int){0}.x)");
            break;
        }
        break;
    case VariantType::Types::Uint:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Float:
        case VariantType::Types::Double:
            format = TEXT("((uint){0})");
            break;
        case VariantType::Types::Float2:
        case VariantType::Types::Float3:
        case VariantType::Types::Float4:
        case VariantType::Types::Double2:
        case VariantType::Types::Double3:
        case VariantType::Types::Double4:
        case VariantType::Types::Quaternion:
        case VariantType::Types::Color:
            format = TEXT("((uint){0}.x)");
            break;
        }
        break;
    case VariantType::Types::Float:
    case VariantType::Types::Double:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
            format = TEXT("((float){0})");
            break;
        case VariantType::Types::Float2:
        case VariantType::Types::Float3:
        case VariantType::Types::Float4:
        case VariantType::Types::Double2:
        case VariantType::Types::Double3:
        case VariantType::Types::Double4:
        case VariantType::Types::Quaternion:
        case VariantType::Types::Color:
            format = TEXT("((float){0}.x)");
            break;
        }
        break;
    case VariantType::Types::Float2:
    case VariantType::Types::Double2:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
        case VariantType::Types::Double:
            format = TEXT("float2({0}, {0})");
            break;
        case VariantType::Types::Float3:
        case VariantType::Types::Float4:
        case VariantType::Types::Double2:
        case VariantType::Types::Double3:
        case VariantType::Types::Double4:
        case VariantType::Types::Quaternion:
        case VariantType::Types::Color:
            format = TEXT("{0}.xy");
            break;
        }
        break;
    case VariantType::Types::Float3:
    case VariantType::Types::Double3:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
        case VariantType::Types::Double:
            format = TEXT("float3({0}, {0}, {0})");
            break;
        case VariantType::Types::Float2:
        case VariantType::Types::Double2:
            format = TEXT("float3({0}.xy, 0)");
            break;
        case VariantType::Types::Double3:
        case VariantType::Types::Float4:
        case VariantType::Types::Double4:
        case VariantType::Types::Color:
            format = TEXT("{0}.xyz");
            break;
        case VariantType::Types::Quaternion:
            format = TEXT("QuatRotateVector({0}, float3(0, 0, 1))"); // Returns direction vector
            break;
        }
        break;
    case VariantType::Types::Float4:
    case VariantType::Types::Double4:
    case VariantType::Types::Color:
    case VariantType::Types::Quaternion:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
        case VariantType::Types::Double:
            format = TEXT("float4({0}, {0}, {0}, {0})");
            break;
        case VariantType::Types::Float2:
        case VariantType::Types::Double2:
            format = TEXT("float4({0}.xy, 0, 0)");
            break;
        case VariantType::Types::Float3:
        case VariantType::Types::Double3:
            format = TEXT("float4({0}.xyz, 0)");
            break;
        case VariantType::Types::Color:
        case VariantType::Types::Float4:
        case VariantType::Types::Double4:
        case VariantType::Types::Quaternion:
            format = TEXT("{}");
            break;
        }
        break;
    }
    if (format == nullptr)
    {
        LOG(Error, "Failed to cast shader graph value of type {0} to {1}", VariantType(v.Type), VariantType(to));
        return Zero;
    }

    return ShaderGraphValue(to, String::Format(format, v.Value));
}
