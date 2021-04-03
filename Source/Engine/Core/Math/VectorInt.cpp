// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "VectorInt.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Engine/Core/Types/String.h"

const Int2 Int2::Zero(0);
const Int2 Int2::One(1);

Int2::Int2(const Vector2& v)
    : X(static_cast<int32>(v.X))
    , Y(static_cast<int32>(v.Y))
{
}

String Int2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

const Int3 Int3::Zero(0);
const Int3 Int3::One(1);

Int3::Int3(const Vector3& v)
    : X(static_cast<int32>(v.X))
    , Y(static_cast<int32>(v.Y))
    , Z(static_cast<int32>(v.Z))
{
}

String Int3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

const Int4 Int4::Zero(0);
const Int4 Int4::One(1);

Int4::Int4(const Vector2& v, float z, float w)
    : X(static_cast<int32>(v.X))
    , Y(static_cast<int32>(v.Y))
    , Z(static_cast<int32>(z))
    , W(static_cast<int32>(w))
{
}

Int4::Int4(const Vector3& v, float w)
    : X(static_cast<int32>(v.X))
    , Y(static_cast<int32>(v.Y))
    , Z(static_cast<int32>(v.Z))
    , W(static_cast<int32>(w))
{
}

Int4::Int4(const Vector4& v)
    : X(static_cast<int32>(v.X))
    , Y(static_cast<int32>(v.Y))
    , Z(static_cast<int32>(v.Z))
    , W(static_cast<int32>(v.W))
{
}

String Int4::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}
