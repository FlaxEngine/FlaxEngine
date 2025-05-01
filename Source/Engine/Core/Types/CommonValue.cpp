// Copyright (c) Wojciech Figat. All rights reserved.

#include "CommonValue.h"
#include "Engine/Scripting/ScriptingObject.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

const CommonValue CommonValue::Zero(0.0f);
const CommonValue CommonValue::One(1.0f);
const CommonValue CommonValue::Null(static_cast<void*>(nullptr));
const CommonValue CommonValue::False(false);
const CommonValue CommonValue::True(true);

const Char* ToString(CommonType value)
{
    const Char* result;
    switch (value)
    {
    case CommonType::Bool:
        result = TEXT("Bool");
        break;
    case CommonType::Integer:
        result = TEXT("Integer");
        break;
    case CommonType::Float:
        result = TEXT("Float");
        break;
    case CommonType::Vector2:
        result = TEXT("Vector2");
        break;
    case CommonType::Vector3:
        result = TEXT("Vector3");
        break;
    case CommonType::Vector4:
        result = TEXT("Vector4");
        break;
    case CommonType::Color:
        result = TEXT("Color");
        break;
    case CommonType::Guid:
        result = TEXT("Guid");
        break;
    case CommonType::String:
        result = TEXT("String");
        break;
    case CommonType::Box:
        result = TEXT("Box");
        break;
    case CommonType::Rotation:
        result = TEXT("Rotation");
        break;
    case CommonType::Transform:
        result = TEXT("Transform");
        break;
    case CommonType::Sphere:
        result = TEXT("Sphere");
        break;
    case CommonType::Rectangle:
        result = TEXT("Rectangle");
        break;
    case CommonType::Pointer:
        result = TEXT("Pointer");
        break;
    case CommonType::Matrix:
        result = TEXT("Matrix");
        break;
    case CommonType::Blob:
        result = TEXT("Blob");
        break;
    case CommonType::Object:
        result = TEXT("Object");
        break;
    case CommonType::Ray:
        result = TEXT("Ray");
        break;
    default:
        result = TEXT("");
        break;
    }
    return result;
}

bool CommonValue::NearEqual(const CommonValue& a, const CommonValue& b, float epsilon)
{
    ASSERT(a.Type == b.Type);
    switch (a.Type)
    {
    case CommonType::Bool:
        return a.AsBool == b.AsBool;
    case CommonType::Integer:
        return Math::Abs(a.AsInteger - b.AsInteger) < epsilon;
    case CommonType::Float:
        return Math::Abs(a.AsFloat - b.AsFloat) < epsilon;
    case CommonType::Vector2:
        return Float2::NearEqual(a.AsVector2, b.AsVector2, epsilon);
    case CommonType::Vector3:
        return Float3::NearEqual(a.AsVector3, b.AsVector3, epsilon);
    case CommonType::Vector4:
        return Float4::NearEqual(a.AsVector4, b.AsVector4, epsilon);
    case CommonType::Color:
        return Color::NearEqual(a.AsColor, b.AsColor, epsilon);
    case CommonType::Guid:
        return a.AsGuid == b.AsGuid;
    case CommonType::String:
        return StringUtils::Compare(a.AsString, b.AsString) > 0;
    case CommonType::Box:
        return BoundingBox::NearEqual(a.AsBox, b.AsBox, epsilon);
    case CommonType::Rotation:
        return Quaternion::NearEqual(a.AsRotation, b.AsRotation, epsilon);
    case CommonType::Transform:
        return Transform::NearEqual(a.AsTransform, b.AsTransform, epsilon);
    case CommonType::Sphere:
        return BoundingSphere::NearEqual(a.AsSphere, b.AsSphere, epsilon);
    case CommonType::Rectangle:
        return Rectangle::NearEqual(a.AsRectangle, b.AsRectangle, epsilon);
    case CommonType::Ray:
        return Ray::NearEqual(a.AsRay, b.AsRay, epsilon);
    case CommonType::Pointer:
    case CommonType::Object:
        return a.AsPointer == b.AsPointer;
    case CommonType::Matrix:
        return a.AsMatrix == b.AsMatrix;
    case CommonType::Blob:
        return a.AsBlob.Length == b.AsBlob.Length;
    default: CRASH;
        return false;
    }
}

CommonValue CommonValue::Lerp(const CommonValue& a, const CommonValue& b, float alpha)
{
    ASSERT(a.Type == b.Type);
    switch (a.Type)
    {
    case CommonType::Bool:
        return alpha < 0.5f ? a : b;
    case CommonType::Integer:
        return Math::Lerp(a.AsInteger, b.AsInteger, alpha);
    case CommonType::Float:
        return Math::Lerp(a.AsFloat, b.AsFloat, alpha);
    case CommonType::Vector2:
        return Float2::Lerp(a.AsVector2, b.AsVector2, alpha);
    case CommonType::Vector3:
        return Float3::Lerp(a.AsVector3, b.AsVector3, alpha);
    case CommonType::Vector4:
        return Float4::Lerp(a.AsVector4, b.AsVector4, alpha);
    case CommonType::Color:
        return Color::Lerp(a.AsColor, b.AsColor, alpha);
    case CommonType::Box:
        return BoundingBox(Vector3::Lerp(a.AsBox.Minimum, b.AsBox.Minimum, alpha), Vector3::Lerp(a.AsBox.Maximum, b.AsBox.Maximum, alpha));
    case CommonType::Rotation:
        return Quaternion::Lerp(a.AsRotation, b.AsRotation, alpha);
    case CommonType::Transform:
        return Transform::Lerp(a.AsTransform, b.AsTransform, alpha);
    case CommonType::Sphere:
        return BoundingSphere(Vector3::Lerp(a.AsSphere.Center, b.AsSphere.Center, alpha), Math::Lerp(a.AsSphere.Radius, b.AsSphere.Radius, alpha));
    case CommonType::Rectangle:
        return Rectangle(Float2::Lerp(a.AsRectangle.Location, b.AsRectangle.Location, alpha), Float2::Lerp(a.AsRectangle.Size, b.AsRectangle.Size, alpha));
    case CommonType::Ray:
        return Ray(Vector3::Lerp(a.AsRay.Position, b.AsRay.Position, alpha), Vector3::Normalize(Vector3::Lerp(a.AsRay.Direction, b.AsRay.Direction, alpha)));
    default:
        return a;
    }
}

Guid CommonValue::GetObjectId() const
{
    ASSERT(Type == CommonType::Object);
    return AsObject ? AsObject->GetID() : Guid::Empty;
}

void CommonValue::LinkObject()
{
    AsObject->Deleted.Bind<CommonValue, &CommonValue::OnObjectDeleted>(this);
}

void CommonValue::UnlinkObject()
{
    AsObject->Deleted.Unbind<CommonValue, &CommonValue::OnObjectDeleted>(this);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
