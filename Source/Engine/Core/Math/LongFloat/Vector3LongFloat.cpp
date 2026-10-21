#pragma once
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector3LongFloat.h"
#include "Engine/Core/Math/LongFloat.h"

Vector3LongFloat Vector3LongFloat::Zero = Vector3LongFloat(LongFloat(0ll, 0ll), LongFloat(0ll, 0ll), LongFloat(0ll, 0ll));
Vector3LongFloat Vector3LongFloat::One = Vector3LongFloat(1ll, 1ll, 1ll);
Vector3LongFloat Vector3LongFloat::Up = Vector3LongFloat(0ll, 1ll, 0ll);
Vector3LongFloat Vector3LongFloat::Down = Vector3LongFloat(0ll, -1ll, 0ll);
Vector3LongFloat Vector3LongFloat::Left = Vector3LongFloat(-1ll, 0ll, 0ll);
Vector3LongFloat Vector3LongFloat::Right = Vector3LongFloat(1ll, 0ll, 0ll);
Vector3LongFloat Vector3LongFloat::Forward = Vector3LongFloat(0ll, 0ll, 1ll);
Vector3LongFloat Vector3LongFloat::Backward = Vector3LongFloat(0ll, 0ll, -1ll);

Float3 Vector3LongFloat::QuantizeToFloat3() const
{
    return LongFloatMath::QuantizeToFloat3(*this);
}

Double3 Vector3LongFloat::QuantizeToDouble3() const
{
    return LongFloatMath::QuantizeToDouble3(*this);
}

Vector3 Vector3LongFloat::Quantize() const
{
    return LongFloatMath::Quantize(*this);
}
