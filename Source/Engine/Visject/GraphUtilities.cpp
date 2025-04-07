// Copyright (c) Wojciech Figat. All rights reserved.

#include "GraphUtilities.h"

// [Deprecated on 31.07.2020, expires on 31.07.2022]
enum class GraphParamType_Deprecated
{
    Bool = 0,
    Integer = 1,
    Float = 2,
    Vector2 = 3,
    Vector3 = 4,
    Vector4 = 5,
    Color = 6,
    Texture = 7,
    NormalMap = 8,
    String = 9,
    Box = 10,
    Rotation = 11,
    Transform = 12,
    Asset = 13,
    Actor = 14,
    Rectangle = 15,
    CubeTexture = 16,
    SceneTexture = 17,
    GPUTexture = 18,
    Matrix = 19,
    GPUTextureArray = 20,
    GPUTextureVolume = 21,
    GPUTextureCube = 22,
    ChannelMask = 23,
};

// [Deprecated on 31.07.2020, expires on 31.07.2022]
enum class GraphConnectionType_Deprecated : uint32
{
    Invalid = 0,
    Impulse = 1 << 0,
    Bool = 1 << 1,
    Integer = 1 << 2,
    Float = 1 << 3,
    Vector2 = 1 << 4,
    Vector3 = 1 << 5,
    Vector4 = 1 << 6,
    String = 1 << 7,
    Object = 1 << 8,
    Rotation = 1 << 9,
    Transform = 1 << 10,
    Box = 1 << 11,
    ImpulseSecondary = 1 << 12,
    UnsignedInteger = 1 << 13,
    Scalar = Bool | Integer | Float | UnsignedInteger,
    Vector = Vector2 | Vector3 | Vector4,
    Variable = Scalar | Vector | String | Object | Rotation | Transform | Box,
    All = Variable | Impulse,
};

