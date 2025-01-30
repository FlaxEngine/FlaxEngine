// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ParticleEmitterGraph.CPU.h"
#include "Engine/Particles/ParticleEmitter.h"
#include "Engine/Particles/ParticleEffect.h"
#include "Engine/Particles/ParticleEmitterFunction.h"
#include "Engine/Graphics/RenderTask.h"

#define GET_VIEW() auto mainViewTask = MainRenderTask::Instance && MainRenderTask::Instance->LastUsedFrame != 0 ? MainRenderTask::Instance : nullptr
#define ACCESS_PARTICLE_ATTRIBUTE(index) (context.Data->Buffer->GetParticleCPU(context.ParticleIndex) + context.Data->Buffer->Layout->Attributes[context.AttributesRemappingTable[node->Attributes[index]]].Offset)
#define GET_PARTICLE_ATTRIBUTE(index, type) *(type*)ACCESS_PARTICLE_ATTRIBUTE(index)

void ParticleEmitterGraphCPUExecutor::ProcessGroupParameters(Box* box, Node* node, Value& value)
{
    auto& context = *Context.Get();
    switch (node->TypeID)
    {
    // Get
    case 1:
    case 2:
    {
        int32 paramIndex;
        const auto param = _graph.GetParameter((Guid)node->Values[0], paramIndex);
        if (param)
        {
            value = context.Data->Parameters[paramIndex];
            switch (param->Type.Type)
            {
            case VariantType::Float2:
                switch (box->ID)
                {
                case 1:
                case 2:
                    value = value.AsFloat2().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Float3:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                    value = value.AsFloat3().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Float4:
            case VariantType::Color:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                case 4:
                    value = value.AsFloat4().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Double2:
                switch (box->ID)
                {
                case 1:
                case 2:
                    value = value.AsDouble2().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Double3:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                    value = value.AsDouble3().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Double4:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                case 4:
                    value = value.AsDouble4().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Matrix:
            {
                auto& matrix = value.Type.Type == VariantType::Matrix && value.AsBlob.Data ? *(Matrix*)value.AsBlob.Data : Matrix::Identity;
                switch (box->ID)
                {
                case 0:
                    value = matrix.GetRow1();
                    break;
                case 1:
                    value = matrix.GetRow2();
                    break;
                case 2:
                    value = matrix.GetRow3();
                    break;
                case 3:
                    value = matrix.GetRow4();
                    break;
                default:
                    break;
                }
                break;
            }
            }
        }
        else
        {
            // TODO: add warning that no parameter selected
            value = Value::Zero;
        }
        break;
    }
    default:
        break;
    }
}

void ParticleEmitterGraphCPUExecutor::ProcessGroupTextures(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Scene Texture
    case 6:
    {
        // Not supported
        value = Value::Zero;
        break;
    }
    // Scene Depth
    case 8:
    {
        // Not supported
        value = Value::Zero;
        break;
    }
    // Texture
    case 11:
    {
        // TODO: support sampling textures in CPU particles
        value = Value::Zero;
        break;
    }
    // Load Texture
    case 13:
    {
        // TODO: support sampling textures in CPU particles
        value = Value::Zero;
        break;
    }
    // Sample Global SDF
    case 14:
    {
        // Not supported
        value = Value::Zero;
        break;
    }
    default:
        break;
    }
}

void ParticleEmitterGraphCPUExecutor::ProcessGroupTools(Box* box, Node* node, Value& value)
{
    auto& context = *Context.Get();
    switch (node->TypeID)
    {
    // Linearize Depth
    case 7:
    {
        // TODO: support Linearize Depth in CPU particles
        value = Value::Zero;
        break;
    }
    // Time
    case 8:
        value = box->ID == 0 ? context.Data->Time : context.DeltaTime;
        break;
    // Transform Position To Screen UV
    case 9:
    {
        GET_VIEW();
        const Matrix viewProjection = context.ViewTask ? context.ViewTask->View.PrevViewProjection : Matrix::Identity;
        const Float3 position = (Float3)TryGetValue(node->GetBox(0), Value::Zero);
        Float4 projPos;
        Float3::Transform(position, viewProjection, projPos);
        projPos /= projPos.W;
        value = Float2(projPos.X * 0.5f + 0.5f, projPos.Y * 0.5f + 0.5f);
        break;
    }
    default:
        VisjectExecutor::ProcessGroupTools(box, node, value);
        break;
    }
}

void ParticleEmitterGraphCPUExecutor::ProcessGroupParticles(Box* box, Node* nodeBase, Value& value)
{
    auto& context = *Context.Get();
    auto node = (ParticleEmitterGraphCPUNode*)nodeBase;
    switch (node->TypeID)
    {
    // Particle Attribute
    case 100:
    {
        byte* ptr = ACCESS_PARTICLE_ATTRIBUTE(0);
        switch ((ParticleAttribute::ValueTypes)node->Attributes[1])
        {
        case ParticleAttribute::ValueTypes::Float:
            value = *(float*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Float2:
            value = *(Float2*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Float3:
            value = *(Float3*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Float4:
            value = *(Float4*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Int:
            value = *(int32*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Uint:
            value = *(uint32*)ptr;
            break;
        default: ;
        }
        break;
    }
    // Particle Attribute (by index)
    case 303:
    {
        const auto particleIndex = tryGetValue(node->GetBox(1), context.ParticleIndex);
        byte* ptr = (context.Data->Buffer->GetParticleCPU((uint32)particleIndex) + context.Data->Buffer->Layout->Attributes[node->Attributes[0]].Offset);
        switch ((ParticleAttribute::ValueTypes)node->Attributes[1])
        {
        case ParticleAttribute::ValueTypes::Float:
            value = *(float*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Float2:
            value = *(Float2*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Float3:
            value = *(Float3*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Float4:
            value = *(Float4*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Int:
            value = *(int32*)ptr;
            break;
        case ParticleAttribute::ValueTypes::Uint:
            value = *(uint32*)ptr;
            break;
        default: ;
        }
        break;
    }
    // Particle Position
    case 101:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float3);
        break;
    }
    // Particle Lifetime
    case 102:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, float);
        break;
    }
    // Particle Age
    case 103:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, float);
        break;
    }
    // Particle Color
    case 104:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float4);
        break;
    }
    // Particle Velocity
    case 105:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float3);
        break;
    }
    // Particle Sprite Size
    case 106:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float2);
        break;
    }
    // Particle Mass
    case 107:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, float);
        break;
    }
    // Particle Rotation
    case 108:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float3);
        break;
    }
    // Particle Angular Velocity
    case 109:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float3);
        break;
    }
    // Particle Normalized Age
    case 110:
    {
        const float age = GET_PARTICLE_ATTRIBUTE(0, float);
        const float lifetime = GET_PARTICLE_ATTRIBUTE(1, float);
        value = age / Math::Max(lifetime, ZeroTolerance);
        break;
    }
    // Particle Radius
    case 111:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, float);
        break;
    }
    // Particle Scale
    case 112:
    {
        value = GET_PARTICLE_ATTRIBUTE(0, Float3);
        break;
    }
    // Effect Position
    case 200:
    {
        value = context.Effect->GetPosition();
        break;
    }
    // Effect Rotation
    case 201:
    {
        value = context.Effect->GetOrientation();
        break;
    }
    // Effect Scale
    case 202:
    {
        value = context.Effect->GetScale();
        break;
    }
    // Simulation Mode
    case 203:
    {
        value = box->ID == 0;
        break;
    }
    // View Position
    case 204:
    {
        value = context.ViewTask ? context.ViewTask->View.Position : Float3::Zero;
        break;
    }
    // View Direction
    case 205:
    {
        value = context.ViewTask ? context.ViewTask->View.Direction : Float3::Forward;
        break;
    }
    // View Far Plane
    case 206:
    {
        value = context.ViewTask ? context.ViewTask->View.Far : 0.0f;
        break;
    }
    // Screen Size
    case 207:
    {
        const Float4 size = context.ViewTask ? context.ViewTask->View.ScreenSize : Float4::Zero;
        if (box->ID == 0)
            value = Float2(size.X, size.Y);
        else
            value = Float2(size.Z, size.W);
        break;
    }
    // Particle Position (world space)
    case 212:
        value = GET_PARTICLE_ATTRIBUTE(0, Float3);
        if (context.Emitter->SimulationSpace == ParticlesSimulationSpace::Local)
            value.AsFloat3() = context.Effect->GetTransform().LocalToWorld(value.AsFloat3());
        break;
    // Particle Emitter Function
    case 300:
    {
        // Load function asset
        const auto function = node->Assets[0].As<ParticleEmitterFunction>();
        if (!function || function->WaitForLoaded())
        {
            value = Value::Zero;
            break;
        }

#if 0
        // Prevent recursive calls
        for (int32 i = context.CallStack.Count() - 1; i >= 0; i--)
        {
            if (context.CallStack[i]->Type == GRAPH_NODE_MAKE_TYPE(14, 300))
            {
                const auto callFunc = context.CallStack[i]->Assets[0].Get();
                if (callFunc == function)
                {
                    value = Value::Zero;
                    return;
                }
            }
        }
#endif

        // Get function graph
        ParticleEmitterGraphCPU* graph = &function->Graph;
        context.Functions[nodeBase] = graph;

        // Peek the function output (function->Outputs maps the functions outputs to output nodes indices)
        const int32 outputIndex = box->ID - 16;
        if (outputIndex < 0 || outputIndex >= function->Outputs.Count())
        {
            value = Value::Zero;
            break;
        }
        auto* functionOutputNode = &graph->Nodes[function->Outputs[outputIndex]];
        Box* functionOutputBox = functionOutputNode->TryGetBox(0);

        // Setup particle attributes remapping (so particle data access nodes inside the function will read data at proper offset, see macro ACCESS_PARTICLE_ATTRIBUTE)
        byte attributesRemappingTable[PARTICLE_ATTRIBUTES_MAX_COUNT];
        Platform::MemoryCopy(attributesRemappingTable, context.AttributesRemappingTable, sizeof(attributesRemappingTable));
        for (int32 i = 0; i < graph->Layout.Attributes.Count(); i++)
        {
            const ParticleAttribute& e = graph->Layout.Attributes[i];
            context.AttributesRemappingTable[i] = context.Data->Buffer->Layout->FindAttribute(e.Name, e.ValueType);
        }

        // Evaluate the function output
        context.GraphStack.Push(graph);
        value = functionOutputBox && functionOutputBox->HasConnection() ? eatBox(nodeBase, functionOutputBox->FirstConnection()) : Value::Zero;
        Platform::MemoryCopy(context.AttributesRemappingTable, attributesRemappingTable, sizeof(attributesRemappingTable));
        context.GraphStack.Pop();
        break;
    }
    // Particle Index
    case 301:
        value = context.ParticleIndex;
        break;
    // Particles Count
    case 302:
        value = (uint32)context.Data->Buffer->CPU.Count;
        break;
    default:
        VisjectExecutor::ProcessGroupParticles(box, nodeBase, value);
        break;
    }
}

