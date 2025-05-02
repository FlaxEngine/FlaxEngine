// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Visject/Graph.h"

namespace GraphUtilities
{
    typedef float (*MathOp1)(float);
    typedef float (*MathOp2)(float, float);
    typedef float (*MathOp3)(float, float, float);

    void ApplySomeMathHere(Variant& v, Variant& a, MathOp1 op);
    void ApplySomeMathHere(Variant& v, Variant& a, Variant& b, MathOp2 op);
    void ApplySomeMathHere(Variant& v, Variant& a, Variant& b, Variant& c, MathOp3 op);

    void ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a);
    void ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a, Variant& b);

    int32 CountComponents(VariantType::Types type);
}