// Required by deprecated graph data upgrade code
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Level/Actor.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
FLAXENGINE_API void ReadOldGraphParamValue_Deprecated(byte graphParamType, ReadStream* stream, GraphParameter* param)
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    CommonValue value;
    stream->ReadCommonValue(&value);
    switch ((GraphParamType_Deprecated)graphParamType)
    {
    case GraphParamType_Deprecated::Bool:
        param->Type = VariantType(VariantType::Bool);
        param->Value = value.GetBool();
        break;
    case GraphParamType_Deprecated::Integer:
        param->Type = VariantType(VariantType::Int);
        param->Value = value.GetInteger();
        break;
    case GraphParamType_Deprecated::Float:
        param->Type = VariantType(VariantType::Float);
        param->Value = value.GetFloat();
        break;
    case GraphParamType_Deprecated::Vector2:
        param->Type = VariantType(VariantType::Float2);
        param->Value = value.GetVector2();
        break;
    case GraphParamType_Deprecated::Vector3:
        param->Type = VariantType(VariantType::Float3);
        param->Value = value.GetVector3();
        break;
    case GraphParamType_Deprecated::Vector4:
        param->Type = VariantType(VariantType::Float4);
        param->Value = value.GetVector4();
        break;
    case GraphParamType_Deprecated::Color:
        param->Type = VariantType(VariantType::Color);
        param->Value = value.GetColor();
        break;
    case GraphParamType_Deprecated::Texture:
    case GraphParamType_Deprecated::NormalMap:
        ASSERT(value.Type == CommonType::Guid);
        param->Type = VariantType(VariantType::Asset, TEXT("FlaxEngine.Texture"));
        param->Value.SetAsset(LoadAsset(value.AsGuid, Texture::TypeInitializer));
        break;
    case GraphParamType_Deprecated::String:
        ASSERT(value.Type == CommonType::String);
        param->Type = VariantType(VariantType::String);
        param->Value.SetString(StringView(value.AsString));
        break;
    case GraphParamType_Deprecated::Box:
        ASSERT(value.Type == CommonType::Box);
        param->Type = VariantType(VariantType::BoundingBox);
        param->Value = Variant(value.AsBox);
        break;
    case GraphParamType_Deprecated::Rotation:
        ASSERT(value.Type == CommonType::Rotation);
        param->Type = VariantType(VariantType::Quaternion);
        param->Value = value.AsRotation;
        break;
    case GraphParamType_Deprecated::Transform:
        ASSERT(value.Type == CommonType::Transform);
        param->Type = VariantType(VariantType::Transform);
        param->Value = Variant(value.AsTransform);
        break;
    case GraphParamType_Deprecated::Asset:
        ASSERT(value.Type == CommonType::Guid);
        param->Type = VariantType(VariantType::Asset);
        param->Value.SetAsset(LoadAsset(value.AsGuid, Asset::TypeInitializer));
        break;
    case GraphParamType_Deprecated::Rectangle:
        ASSERT(value.Type == CommonType::Rectangle);
        param->Type = VariantType(VariantType::Rectangle);
        param->Value = value.AsRectangle;
        break;
    case GraphParamType_Deprecated::Matrix:
        ASSERT(value.Type == CommonType::Matrix);
        param->Type = VariantType(VariantType::Matrix);
        param->Value = Variant(value.AsMatrix);
        break;
    case GraphParamType_Deprecated::Actor:
        ASSERT(value.Type == CommonType::Guid);
        param->Type = VariantType(VariantType::Object, TEXT("FlaxEngine.Actor"));
        param->Value.SetObject(FindObject(value.AsGuid, Actor::GetStaticClass()));
        break;
    case GraphParamType_Deprecated::CubeTexture:
        ASSERT(value.Type == CommonType::Guid);
        param->Type = VariantType(VariantType::Asset, TEXT("FlaxEngine.CubeTexture"));
        param->Value.SetAsset(LoadAsset(value.AsGuid, CubeTexture::TypeInitializer));
        break;
    case GraphParamType_Deprecated::GPUTexture:
    case GraphParamType_Deprecated::GPUTextureArray:
    case GraphParamType_Deprecated::GPUTextureVolume:
    case GraphParamType_Deprecated::GPUTextureCube:
        param->Type = VariantType(VariantType::Object, TEXT("FlaxEngine.GPUTexture"));
        param->Value.SetObject(nullptr);
        break;
    case GraphParamType_Deprecated::SceneTexture:
        param->Type = VariantType(VariantType::Enum, TEXT("FlaxEngine.MaterialSceneTextures"));
        param->Value.AsUint64 = (uint64)value.AsInteger;
        break;
    case GraphParamType_Deprecated::ChannelMask:
        param->Type = VariantType(VariantType::Enum, TEXT("FlaxEngine.ChannelMask"));
        param->Value.AsUint64 = (uint64)value.AsInteger;
        break;
    default:
        CRASH;
    }
}

FLAXENGINE_API void ReadOldGraphNodeValue_Deprecated(ReadStream* stream, Variant& result)
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    CommonValue value;
    stream->ReadCommonValue(&value);
    result = Variant(value);
}

