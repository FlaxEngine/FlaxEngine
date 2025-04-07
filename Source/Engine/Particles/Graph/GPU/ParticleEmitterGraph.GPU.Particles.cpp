// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PARTICLE_GPU_GRAPH

#include "ParticleEmitterGraph.GPU.h"
#include "Engine/Particles/ParticleEmitterFunction.h"

namespace
{
    VariantType::Types GetValueType(ParticleAttribute::ValueTypes valueType)
    {
        VariantType::Types type;
        switch (valueType)
        {
        case ParticleAttribute::ValueTypes::Float:
            type = VariantType::Float;
            break;
        case ParticleAttribute::ValueTypes::Float2:
            type = VariantType::Float2;
            break;
        case ParticleAttribute::ValueTypes::Float3:
            type = VariantType::Float3;
            break;
        case ParticleAttribute::ValueTypes::Float4:
            type = VariantType::Float4;
            break;
        case ParticleAttribute::ValueTypes::Int:
            type = VariantType::Int;
            break;
        case ParticleAttribute::ValueTypes::Uint:
            type = VariantType::Uint;
            break;
        default:
            type = VariantType::Null;
        }
        return type;
    }
}

ParticleEmitterGPUGenerator::Value ParticleEmitterGPUGenerator::AccessParticleAttribute(Node* caller, const StringView& name, ParticleAttribute::ValueTypes valueType, AccessMode mode)
{
    // Find this attribute
    return AccessParticleAttribute(caller, GetRootGraph()->Layout.FindAttribute(name, valueType), mode);
}

ParticleEmitterGPUGenerator::Value ParticleEmitterGPUGenerator::AccessParticleAttribute(Node* caller, int32 index, AccessMode mode)
{
    // Handle invalid attribute
    if (index == -1)
        return Value::Zero;

    // Try reuse cached value
    auto& value = _attributeValues[index];
    value.Access = (AccessMode)((int)value.Access | (int)mode);
    if (value.Variable.Type != VariantType::Null)
        return value.Variable;

    auto& attribute = GetRootGraph()->Layout.Attributes[index];
    const VariantType::Types type = GetValueType(attribute.ValueType);

    // Generate local variable name that matches the attribute name for easier shader source debugging
    String attributeNameAnsi;
    for (int32 i = 0; i < attribute.Name.Length(); i++)
    {
        if (StringUtils::IsAlnum(attribute.Name[i]))
            attributeNameAnsi += attribute.Name[i];
    }
    if (attributeNameAnsi.IsEmpty())
        attributeNameAnsi = getLocalName(_localIndex++);
    const String localName = String(TEXT("particle")) + attributeNameAnsi;

    if (mode == AccessMode::Write)
    {
        // Create local variable
        value.Variable = writeLocal(type, caller, localName);
    }
    else if (_contextType == ParticleContextType::Initialize)
    {
        // Initialize with default value
        const Value defaultValue(GetRootGraph()->AttributesDefaults[index]);
        value.Variable = writeLocal(type, defaultValue.Value, caller, localName);
    }
    else
    {
        // Read from the attributes buffer
        const Char* format;
        switch (attribute.ValueType)
        {
        case ParticleAttribute::ValueTypes::Float:
            format = TEXT("GetParticleFloat(context.ParticleIndex, {0})");
            break;
        case ParticleAttribute::ValueTypes::Float2:
            format = TEXT("GetParticleVec2(context.ParticleIndex, {0})");
            break;
        case ParticleAttribute::ValueTypes::Float3:
            format = TEXT("GetParticleVec3(context.ParticleIndex, {0})");
            break;
        case ParticleAttribute::ValueTypes::Float4:
            format = TEXT("GetParticleVec4(context.ParticleIndex, {0})");
            break;
        case ParticleAttribute::ValueTypes::Int:
            format = TEXT("GetParticleInt(context.ParticleIndex, {0})");
            break;
        case ParticleAttribute::ValueTypes::Uint:
            format = TEXT("GetParticleUint(context.ParticleIndex, {0})");
            break;
        default:
            return Value::Zero;
        }
        value.Variable = writeLocal(type, String::Format(format, attribute.Offset), caller, localName);
    }

    return value.Variable;
}