void ParticleEmitterGraphCPUExecutor::ProcessGroupFunction(Box* box, Node* node, Value& value)
{
    auto& context = *Context.Get();
    switch (node->TypeID)
    {
    // Function Input
    case 1:
    {
        // Find the function call
        Node* functionCallNode = nullptr;
        ASSERT(context.GraphStack.Count() >= 2);
        ParticleEmitterGraphCPU* graph;
        for (int32 i = context.CallStackSize - 1; i >= 0; i--)
        {
            if (context.CallStack[i]->Type == GRAPH_NODE_MAKE_TYPE(14, 300) && context.Functions.TryGet(context.CallStack[i], graph) && context.GraphStack[context.GraphStack.Count() - 1] == graph)
            {
                functionCallNode = context.CallStack[i];
                break;
            }
        }
        if (!functionCallNode)
        {
            value = Value::Zero;
            break;
        }
        const auto function = functionCallNode->Assets[0].As<ParticleEmitterFunction>();
        if (!context.Functions.TryGet(functionCallNode, graph) || !function)
        {
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
            value = Value::Zero;
            break;
        }
        Box* functionCallBox = functionCallNode->TryGetBox(inputIndex);
        if (functionCallBox && functionCallBox->HasConnection())
        {
            // Use provided input value from the function call
            context.GraphStack.Pop();
            value = eatBox(node, functionCallBox->FirstConnection());
            context.GraphStack.Push(graph);
        }
        else
        {
            // Use the default value from the function graph
            value = TryGetValue(node->TryGetBox(1), Value::Zero);
        }
        break;
    }
    default:
        break;
    }
}