FLAXENGINE_API void ReadOldGraphBoxType_Deprecated(uint32 connectionType, VariantType& type)
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    switch ((GraphConnectionType_Deprecated)connectionType)
    {
    case GraphConnectionType_Deprecated::Invalid:
        type = VariantType(VariantType::Null);
        break;
    case GraphConnectionType_Deprecated::Impulse:
        type = VariantType(VariantType::Void);
        break;
    case GraphConnectionType_Deprecated::Bool:
        type = VariantType(VariantType::Bool);
        break;
    case GraphConnectionType_Deprecated::Integer:
        type = VariantType(VariantType::Int);
        break;
    case GraphConnectionType_Deprecated::Float:
        type = VariantType(VariantType::Float);
        break;
    case GraphConnectionType_Deprecated::Vector2:
        type = VariantType(VariantType::Float2);
        break;
    case GraphConnectionType_Deprecated::Vector3:
        type = VariantType(VariantType::Float3);
        break;
    case GraphConnectionType_Deprecated::Vector4:
        type = VariantType(VariantType::Float4);
        break;
    case GraphConnectionType_Deprecated::String:
        type = VariantType(VariantType::String);
        break;
    case GraphConnectionType_Deprecated::Object:
        type = VariantType(VariantType::Object);
        break;
    case GraphConnectionType_Deprecated::Rotation:
        type = VariantType(VariantType::Quaternion);
        break;
    case GraphConnectionType_Deprecated::Transform:
        type = VariantType(VariantType::Transform);
        break;
    case GraphConnectionType_Deprecated::Box:
        type = VariantType(VariantType::BoundingBox);
        break;
    case GraphConnectionType_Deprecated::ImpulseSecondary:
        type = VariantType(VariantType::Void);
        break;
    case GraphConnectionType_Deprecated::UnsignedInteger:
        type = VariantType(VariantType::Uint);
        break;
    case GraphConnectionType_Deprecated::Scalar:
        type = VariantType(VariantType::Null);
        break;
    case GraphConnectionType_Deprecated::Vector:
        type = VariantType(VariantType::Null);
        break;
    case GraphConnectionType_Deprecated::Variable:
        type = VariantType(VariantType::Null);
        break;
    case GraphConnectionType_Deprecated::All:
        type = VariantType(VariantType::Null);
        break;
    default:
        type = VariantType(VariantType::Null);
    }
}

