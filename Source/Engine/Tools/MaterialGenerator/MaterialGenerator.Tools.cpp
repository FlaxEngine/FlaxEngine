// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"

void MaterialGenerator::ProcessGroupTools(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Fresnel
    case 1:
    case 4:
    {
        // Gets constants
        auto cameraVector = getCameraVector(node);

        // Get inputs
        Value exponent = tryGetValue(node->GetBox(0), 0, Value(5.0f)).AsFloat();
        Value fraction = tryGetValue(node->GetBox(1), 1, Value(0.04f)).AsFloat();
        Value normal = tryGetValue(node->GetBox(2), getNormal).AsFloat3();

        // Write operations
        auto local1 = writeFunction2(node, normal, cameraVector, TEXT("dot"), VariantType::Float);
        auto local2 = writeFunction2(node, Value::Zero, local1, TEXT("max"));
        auto local3 = writeOperation2(node, Value::One, local2, '-');
        auto local4 = writeFunction2(node, local3, exponent, TEXT("ClampedPow"), VariantType::Float);
        auto local5 = writeLocal(VariantType::Float, String::Format(TEXT("{0} * (1.0 - {1})"), local4.Value, fraction.Value), node);
        auto local6 = writeOperation2(node, local5, fraction, '+');
        _includes.Add(TEXT("./Flax/Math.hlsl"));

        // Gets value
        value = local6;
        break;
    }
    // Desaturation
    case 2:
    {
        // Get inputs
        Value input = tryGetValue(node->GetBox(0), Value::Zero).AsFloat3();
        Value scale = tryGetValue(node->GetBox(1), Value::Zero).AsFloat();
        Value luminanceFactors = Value(node->Values[0].AsFloat3());

        // Write operations
        auto dot = writeFunction2(node, input, luminanceFactors, TEXT("dot"), VariantType::Float);
        value = writeFunction3(node, input, dot, scale, TEXT("lerp"), VariantType::Float3);
        break;
    }
    // Time
    case 3:
        value = box->ID == 1 ? getUnscaledTime : getTime;
        break;
    // Panner
    case 6:
    {
        // Get inputs
        const Value uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const Value time = tryGetValue(node->GetBox(1), getTime).AsFloat();
        const Value speed = tryGetValue(node->GetBox(2), Value::One).AsFloat2();
        const bool useFractionalPart = (bool)node->Values[0];

        // Write operations
        auto local1 = writeOperation2(node, speed, time, '*');
        if (useFractionalPart)
            local1 = writeFunction1(node, local1, TEXT("frac"));
        value = writeOperation2(node, uv, local1, '+');
        break;
    }
    // Linearize Depth
    case 7:
    {
        // Get input
        const Value depth = tryGetValue(node->GetBox(0), Value::Zero).AsFloat();

        // Linearize raw device depth
        linearizeSceneDepth(node, depth, value);
        break;
    }
    default:
        ShaderGenerator::ProcessGroupTools(box, node, value);
        break;
    }
}

#endif
