// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ShaderGraphValue.h"
#include "Engine/Core/Log.h"
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
        Value = String::Format(TEXT("{0}"), v.AsFloat);
        break;
    case VariantType::Vector2:
        Type = VariantType::Types::Vector2;
        Value = String::Format(TEXT("float2({0}, {1})"), (*(Vector2*)v.AsData).X, (*(Vector2*)v.AsData).Y);
        break;
    case VariantType::Vector3:
        Type = VariantType::Types::Vector3;
        Value = String::Format(TEXT("float3({0}, {1}, {2})"), (*(Vector3*)v.AsData).X, (*(Vector3*)v.AsData).Y, (*(Vector3*)v.AsData).Z);
        break;
    case VariantType::Vector4:
    case VariantType::Color:
        Type = VariantType::Types::Vector4;
        Value = String::Format(TEXT("float4({0}, {1}, {2}, {3})"), (*(Vector4*)v.AsData).X, (*(Vector4*)v.AsData).Y, (*(Vector4*)v.AsData).Z, (*(Vector4*)v.AsData).W);
        break;
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

ShaderGraphValue ShaderGraphValue::InitForZero(VariantType::Types type)
{
    const Char* v;
    switch (type)
    {
    case VariantType::Types::Float:
        v = TEXT("0.0");
        break;
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
        v = TEXT("0");
        break;
    case VariantType::Types::Vector2:
        v = TEXT("float2(0, 0)");
        break;
    case VariantType::Types::Vector3:
        v = TEXT("float3(0, 0, 0)");
        break;
    case VariantType::Types::Vector4:
    case VariantType::Types::Color:
        v = TEXT("float4(0, 0, 0, 0)");
        break;
    case VariantType::Types::Void:
        v = TEXT("((Material)0)");
        break;
    default:
    CRASH;
        v = nullptr;
    }
    return ShaderGraphValue(type, String(v));
}

ShaderGraphValue ShaderGraphValue::InitForHalf(VariantType::Types type)
{
    const Char* v;
    switch (type)
    {
    case VariantType::Types::Float:
        v = TEXT("0.5");
        break;
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
        v = TEXT("0");
        break;
    case VariantType::Types::Vector2:
        v = TEXT("float2(0.5, 0.5)");
        break;
    case VariantType::Types::Vector3:
        v = TEXT("float3(0.5, 0.5, 0.5)");
        break;
    case VariantType::Types::Vector4:
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
        v = TEXT("1.0");
        break;
    case VariantType::Types::Bool:
    case VariantType::Types::Int:
    case VariantType::Types::Uint:
        v = TEXT("1");
        break;
    case VariantType::Types::Vector2:
        v = TEXT("float2(1, 1)");
        break;
    case VariantType::Types::Vector3:
        v = TEXT("float3(1, 1, 1)");
        break;
    case VariantType::Types::Vector4:
    case VariantType::Types::Color:
        v = TEXT("float4(1, 1, 1, 1)");
        break;
    default:
    CRASH;
        v = nullptr;
    }
    return ShaderGraphValue(type, String(v));
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
            format = TEXT("((bool){0})");
            break;
        case VariantType::Types::Vector2:
        case VariantType::Types::Vector3:
        case VariantType::Types::Vector4:
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
            format = TEXT("((int){0})");
            break;
        case VariantType::Types::Vector2:
        case VariantType::Types::Vector3:
        case VariantType::Types::Vector4:
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
            format = TEXT("((uint){0})");
            break;
        case VariantType::Types::Vector2:
        case VariantType::Types::Vector3:
        case VariantType::Types::Vector4:
        case VariantType::Types::Color:
            format = TEXT("((uint){0}.x)");
            break;
        }
        break;
    case VariantType::Types::Float:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
            format = TEXT("((float){0})");
            break;
        case VariantType::Types::Vector2:
        case VariantType::Types::Vector3:
        case VariantType::Types::Vector4:
        case VariantType::Types::Color:
            format = TEXT("((float){0}.x)");
            break;
        }
        break;
    case VariantType::Types::Vector2:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
            format = TEXT("float2({0}, {0})");
            break;
        case VariantType::Types::Vector3:
        case VariantType::Types::Vector4:
        case VariantType::Types::Color:
            format = TEXT("{0}.xy");
            break;
        }
        break;
    case VariantType::Types::Vector3:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
            format = TEXT("float3({0}, {0}, {0})");
            break;
        case VariantType::Types::Vector2:
            format = TEXT("float3({0}.xy, 0)");
            break;
        case VariantType::Types::Vector4:
        case VariantType::Types::Color:
            format = TEXT("{0}.xyz");
            break;
        }
        break;
    case VariantType::Types::Vector4:
    case VariantType::Types::Color:
        switch (v.Type)
        {
        case VariantType::Types::Bool:
        case VariantType::Types::Int:
        case VariantType::Types::Uint:
        case VariantType::Types::Float:
            format = TEXT("float4({0}, {0}, {0}, {0})");
            break;
        case VariantType::Types::Vector2:
            format = TEXT("float4({0}.xy, 0, 0)");
            break;
        case VariantType::Types::Vector3:
            format = TEXT("float4({0}.xyz, 0)");
            break;
        case VariantType::Types::Color:
        case VariantType::Types::Vector4:
            format = TEXT("{0}");
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