FLAXENGINE_API StringView GetGraphFunctionTypeName_Deprecated(const Variant& v)
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    if (v.Type.Type == VariantType::String)
        return (StringView)v;
    if (v.Type.Type == VariantType::Int)
    {
        switch ((GraphConnectionType_Deprecated)v.AsInt)
        {
        case GraphConnectionType_Deprecated::Impulse:
        case GraphConnectionType_Deprecated::ImpulseSecondary:
            return TEXT("System.Void");
        case GraphConnectionType_Deprecated::Invalid:
        case GraphConnectionType_Deprecated::Variable:
        case GraphConnectionType_Deprecated::All:
            return StringView::Empty;
        case GraphConnectionType_Deprecated::Bool:
            return TEXT("System.Boolean");
        case GraphConnectionType_Deprecated::Integer:
            return TEXT("System.Int32");
        case GraphConnectionType_Deprecated::Float:
        case GraphConnectionType_Deprecated::Scalar:
            return TEXT("System.Single");
        case GraphConnectionType_Deprecated::Vector2:
            return TEXT("FlaxEngine.Float2");
        case GraphConnectionType_Deprecated::Vector3:
            return TEXT("FlaxEngine.Float3");
        case GraphConnectionType_Deprecated::Vector4:
        case GraphConnectionType_Deprecated::Vector:
            return TEXT("FlaxEngine.Float4");
        case GraphConnectionType_Deprecated::String:
            return TEXT("System.String");
        case GraphConnectionType_Deprecated::Object:
            return TEXT("FlaxEngine.Object");
        case GraphConnectionType_Deprecated::Rotation:
            return TEXT("System.Quaternion");
        case GraphConnectionType_Deprecated::Transform:
            return TEXT("System.Transform");
        case GraphConnectionType_Deprecated::Box:
            return TEXT("System.BoundingBox");
        case GraphConnectionType_Deprecated::UnsignedInteger:
            return TEXT("System.UInt32");
        default: ;
        }
    }
    return StringView::Empty;
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void GraphUtilities::ApplySomeMathHere(Variant& v, Variant& a, MathOp1 op)
{
    v.SetType(a.Type);
    switch (a.Type.Type)
    {
    case VariantType::Bool:
        v.AsBool = op(a.AsBool ? 1.0f : 0.0f) > ZeroTolerance;
        break;
    case VariantType::Int:
        v.AsInt = (int32)op((float)a.AsInt);
        break;
    case VariantType::Uint:
        v.AsUint = (uint32)op((float)a.AsUint);
        break;
    case VariantType::Float:
        v.AsFloat = op(a.AsFloat);
        break;
    case VariantType::Float2:
    {
        Float2& vv = *(Float2*)v.AsData;
        const Float2& aa = *(const Float2*)a.AsData;
        vv.X = op(aa.X);
        vv.Y = op(aa.Y);
        break;
    }
    case VariantType::Float3:
    {
        Float3& vv = *(Float3*)v.AsData;
        const Float3& aa = *(const Float3*)a.AsData;
        vv.X = op(aa.X);
        vv.Y = op(aa.Y);
        vv.Z = op(aa.Z);
        break;
    }
    case VariantType::Float4:
    case VariantType::Color:
    {
        Float4& vv = *(Float4*)v.AsData;
        const Float4& aa = *(const Float4*)a.AsData;
        vv.X = op(aa.X);
        vv.Y = op(aa.Y);
        vv.Z = op(aa.Z);
        vv.W = op(aa.W);
        break;
    }
    case VariantType::Double2:
    {
        Double2& vv = *(Double2*)v.AsData;
        const Double2& aa = *(const Double2*)a.AsData;
        vv.X = (double)op((float)aa.X);
        vv.Y = (double)op((float)aa.Y);
        break;
    }
    case VariantType::Double3:
    {
        Double3& vv = *(Double3*)v.AsData;
        const Double3& aa = *(const Double3*)a.AsData;
        vv.X = (double)op((float)aa.X);
        vv.Y = (double)op((float)aa.Y);
        vv.Z = (double)op((float)aa.Z);
        break;
    }
    case VariantType::Double4:
    {
        Double4& vv = *(Double4*)v.AsBlob.Data;
        const Double4& aa = *(const Double4*)a.AsBlob.Data;
        vv.X = (double)op((float)aa.X);
        vv.Y = (double)op((float)aa.Y);
        vv.Z = (double)op((float)aa.Z);
        vv.W = (double)op((float)aa.W);
        break;
    }
    case VariantType::Quaternion:
    {
        Quaternion& vv = *(Quaternion*)v.AsData;
        const Quaternion& aa = *(const Quaternion*)a.AsData;
        vv.X = op(aa.X);
        vv.Y = op(aa.Y);
        vv.Z = op(aa.Z);
        vv.W = op(aa.W);
        break;
    }
    case VariantType::Transform:
    {
        Transform& vv = *(Transform*)v.AsBlob.Data;
        const Transform& aa = *(const Transform*)a.AsBlob.Data;
        vv.Translation.X = op((float)aa.Translation.X);
        vv.Translation.Y = op((float)aa.Translation.Y);
        vv.Translation.Z = op((float)aa.Translation.Z);
        vv.Orientation.X = op(aa.Orientation.X);
        vv.Orientation.Y = op(aa.Orientation.Y);
        vv.Orientation.Z = op(aa.Orientation.Z);
        vv.Orientation.W = op(aa.Orientation.W);
        vv.Scale.X = op(aa.Scale.X);
        vv.Scale.Y = op(aa.Scale.Y);
        vv.Scale.Z = op(aa.Scale.Z);
        break;
    }
    default:
        v = a;
        break;
    }
}

