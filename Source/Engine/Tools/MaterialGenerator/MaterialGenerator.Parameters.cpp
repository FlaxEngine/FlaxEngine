// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"

void MaterialGenerator::ProcessGroupParameters(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Get
    case 1:
    {
        // Get parameter
        const auto param = findParam((Guid)node->Values[0], _treeLayer);
        if (param)
        {
            switch (param->Type)
            {
            case MaterialParameterType::Bool:
                value = Value(VariantType::Bool, param->ShaderName);
                break;
            case MaterialParameterType::Integer:
            case MaterialParameterType::SceneTexture:
                value = Value(VariantType::Int, param->ShaderName);
                break;
            case MaterialParameterType::Float:
                value = Value(VariantType::Float, param->ShaderName);
                break;
            case MaterialParameterType::Vector2:
            case MaterialParameterType::Vector3:
            case MaterialParameterType::Vector4:
            case MaterialParameterType::Color:
            {
                // Set result values based on box ID
                const Value sample(box->Type.Type, param->ShaderName);
                switch (box->ID)
                {
                case 0:
                    value = sample;
                    break;
                case 1:
                    value.Value = sample.Value + _subs[0];
                    break;
                case 2:
                    value.Value = sample.Value + _subs[1];
                    break;
                case 3:
                    value.Value = sample.Value + _subs[2];
                    break;
                case 4:
                    value.Value = sample.Value + _subs[3];
                    break;
                default: CRASH;
                    break;
                }
                value.Type = box->Type.Type;
                break;
            }
            case MaterialParameterType::Matrix:
            {
                value = Value(box->Type.Type, String::Format(TEXT("{0}[{1}]"), param->ShaderName, box->ID));
                break;
            }
            case MaterialParameterType::ChannelMask:
            {
                const auto input = tryGetValue(node->GetBox(0), Value::Zero);
                value = writeLocal(VariantType::Float, String::Format(TEXT("dot({0}, {1})"), input.Value, param->ShaderName), node);
                break;
            }
            case MaterialParameterType::CubeTexture:
            case MaterialParameterType::NormalMap:
            case MaterialParameterType::Texture:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTextureCube:
            case MaterialParameterType::GPUTextureVolume:
            case MaterialParameterType::GPUTexture:
                sampleTexture(node, value, box, param);
                break;
            default: CRASH;
                break;
            }
        }
        else
        {
            OnError(node, box, String::Format(TEXT("Missing graph parameter {0}."), node->Values[0]));
            value = Value::Zero;
        }
        break;
    }
    default:
        break;
    }
}

#endif