void ParticleEmitterGPUGenerator::ProcessGroupParameters(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Get
    case 1:
    case 2:
    {
        // Get parameter
        const auto param = findParam((Guid)node->Values[0]);
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
            case MaterialParameterType::Texture:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTextureCube:
            case MaterialParameterType::GPUTextureVolume:
            case MaterialParameterType::GPUTexture:
                value = Value(VariantType::Object, param->ShaderName);
                break;
            default:
                CRASH;
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
        value = box->ID == 0 ? Value(VariantType::Float, TEXT("Time")) : Value(VariantType::Float, TEXT("DeltaTime"));
        break;
    // Transform Position To Screen UV
    case 9:
    {
        const Value position = tryGetValue(node->GetBox(0), Value::Zero).AsFloat3();
        const Value projPos = writeLocal(VariantType::Float4, String::Format(TEXT("mul(float4({0}, 1.0f), ViewProjectionMatrix)"), position.Value), node);
        _writer.Write(TEXT("\t{0}.xy /= {0}.w;\n"), projPos.Value);
        _writer.Write(TEXT("\t{0}.xy = {0}.xy * 0.5f + 0.5f;\n"), projPos.Value);
        value = Value(VariantType::Float2, projPos.Value + TEXT(".xy"));
        break;
    }
    default:
        ShaderGenerator::ProcessGroupTools(box, node, value);
        break;
    }
}

