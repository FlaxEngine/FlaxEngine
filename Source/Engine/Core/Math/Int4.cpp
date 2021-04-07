// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Int2.h"
#include "Int3.h"
#include "Int4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Engine/Core/Types/String.h"

static_assert(sizeof(Int4) == 16, "Invalid Int4 type size.");

const Int4 Int4::Zero(0);
const Int4 Int4::One(1);
const Int4 Int4::Minimum(MIN_int32);
const Int4 Int4::Maximum(MAX_int32);

Int4::Int4(const Int2& xy, float z, float w)
        : X(xy.X)
        , Y(xy.Y)
        , Z(static_cast<int32>(z))
        , W(static_cast<int32>(w))
{
}

Int4::Int4(const Int3& xyz, float w)
        : X(xyz.X)
        , Y(xyz.Y)
        , Z(xyz.Z)
        , W(static_cast<int32>(w))
{
}

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
