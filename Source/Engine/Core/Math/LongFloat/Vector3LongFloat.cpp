#pragma once
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector3LongFloat.h"
#include "Engine/Core/Math/LongFloat.h"

Vector3LongFloat Vector3LongFloat::Zero = Vector3LongFloat(LongFloat(0, 0), LongFloat(0, 0), LongFloat(0, 0));

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