void ParticleEmitterGPUGenerator::ProcessGroupParticles(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Particle Attribute
    case 100:
        value = AccessParticleAttribute(node, (StringView)node->Values[0], static_cast<ParticleAttribute::ValueTypes>(node->Values[1].AsInt), AccessMode::Read);
        break;
    // Particle Attribute (by index)
    case 303:
    {
        const Char* format;
        auto valueType = static_cast<ParticleAttribute::ValueTypes>(node->Values[1].AsInt);
        const int32 attributeIndex = GetRootGraph()->Layout.FindAttribute((StringView)node->Values[0], valueType);
        if (attributeIndex == -1)
            return;
        auto& attribute = GetRootGraph()->Layout.Attributes[attributeIndex];
        const auto particleIndex = Value::Cast(tryGetValue(node->GetBox(1), Value(VariantType::Uint, TEXT("context.ParticleIndex"))), VariantType::Uint);
        switch (valueType)
        {
        case ParticleAttribute::ValueTypes::Float:
            format = TEXT("GetParticleFloat({1}, {0})");
            break;
        case ParticleAttribute::ValueTypes::Float2:
            format = TEXT("GetParticleVec2({1}, {0})");
            break;
        case ParticleAttribute::ValueTypes::Float3:
            format = TEXT("GetParticleVec3({1}, {0})");
            break;
        case ParticleAttribute::ValueTypes::Float4:
            format = TEXT("GetParticleVec4({1}, {0})");
            break;
        case ParticleAttribute::ValueTypes::Int:
            format = TEXT("GetParticleInt({1}, {0})");
            break;
        case ParticleAttribute::ValueTypes::Uint:
            format = TEXT("GetParticleUint({1}, {0})");
            break;
        default:
            return;
        }
        const VariantType::Types type = GetValueType(attribute.ValueType);
        value = writeLocal(type, String::Format(format, attribute.Offset, particleIndex.Value), node);
        break;
    }
    // Particle Position
    case 101:
        value = AccessParticleAttribute(node, TEXT("Position"), ParticleAttribute::ValueTypes::Float3, AccessMode::Read);
        break;
    // Particle Lifetime
    case 102:
        value = AccessParticleAttribute(node, TEXT("Lifetime"), ParticleAttribute::ValueTypes::Float, AccessMode::Read);
        break;
    // Particle Age
    case 103:
        value = AccessParticleAttribute(node, TEXT("Age"), ParticleAttribute::ValueTypes::Float, AccessMode::Read);
        break;
    // Particle Color
    case 104:
        value = AccessParticleAttribute(node, TEXT("Color"), ParticleAttribute::ValueTypes::Float4, AccessMode::Read);
        break;
    // Particle Velocity
    case 105:
        value = AccessParticleAttribute(node, TEXT("Velocity"), ParticleAttribute::ValueTypes::Float3, AccessMode::Read);
        break;
    // Particle Sprite Size
    case 106:
        value = AccessParticleAttribute(node, TEXT("SpriteSize"), ParticleAttribute::ValueTypes::Float2, AccessMode::Read);
        break;
    // Particle Mass
    case 107:
        value = AccessParticleAttribute(node, TEXT("Mass"), ParticleAttribute::ValueTypes::Float, AccessMode::Read);
        break;
    // Particle Rotation
    case 108:
        value = AccessParticleAttribute(node, TEXT("Rotation"), ParticleAttribute::ValueTypes::Float3, AccessMode::Read);
        break;
    // Particle Angular Velocity
    case 109:
        value = AccessParticleAttribute(node, TEXT("AngularVelocity"), ParticleAttribute::ValueTypes::Float3, AccessMode::Read);
        break;
    // Particle Normalized Age
    case 110:
    {
        const auto age = AccessParticleAttribute(node, TEXT("Age"), ParticleAttribute::ValueTypes::Float, AccessMode::Read);
        const auto lifetime = AccessParticleAttribute(node, TEXT("Lifetime"), ParticleAttribute::ValueTypes::Float, AccessMode::Read);
        value = writeOperation2(node, age, lifetime, '/');
        break;
    }
    // Particle Radius
    case 111:
        value = AccessParticleAttribute(node, TEXT("Radius"), ParticleAttribute::ValueTypes::Float, AccessMode::Read);
        break;
    // Particle Scale
    case 112:
        value = AccessParticleAttribute(node, TEXT("Scale"), ParticleAttribute::ValueTypes::Float3, AccessMode::Read);
        break;
    // Effect Position
    case 200:
        value = Value(VariantType::Float3, TEXT("EffectPosition"));
        break;
    // Effect Rotation
    case 201:
        value = Value(VariantType::Quaternion, TEXT("EffectRotation"));
        break;
    // Effect Scale
    case 202:
        value = Value(VariantType::Float3, TEXT("EffectScale"));
        break;
    // Simulation Mode
    case 203:
        value = Value(box->ID == 1);
        break;
    // View Position
    case 204:
        value = Value(VariantType::Float3, TEXT("ViewPos"));
        break;
    // View Direction
    case 205:
        value = Value(VariantType::Float3, TEXT("ViewDir"));
        break;
    // View Far Plane
    case 206:
        value = Value(VariantType::Float, TEXT("ViewFar"));
        break;
    // Screen Size
    case 207:
        value = Value(VariantType::Float2, box->ID == 0 ? TEXT("ScreenSize.xy") : TEXT("ScreenSize.zw"));
        break;
    // Random Float
    case 208:
        value = writeLocal(VariantType::Float, TEXT("RAND"), node);
        break;
    // Random Vector2
    case 209:
        value = writeLocal(VariantType::Float2, TEXT("RAND2"), node);
        break;
    // Random Vector3
    case 210:
        value = writeLocal(VariantType::Float3, TEXT("RAND3"), node);
        break;
    // Random Vector4
    case 211:
        value = writeLocal(VariantType::Float4, TEXT("RAND4"), node);
        break;
    // Particle Position (world space)
    case 212:
        value = AccessParticleAttribute(node, TEXT("Position"), ParticleAttribute::ValueTypes::Float3, AccessMode::Read);
        if (IsLocalSimulationSpace())
            value = writeLocal(VariantType::Float3, String::Format(TEXT("mul(float4({0}, 1), WorldMatrix).xyz"), value.Value), node);
        break;
    // Random Float Range
    case 213:
    {
        auto a = tryGetValue(node->TryGetBox(1), node->Values[0]).AsFloat();
        auto b = tryGetValue(node->TryGetBox(2), node->Values[1]).AsFloat();
        value = writeLocal(VariantType::Float, String::Format(TEXT("lerp({0}, {1}, RAND)"), a.Value, b.Value), node);
        break;
    }
    // Random Vector2 Range
    case 214:
    {
        auto a = tryGetValue(node->TryGetBox(1), node->Values[0]).AsFloat2();
        auto b = tryGetValue(node->TryGetBox(2), node->Values[1]).AsFloat2();
        value = writeLocal(VariantType::Float2, String::Format(TEXT("float2(lerp({0}.x, {1}.x, RAND), lerp({0}.y, {1}.y, RAND))"), a.Value, b.Value), node);
        break;
    }
    // Random Vector3 Range
    case 215:
    {
        auto a = tryGetValue(node->TryGetBox(1), node->Values[0]).AsFloat3();
        auto b = tryGetValue(node->TryGetBox(2), node->Values[1]).AsFloat3();
        value = writeLocal(VariantType::Float3, String::Format(TEXT("float3(lerp({0}.x, {1}.x, RAND), lerp({0}.y, {1}.y, RAND), lerp({0}.z, {1}.z, RAND))"), a.Value, b.Value), node);
        break;
    }
    // Random Vector4 Range
    case 216:
    {
        auto a = tryGetValue(node->TryGetBox(1), node->Values[0]).AsFloat4();
        auto b = tryGetValue(node->TryGetBox(2), node->Values[1]).AsFloat4();
        value = writeLocal(VariantType::Float4, String::Format(TEXT("float4(lerp({0}.x, {1}.x, RAND), lerp({0}.y, {1}.y, RAND), lerp({0}.z, {1}.z, RAND), lerp({0}.w, {1}.w, RAND))"), a.Value, b.Value), node);
        break;
    }
    // Particle Emitter Function
    case 300:
    {
        // Load function asset
        const auto function = Assets.LoadAsync<ParticleEmitterFunction>((Guid)node->Values[0]);
        if (!function || function->WaitForLoaded())
        {
            OnError(node, box, TEXT("Missing or invalid function."));
            value = Value::Zero;
            break;
        }

#if 0
        // Prevent recursive calls
        for (int32 i = _callStack.Count() - 1; i >= 0; i--)
        {
            if (_callStack[i]->Type == GRAPH_NODE_MAKE_TYPE(14, 300))
            {
                const auto callFunc = Assets.LoadAsync<ParticleEmitterFunction>((Guid)_callStack[i]->Values[0]);
                if (callFunc == function)
                {
                    OnError(node, box, String::Format(TEXT("Recursive call to function '{0}'!"), function->ToString()));
                    value = Value::Zero;
                    return;
                }
            }
        }
#endif

        // Create a instanced version of the function graph
        Graph* graph;
        if (!_functions.TryGet(node, graph))
        {
            graph = (Graph*)New<ParticleEmitterGraphGPU>();
            function->LoadSurface((ParticleEmitterGraphGPU&)*graph);
            _functions.Add(node, graph);
        }

        // Peek the function output (function->Outputs maps the functions outputs to output nodes indices)
        const int32 outputIndex = box->ID - 16;
        if (outputIndex < 0 || outputIndex >= function->Outputs.Count())
        {
            OnError(node, box, TEXT("Invalid function output box."));
            value = Value::Zero;
            break;
        }
        Node* functionOutputNode = &graph->Nodes[function->Outputs[outputIndex]];
        Box* functionOutputBox = functionOutputNode->TryGetBox(0);

        // Evaluate the function output
        _graphStack.Push(graph);
        value = functionOutputBox && functionOutputBox->HasConnection() ? eatBox(node, functionOutputBox->FirstConnection()) : Value::Zero;
        _graphStack.Pop();
        break;
    }
    // Particle Index
    case 301:
        value = Value(VariantType::Uint, TEXT("context.ParticleIndex"));
        break;
    // Particles Count
    case 302:
        value = Value(VariantType::Uint, TEXT("context.ParticlesCount"));
        break;
    default:
        break;
    }
}