void GraphUtilities::ApplySomeMathHere(Variant& v, Variant& a, Variant& b, MathOp2 op)
{
    v.SetType(a.Type);
    switch (a.Type.Type)
    {
    case VariantType::Bool:
        v.AsBool = op(a.AsBool ? 1.0f : 0.0f, b.AsBool ? 1.0f : 0.0f) > ZeroTolerance;
        break;
    case VariantType::Int:
        v.AsInt = (int32)op((float)a.AsInt, (float)b.AsInt);
        break;
    case VariantType::Uint:
        v.AsUint = (uint32)op((float)a.AsUint, (float)b.AsUint);
        break;
    case VariantType::Int64:
        v.AsUint = (int64)op((float)a.AsInt64, (float)b.AsInt64);
        break;
    case VariantType::Uint64:
        v.AsUint = (uint64)op((float)a.AsUint64, (float)b.AsUint64);
        break;
    case VariantType::Int16:
        v.AsUint = (int16)op((float)a.AsInt16, (float)b.AsInt16);
        break;
    case VariantType::Uint16:
        v.AsUint = (uint16)op((float)a.AsUint16, (float)b.AsUint16);
        break;
    case VariantType::Float:
        v.AsFloat = op(a.AsFloat, b.AsFloat);
        break;
    case VariantType::Double:
        v.AsDouble = op((float)a.AsDouble, (float)b.AsDouble);
        break;
    case VariantType::Float2:
    {
        Float2& vv = *(Float2*)v.AsData;
        const Float2& aa = *(const Float2*)a.AsData;
        const Float2& bb = *(const Float2*)b.AsData;
        vv.X = op(aa.X, bb.X);
        vv.Y = op(aa.Y, bb.Y);
        break;
    }
    case VariantType::Float3:
    {
        Float3& vv = *(Float3*)v.AsData;
        const Float3& aa = *(const Float3*)a.AsData;
        const Float3& bb = *(const Float3*)b.AsData;
        vv.X = op(aa.X, bb.X);
        vv.Y = op(aa.Y, bb.Y);
        vv.Z = op(aa.Z, bb.Z);
        break;
    }
    case VariantType::Float4:
    case VariantType::Color:
    {
        Float4& vv = *(Float4*)v.AsData;
        const Float4& aa = *(const Float4*)a.AsData;
        const Float4& bb = *(const Float4*)b.AsData;
        vv.X = op(aa.X, bb.X);
        vv.Y = op(aa.Y, bb.Y);
        vv.Z = op(aa.Z, bb.Z);
        vv.W = op(aa.W, bb.W);
        break;
    }
    case VariantType::Double2:
    {
        Double2& vv = *(Double2*)v.AsData;
        const Double2& aa = *(const Double2*)a.AsData;
        const Double2& bb = *(const Double2*)b.AsData;
        vv.X = (double)op((float)aa.X, (float)bb.X);
        vv.Y = (double)op((float)aa.Y, (float)bb.Y);
        break;
    }
    case VariantType::Double3:
    {
        Double3& vv = *(Double3*)v.AsData;
        const Double3& aa = *(const Double3*)a.AsData;
        const Double3& bb = *(const Double3*)b.AsData;
        vv.X = (double)op((float)aa.X, (float)bb.X);
        vv.Y = (double)op((float)aa.Y, (float)bb.Y);
        vv.Z = (double)op((float)aa.Z, (float)bb.Z);
        break;
    }
    case VariantType::Double4:
    {
        Double4& vv = *(Double4*)v.AsBlob.Data;
        const Double4& aa = *(const Double4*)a.AsBlob.Data;
        const Double4& bb = *(const Double4*)b.AsBlob.Data;
        vv.X = (double)op((float)aa.X, (float)bb.X);
        vv.Y = (double)op((float)aa.Y, (float)bb.Y);
        vv.Z = (double)op((float)aa.Z, (float)bb.Z);
        vv.W = (double)op((float)aa.W, (float)bb.W);
        break;
    }
    case VariantType::Quaternion:
    {
        Quaternion& vv = *(Quaternion*)v.AsData;
        const Quaternion& aa = *(const Quaternion*)a.AsData;
        const Quaternion& bb = *(const Quaternion*)b.AsData;
        vv.X = op(aa.X, bb.X);
        vv.Y = op(aa.Y, bb.Y);
        vv.Z = op(aa.Z, bb.Z);
        vv.W = op(aa.W, bb.W);
        break;
    }
    case VariantType::Transform:
    {
        Transform& vv = *(Transform*)v.AsBlob.Data;
        const Transform& aa = *(const Transform*)a.AsBlob.Data;
        const Transform& bb = *(const Transform*)b.AsBlob.Data;
        vv.Translation.X = op((float)aa.Translation.X, (float)bb.Translation.X);
        vv.Translation.Y = op((float)aa.Translation.Y, (float)bb.Translation.Y);
        vv.Translation.Z = op((float)aa.Translation.Z, (float)bb.Translation.Z);
        vv.Orientation.X = op(aa.Orientation.X, bb.Orientation.X);
        vv.Orientation.Y = op(aa.Orientation.Y, bb.Orientation.Y);
        vv.Orientation.Z = op(aa.Orientation.Z, bb.Orientation.Z);
        vv.Orientation.W = op(aa.Orientation.W, bb.Orientation.W);
        vv.Scale.X = op(aa.Scale.X, bb.Scale.X);
        vv.Scale.Y = op(aa.Scale.Y, bb.Scale.Y);
        vv.Scale.Z = op(aa.Scale.Z, bb.Scale.Z);
        break;
    }
    default:
        v = a;
        break;
    }
}

