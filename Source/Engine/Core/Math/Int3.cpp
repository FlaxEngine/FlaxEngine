// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Int2.h"
#include "Int3.h"
#include "Int4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Engine/Core/Types/String.h"

static_assert(sizeof(Int3) == 12, "Invalid Int3 type size.");

const Int3 Int3::Zero(0);
const Int3 Int3::One(1);
const Int3 Int3::Minimum(MIN_int32);
const Int3 Int3::Maximum(MAX_int32);

Int3::Int3(const Int2& xy, float z)
    : X(xy.X)
    , Y(xy.Y)
    , Z(static_cast<int32>(z))
{
}

Int3::Int3(const Int4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
    , Z(xyzw.Z)
{
}

Int3::Int3(const Vector2& xy, float z)
    : X(static_cast<int32>(xy.X))
    , Y(static_cast<int32>(xy.Y))
    , Z(static_cast<int32>(z))
{
}

Int3::Int3(const Vector3& xyz)
    : X(static_cast<int32>(xyz.X))
    , Y(static_cast<int32>(xyz.Y))
    , Z(static_cast<int32>(xyz.Z))
{
}

Int3::Int3(const Vector4& xyzw)
    : X(static_cast<int32>(xyzw.X))
    , Y(static_cast<int32>(xyzw.Y))
    , Z(static_cast<int32>(xyzw.Z))
{
}

String Int3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}