void ParticleEmitterGPUGenerator::ProcessGroupFunction(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Function Input
    case 1:
    {
        // Find the function call
        Node* functionCallNode = nullptr;
        ASSERT(_graphStack.Count() >= 2);
        Graph* graph;
        for (int32 i = _callStack.Count() - 1; i >= 0; i--)
        {
            if (_callStack[i]->Type == GRAPH_NODE_MAKE_TYPE(14, 300) && _functions.TryGet(_callStack[i], graph) && _graphStack[_graphStack.Count() - 1] == graph)
            {
                functionCallNode = _callStack[i];
                break;
            }
        }
        if (!functionCallNode)
        {
            OnError(node, box, TEXT("Missing calling function node."));
            value = Value::Zero;
            break;
        }
        const auto function = Assets.LoadAsync<ParticleEmitterFunction>((Guid)functionCallNode->Values[0]);
        if (!_functions.TryGet(functionCallNode, graph) || !function)
        {
            OnError(node, box, TEXT("Missing calling function graph."));
            value = Value::Zero;
            break;
        }

        // Peek the input box to use
        int32 inputIndex = -1;
        for (int32 i = 0; i < function->Inputs.Count(); i++)
        {
            if (node->ID == graph->Nodes[function->Inputs[i]].ID)
            {
                inputIndex = i;
                break;
            }
        }
        if (inputIndex < 0 || inputIndex >= function->Inputs.Count())
        {
            OnError(node, box, TEXT("Invalid function input box."));
            value = Value::Zero;
            break;
        }
        Box* functionCallBox = functionCallNode->TryGetBox(inputIndex);
        if (functionCallBox && functionCallBox->HasConnection())
        {
            // Use provided input value from the function call
            _graphStack.Pop();
            value = eatBox(node, functionCallBox->FirstConnection());
            _graphStack.Push(graph);
        }
        else
        {
            // Use the default value from the function graph
            value = tryGetValue(node->TryGetBox(1), Value::Zero);
        }
        break;
    }
    default:
        break;
    }
}

#endif