void GraphUtilities::ApplySomeMathHere(Variant& v, Variant& a, Variant& b, Variant& c, MathOp3 op)
{
    v.SetType(a.Type);
    switch (a.Type.Type)
    {
    case VariantType::Bool:
        v.AsBool = op(a.AsBool ? 1.0f : 0.0f, b.AsBool ? 1.0f : 0.0f, c.AsBool ? 1.0f : 0.0f) > ZeroTolerance;
        break;
    case VariantType::Int:
        v.AsInt = (int32)op((float)a.AsInt, (float)b.AsInt, (float)c.AsInt);
        break;
    case VariantType::Uint:
        v.AsUint = (int32)op((float)a.AsUint, (float)b.AsUint, (float)c.AsUint);
        break;
    case VariantType::Float:
        v.AsFloat = op(a.AsFloat, b.AsFloat, c.AsFloat);
        break;
    case VariantType::Float2:
    {
        Float2& vv = *(Float2*)v.AsData;
        const Float2& aa = *(const Float2*)a.AsData;
        const Float2& bb = *(const Float2*)b.AsData;
        const Float2& cc = *(const Float2*)b.AsData;
        vv.X = op(aa.X, bb.X, cc.X);
        vv.Y = op(aa.Y, bb.Y, cc.Y);
        break;
    }
    case VariantType::Float3:
    {
        Float3& vv = *(Float3*)v.AsData;
        const Float3& aa = *(const Float3*)a.AsData;
        const Float3& bb = *(const Float3*)b.AsData;
        const Float3& cc = *(const Float3*)b.AsData;
        vv.X = op(aa.X, bb.X, cc.X);
        vv.Y = op(aa.Y, bb.Y, cc.Y);
        vv.Z = op(aa.Z, bb.Z, cc.Z);
        break;
    }
    case VariantType::Float4:
    case VariantType::Color:
    {
        Float4& vv = *(Float4*)v.AsData;
        const Float4& aa = *(const Float4*)a.AsData;
        const Float4& bb = *(const Float4*)b.AsData;
        const Float4& cc = *(const Float4*)b.AsData;
        vv.X = op(aa.X, bb.X, cc.X);
        vv.Y = op(aa.Y, bb.Y, cc.Y);
        vv.Z = op(aa.Z, bb.Z, cc.Z);
        vv.W = op(aa.W, bb.W, cc.W);
        break;
    }
    case VariantType::Double2:
    {
        Double2& vv = *(Double2*)v.AsData;
        const Double2& aa = *(const Double2*)a.AsData;
        const Double2& bb = *(const Double2*)b.AsData;
        const Double2& cc = *(const Double2*)b.AsData;
        vv.X = (double)op((float)aa.X, (float)bb.X, (float)cc.X);
        vv.Y = (double)op((float)aa.Y, (float)bb.Y, (float)cc.Y);
        break;
    }
    case VariantType::Double3:
    {
        Double3& vv = *(Double3*)v.AsData;
        const Double3& aa = *(const Double3*)a.AsData;
        const Double3& bb = *(const Double3*)b.AsData;
        const Double3& cc = *(const Double3*)b.AsData;
        vv.X = (double)op((float)aa.X, (float)bb.X, (float)cc.X);
        vv.Y = (double)op((float)aa.Y, (float)bb.Y, (float)cc.Y);
        vv.Z = (double)op((float)aa.Z, (float)bb.Z, (float)cc.Z);
        break;
    }
    case VariantType::Double4:
    {
        Double4& vv = *(Double4*)v.AsBlob.Data;
        const Double4& aa = *(const Double4*)a.AsBlob.Data;
        const Double4& bb = *(const Double4*)b.AsBlob.Data;
        const Double4& cc = *(const Double4*)b.AsBlob.Data;
        vv.X = (double)op((float)aa.X, (float)bb.X, (float)cc.X);
        vv.Y = (double)op((float)aa.Y, (float)bb.Y, (float)cc.Y);
        vv.Z = (double)op((float)aa.Z, (float)bb.Z, (float)cc.Z);
        vv.W = (double)op((float)aa.W, (float)bb.W, (float)cc.W);
        break;
    }
    case VariantType::Quaternion:
    {
        Quaternion& vv = *(Quaternion*)v.AsData;
        const Quaternion& aa = *(const Quaternion*)a.AsData;
        const Quaternion& bb = *(const Quaternion*)b.AsData;
        const Quaternion& cc = *(const Quaternion*)b.AsData;
        vv.X = op(aa.X, bb.X, cc.X);
        vv.Y = op(aa.Y, bb.Y, cc.Y);
        vv.Z = op(aa.Z, bb.Z, cc.Z);
        vv.W = op(aa.W, bb.W, cc.W);
        break;
    }
    case VariantType::Transform:
    {
        Transform& vv = *(Transform*)v.AsBlob.Data;
        const Transform& aa = *(const Transform*)a.AsBlob.Data;
        const Transform& bb = *(const Transform*)b.AsBlob.Data;
        const Transform& cc = *(const Transform*)c.AsBlob.Data;
        vv.Translation.X = op((float)aa.Translation.X, (float)bb.Translation.X, (float)cc.Translation.X);
        vv.Translation.Y = op((float)aa.Translation.Y, (float)bb.Translation.Y, (float)cc.Translation.Y);
        vv.Translation.Z = op((float)aa.Translation.Z, (float)bb.Translation.Z, (float)cc.Translation.Z);
        vv.Orientation.X = op(aa.Orientation.X, bb.Orientation.X, cc.Orientation.X);
        vv.Orientation.Y = op(aa.Orientation.Y, bb.Orientation.Y, cc.Orientation.Y);
        vv.Orientation.Z = op(aa.Orientation.Z, bb.Orientation.Z, cc.Orientation.Z);
        vv.Orientation.W = op(aa.Orientation.W, bb.Orientation.W, cc.Orientation.W);
        vv.Scale.X = op(aa.Scale.X, bb.Scale.X, cc.Scale.X);
        vv.Scale.Y = op(aa.Scale.Y, bb.Scale.Y, cc.Scale.Y);
        vv.Scale.Z = op(aa.Scale.Z, bb.Scale.Z, cc.Scale.Z);
        break;
    }
    default:
        v = a;
        break;
    }
}

