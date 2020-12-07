// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PARTICLE_GPU_GRAPH

#include "ParticleEmitterGraph.GPU.h"

void ParticleEmitterGPUGenerator::ProcessGroupTools(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Linearize Depth
    case 7:
    {
        // Get input
        const Value depth = tryGetValue(node->GetBox(0), Value::Zero).AsFloat();

        // Linearize raw device depth
        linearizeSceneDepth(node, depth, value);
        break;
    }
        // Time
    case 8:
    {
        value = box->ID == 0 ? Value(VariantType::Float, TEXT("Time")) : Value(VariantType::Float, TEXT("DeltaTime"));
        break;
    }
        // Transform Position To Screen UV
    case 9:
    {
        const Value position = tryGetValue(node->GetBox(0), Value::Zero).AsVector3();
        const Value projPos = writeLocal(VariantType::Vector4, String::Format(TEXT("mul(float4({0}, 1.0f), ViewProjectionMatrix)"), position.Value), node);
        _writer.Write(TEXT("\t{0}.xy /= {0}.w;\n"), projPos.Value);
        _writer.Write(TEXT("\t{0}.xy = {0}.xy * 0.5f + 0.5f;\n"), projPos.Value);
        value = Value(VariantType::Vector2, projPos.Value + TEXT(".xy"));
        break;
    }
    default:
        ShaderGenerator::ProcessGroupTools(box, node, value);
        break;
    }
}

#endif
