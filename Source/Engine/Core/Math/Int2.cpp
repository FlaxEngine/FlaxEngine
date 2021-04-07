// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Int2.h"
#include "Int3.h"
#include "Int4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Engine/Core/Types/String.h"

static_assert(sizeof(Int2) == 8, "Invalid Int2 type size.");

const Int2 Int2::Zero(0);
const Int2 Int2::One(1);
const Int2 Int2::Minimum(MIN_float);
const Int2 Int2::Maximum(MAX_float);

Int2::Int2(const Int3& xyz)
    : X(xyz.X)
    , Y(xyz.Y)
{
}

Int2::Int2(const Int4& xyzw)
    : X(xyzw.X)
    , Y(xyzw.Y)
{
}

Int2::Int2(const Vector2& xy)
    : X(static_cast<int32>(xy.X))
    , Y(static_cast<int32>(xy.Y))
{
}

Int2::Int2(const Vector3& xyz)
    : X(static_cast<int32>(xyz.X))
    , Y(static_cast<int32>(xyz.Y))
{
}

Int2::Int2(const Vector4& xyzw)
    : X(static_cast<int32>(xyzw.X))
    , Y(static_cast<int32>(xyzw.Y))
{
}

String Int2::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}