void GraphUtilities::ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a)
{
    // Select operation
    MathOp1 op;
    switch (typeId)
    {
    case 7:
        op = [](float a)
        {
            return Math::Abs(a);
        };
        break;
    case 8:
        op = [](float a)
        {
            return Math::Ceil(a);
        };
        break;
    case 9:
        op = [](float a)
        {
            return Math::Cos(a);
        };
        break;
    case 10:
        op = [](float a)
        {
            return Math::Floor(a);
        };
        break;
    case 13:
        op = [](float a)
        {
            return Math::Round(a);
        };
        break;
    case 14:
        op = [](float a)
        {
            return Math::Saturate(a);
        };
        break;
    case 15:
        op = [](float a)
        {
            return Math::Sin(a);
        };
        break;
    case 16:
        op = [](float a)
        {
            return Math::Sqrt(a);
        };
        break;
    case 17:
        op = [](float a)
        {
            return Math::Tan(a);
        };
        break;
    case 27:
        op = [](float a)
        {
            return -a;
        };
        break;
    case 28:
        op = [](float a)
        {
            return 1 - a;
        };
        break;
    case 33:
        op = [](float a)
        {
            return Math::Asin(a);
        };
        break;
    case 34:
        op = [](float a)
        {
            return Math::Acos(a);
        };
        break;
    case 35:
        op = [](float a)
        {
            return Math::Atan(a);
        };
        break;
    case 38:
        op = [](float a)
        {
            return Math::Trunc(a);
        };
        break;
    case 39:
        op = [](float a)
        {
            float tmp;
            return Math::ModF(a, &tmp);
        };
        break;
    case 43:
    {
        op = [](float a)
        {
            return a * RadiansToDegrees;
        };
        break;
    }
    case 44:
    {
        op = [](float a)
        {
            return a * DegreesToRadians;
        };
        break;
    }

    default:
        return;
    }

    // Perform operation
    ApplySomeMathHere(v, a, op);
}

void GraphUtilities::ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a, Variant& b)
{
    // Select operation
    MathOp2 op;
    switch (typeId)
    {
    case 1:
        op = [](float a, float b)
        {
            return a + b;
        };
        break;
    case 2:
        op = [](float a, float b)
        {
            return a - b;
        };
        break;
    case 3:
        op = [](float a, float b)
        {
            return a * b;
        };
        break;
    case 4:
        op = [](float a, float b)
        {
            return (float)((int)a % (int)b);
        };
        break;
    case 5:
        op = [](float a, float b)
        {
            return a / b;
        };
        break;
    case 21:
        op = [](float a, float b)
        {
            return Math::Max(a, b);
        };
        break;
    case 22:
        op = [](float a, float b)
        {
            return Math::Min(a, b);
        };
        break;
    case 23:
        op = [](float a, float b)
        {
            return Math::Pow(a, b);
        };
        break;
    case 40:
        op = [](float a, float b)
        {
            return Math::Mod(a, b);
        };
        break;
    case 41:
        op = [](float a, float b)
        {
            return Math::Atan2(a, b);
        };
        break;
    default:
        return;
    }

    // Perform operation
    ApplySomeMathHere(v, a, b, op);
}

int32 GraphUtilities::CountComponents(VariantType::Types type)
{
    switch (type)
    {
    case VariantType::Bool:
    case VariantType::Int:
    case VariantType::Int64:
    case VariantType::Uint:
    case VariantType::Uint64:
    case VariantType::Float:
    case VariantType::Double:
    case VariantType::Pointer:
        return 1;
    case VariantType::Float2:
    case VariantType::Double2:
    case VariantType::Int2:
        return 2;
    case VariantType::Float3:
    case VariantType::Double3:
    case VariantType::Int3:
        return 3;
    case VariantType::Float4:
    case VariantType::Double4:
    case VariantType::Int4:
    case VariantType::Color:
        return 4;
    default:
        return 0;
    }
}
