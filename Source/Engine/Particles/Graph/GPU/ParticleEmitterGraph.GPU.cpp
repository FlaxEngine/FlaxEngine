// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PARTICLE_GPU_GRAPH

#include "ParticleEmitterGraph.GPU.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Visject/ShaderGraphUtilities.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Core/Types/CommonValue.h"

/// <summary>
/// GPU particles shader source code template has special marks for generated code.
/// Each starts with '@' char and index of the mapped string.
/// </summary>
enum GPUParticlesTemplateInputsMapping
{
    In_VersionNumber = 0,
    In_Constants = 1,
    In_ShaderResources = 2,
    In_Defines = 3,
    In_Initialize = 4,
    In_Update = 5,
    In_Layout = 6,
    In_Includes = 7,

    In_MAX
};

void ParticleEmitterGraphGPU::ClearCache()
{
    for (int32 i = 0; i < Nodes.Count(); i++)
    {
        ParticleEmitterGraphGPUNode& node = Nodes.Get()[i];
        for (int32 j = 0; j < node.Boxes.Count(); j++)
        {
            node.Boxes.Get()[j].Cache.Clear();
        }
    }
}

ParticleEmitterGPUGenerator::ParticleEmitterGPUGenerator()
{
    // Register per group type processing events
    // Note: index must match group id
    _perGroupProcessCall[5].Bind<ParticleEmitterGPUGenerator, &ParticleEmitterGPUGenerator::ProcessGroupTextures>(this);
    _perGroupProcessCall[6].Bind<ParticleEmitterGPUGenerator, &ParticleEmitterGPUGenerator::ProcessGroupParameters>(this);
    _perGroupProcessCall[7].Bind<ParticleEmitterGPUGenerator, &ParticleEmitterGPUGenerator::ProcessGroupTools>(this);
    _perGroupProcessCall[14].Bind<ParticleEmitterGPUGenerator, &ParticleEmitterGPUGenerator::ProcessGroupParticles>(this);
    _perGroupProcessCall[16].Bind<ParticleEmitterGPUGenerator, &ParticleEmitterGPUGenerator::ProcessGroupFunction>(this);
}

ParticleEmitterGPUGenerator::~ParticleEmitterGPUGenerator()
{
    _graphs.ClearDelete();
    _functions.ClearDelete();
}

void ParticleEmitterGPUGenerator::AddGraph(ParticleEmitterGraphGPU* graph)
{
    _graphs.Add(graph);
}

bool ParticleEmitterGPUGenerator::Generate(WriteStream& source, BytesContainer& parametersData, int32& customDataSize)
{
    ASSERT_LOW_LAYER(_graphs.HasItems());

    String inputs[In_MAX];

    // Setup and prepare graphs
    _writer.Clear();
    _includes.Clear();
    _callStack.Clear();
    _parameters.Clear();
    _localIndex = 0;
    _customDataSize = 0;
    _graphStack.Clear();
    for (int32 i = 0; i < _graphs.Count(); i++)
    {
        PrepareGraph(_graphs[i]);
    }
    inputs[In_VersionNumber] = StringUtils::ToString(PARTICLE_GPU_GRAPH_VERSION);

    // Cache data
    ParticleEmitterGraphGPU* baseGraph = GetRootGraph();
    _graphStack.Push((Graph*)baseGraph);
    auto& layout = baseGraph->Layout;
    _attributeValues.Resize(layout.Attributes.Count());
    for (int32 i = 0; i < _attributeValues.Count(); i++)
        _attributeValues[i] = AttributeCache();
    _contextUsesKill = false;

    // Cache attributes
    const int32 positionIdx = layout.FindAttribute(StringView(TEXT("Position")), ParticleAttribute::ValueTypes::Float3);
    const int32 velocityIdx = layout.FindAttribute(StringView(TEXT("Velocity")), ParticleAttribute::ValueTypes::Float3);
    const int32 rotationIdx = layout.FindAttribute(StringView(TEXT("Rotation")), ParticleAttribute::ValueTypes::Float3);
    const int32 angularVelocityIdx = layout.FindAttribute(StringView(TEXT("AngularVelocity")), ParticleAttribute::ValueTypes::Float3);
    const int32 ageIdx = layout.FindAttribute(StringView(TEXT("Age")), ParticleAttribute::ValueTypes::Float);
    const int32 lifetimeIdx = layout.FindAttribute(StringView(TEXT("Lifetime")), ParticleAttribute::ValueTypes::Float);

    // Initialize spawned particles
    {
        _contextType = ParticleContextType::Initialize;

        // Initialize all attributes to zero (as local variable) and mark them for write to buffer
        for (int32 i = 0; i < layout.Attributes.Count(); i++)
        {
            AccessParticleAttribute(baseGraph->Root, i, AccessMode::Read);
            AccessParticleAttribute(baseGraph->Root, i, AccessMode::Write);
        }

        for (int32 i = 0; i < baseGraph->InitModules.Count(); i++)
        {
            ProcessModule(baseGraph->InitModules.Get()[i]);
        }

        WriteParticleAttributesWrites();

        inputs[In_Initialize] = _writer.ToString();

        _writer.Clear();
        clearCache();
    }

    // Update particles
    {
        _contextType = ParticleContextType::Update;

        // Read all particle attributes to preserve its value
        for (int32 i = 0; i < layout.Attributes.Count(); i++)
        {
            AccessParticleAttribute(baseGraph->Root, i, AccessMode::ReadWrite);
        }

        for (int32 i = 0; i < baseGraph->UpdateModules.Count(); i++)
        {
            ProcessModule(baseGraph->UpdateModules.Get()[i]);
        }

        // Dead particles removal
        if (ageIdx != -1 && lifetimeIdx != -1)
        {
            UseKill();

            const auto age = AccessParticleAttribute(baseGraph->Root, ageIdx, AccessMode::Read);
            const auto lifetime = AccessParticleAttribute(baseGraph->Root, lifetimeIdx, AccessMode::Read);
            _writer.Write(TEXT("\tkill = kill || ({0} >= {1});\n"), age.Value, lifetime.Value);
        }

        WriteReturnOnKill();

        // Euler integration
        if (positionIdx != -1 && velocityIdx != -1)
        {
            const auto position = AccessParticleAttribute(baseGraph->Root, positionIdx, AccessMode::ReadWrite);
            const auto velocity = AccessParticleAttribute(baseGraph->Root, velocityIdx, AccessMode::Read);
            _writer.Write(TEXT("\t{0} += {1} * DeltaTime;\n"), position.Value, velocity.Value);
        }

        // Angular Euler Integration
        if (rotationIdx != -1 && angularVelocityIdx != -1)
        {
            const auto rotation = AccessParticleAttribute(baseGraph->Root, rotationIdx, AccessMode::ReadWrite);
            const auto angularVelocity = AccessParticleAttribute(baseGraph->Root, angularVelocityIdx, AccessMode::Read);
            _writer.Write(TEXT("\t{0} += {1} * DeltaTime;\n"), rotation.Value, angularVelocity.Value);
        }

        _writer.Write(
            TEXT(
                "\t\n"
                "\tif (AddParticle(context.ParticleIndex))\n"
                "\t\treturn;\n"
            ));

        WriteParticleAttributesWrites();

        inputs[In_Update] = _writer.ToString();
        _writer.Clear();
        clearCache();
    }

    // Particle attributes layout
    {
        _writer.Write(TEXT("// Particle Attributes Layout\n"));
        _writer.Write(TEXT("// Offset -  Type  -  Name\n"));
        for (int32 i = 0; i < layout.Attributes.Count(); i++)
        {
            auto& a = layout.Attributes[i];
            const Char* typeName;
            switch (a.ValueType)
            {
            case ParticleAttribute::ValueTypes::Float:
                typeName = TEXT("float");
                break;
            case ParticleAttribute::ValueTypes::Float2:
                typeName = TEXT("float2");
                break;
            case ParticleAttribute::ValueTypes::Float3:
                typeName = TEXT("float3");
                break;
            case ParticleAttribute::ValueTypes::Float4:
                typeName = TEXT("float4");
                break;
            case ParticleAttribute::ValueTypes::Int:
                typeName = TEXT("int");
                break;
            case ParticleAttribute::ValueTypes::Uint:
                typeName = TEXT("uint");
                break;
            default:
                CRASH;
            }
            _writer.Write(TEXT("// {0:^6} | {1:^6} | {2}\n"), a.Offset, typeName, a.Name);
        }
        _writer.Write(TEXT("// Total particle size: {0} bytes\n"), layout.Size);
        _writer.Write(TEXT("// Total buffer size: {0} kB\n"), (layout.Size * baseGraph->Capacity + sizeof(uint32) + _customDataSize) / 1024);

        inputs[In_Layout] = _writer.ToString();
        _writer.Clear();
    }

    // Defines
    {
        _writer.Write(TEXT("#define PARTICLE_STRIDE {0}\n"), layout.Size);
        _writer.Write(TEXT("#define PARTICLE_CAPACITY {0}\n"), baseGraph->Capacity);
        _writer.Write(TEXT("#define PARTICLE_THRESHOLD 1e-6f\n"));

        inputs[In_Defines] = _writer.ToString();
        _writer.Clear();
    }

    // Includes
    {
        for (auto& include : _includes)
        {
            _writer.Write(TEXT("#include \"{0}\"\n"), include.Item);
        }
        inputs[In_Includes] = _writer.ToString();
        _writer.Clear();
    }

    // Check if graph is using any parameters
    if (_parameters.HasItems())
    {
        ShaderGraphUtilities::GenerateShaderConstantBuffer(_writer, _parameters);
        inputs[In_Constants] = _writer.ToString();
        _writer.Clear();

        const int32 startRegister = 1;
        const auto error = ShaderGraphUtilities::GenerateShaderResources(_writer, _parameters, startRegister);
        if (error)
        {
            OnError(nullptr, nullptr, error);
            return true;
        }
        inputs[In_ShaderResources] = _writer.ToString();
        _writer.Clear();

        MaterialParams::Save(parametersData, &_parameters);
    }
    else
    {
        parametersData.Release();
    }
    _parameters.Clear();

    // Set the custom data usage info
    customDataSize = _customDataSize;

    // Create source code
    {
        // Open template file
        const String path = Globals::EngineContentFolder / TEXT("Editor/MaterialTemplates/GPUParticles.shader");
        auto file = FileReadStream::Open(path);
        if (file == nullptr)
        {
            LOG(Warning, "Cannot load GPU particles simulation shader source code template.");
            return true;
        }

        // Format template
        const uint32 length = file->GetLength();
        Array<char> tmp;
        for (uint32 i = 0; i < length; i++)
        {
            const char c = file->ReadByte();

            if (c != '@')
            {
                source.WriteByte(c);
                continue;
            }

            i++;
            const int32 inIndex = file->ReadByte() - '0';
            ASSERT_LOW_LAYER(Math::IsInRange(inIndex, 0, In_MAX - 1));

            const String& in = inputs[inIndex];
            if (in.Length() > 0)
            {
                tmp.EnsureCapacity(in.Length() + 1, false);
                StringUtils::ConvertUTF162ANSI(*in, tmp.Get(), in.Length());
                source.WriteBytes(tmp.Get(), in.Length());
            }
        }

        // Close file
        Delete(file);

        // Ensure to have null-terminated source code
        source.WriteByte(0);
    }

    return
            false;
}

void ParticleEmitterGPUGenerator::clearCache()
{
    // Reset cached boxes values
    for (int32 i = 0; i < _graphs.Count(); i++)
    {
        _graphs.Get()[i]->ClearCache();
    }
    for (const auto& e : _functions)
    {
        auto& nodes = ((ParticleEmitterGraphGPU*)e.Value)->Nodes;
        for (int32 i = 0; i < nodes.Count(); i++)
        {
            ParticleEmitterGraphGPUNode& node = nodes.Get()[i];
            for (int32 j = 0; j < node.Boxes.Count(); j++)
                node.Boxes.Get()[j].Cache.Clear();
        }
    }

    // Reset cached attributes
    for (int32 i = 0; i < _attributeValues.Count(); i++)
        _attributeValues.Get()[i] = AttributeCache();

    _contextUsesKill = false;
}

void ParticleEmitterGPUGenerator::WriteParticleAttributesWrites()
{
    bool hadAnyWrite = false;
    ParticleEmitterGraphGPU* graph = GetRootGraph();
    for (int32 i = 0; i < _attributeValues.Count(); i++)
    {
        auto& value = _attributeValues[i];
        auto& attribute = graph->Layout.Attributes[i];

        // Skip not used attributes or read-only attributes
        if (value.Variable.Type == VariantType::Null || ((int)value.Access & (int)AccessMode::Write) == 0)
            continue;

        // Write comment
        if (!hadAnyWrite)
        {
            hadAnyWrite = true;
            _writer.Write(TEXT("\t\n\t// Write attributes\n"));
        }

        // Write to the attributes buffer
        const Char* format;
        switch (attribute.ValueType)
        {
        case ParticleAttribute::ValueTypes::Float:
            format = TEXT("\tSetParticleFloat(context.ParticleIndex, {0}, {1});\n");
            break;
        case ParticleAttribute::ValueTypes::Float2:
            format = TEXT("\tSetParticleVec2(context.ParticleIndex, {0}, {1});\n");
            break;
        case ParticleAttribute::ValueTypes::Float3:
            format = TEXT("\tSetParticleVec3(context.ParticleIndex, {0}, {1});\n");
            break;
        case ParticleAttribute::ValueTypes::Float4:
            format = TEXT("\tSetParticleVec4(context.ParticleIndex, {0}, {1});\n");
            break;
        case ParticleAttribute::ValueTypes::Int:
            format = TEXT("\tSetParticleInt(context.ParticleIndex, {0}, {1});\n");
            break;
        case ParticleAttribute::ValueTypes::Uint:
            format = TEXT("\tSetParticleUint(context.ParticleIndex, {0}, {1});\n");
            break;
        default:
            continue;
        }
        _writer.Write(format, attribute.Offset, value.Variable.Value);
    }
}

ParticleEmitterGPUGenerator::Parameter* ParticleEmitterGPUGenerator::findGraphParam(const Guid& id)
{
    Parameter* result = nullptr;
    for (int32 i = 0; i < _graphs.Count(); i++)
    {
        result = (Parameter*)_graphs[i]->GetParameter(id);
        if (result)
            break;
    }
    return result;
}

void ParticleEmitterGPUGenerator::PrepareGraph(ParticleEmitterGraphGPU* graph)
{
    graph->ClearCache();

    SerializedMaterialParam mp;

    // Add all parameters to be saved in the result parameters collection (perform merge)
    for (int32 j = 0; j < graph->Parameters.Count(); j++)
    {
        const auto param = &graph->Parameters[j];

        SerializedMaterialParam& mp = _parameters.AddOne();
        mp.ID = param->Identifier;
        mp.IsPublic = param->IsPublic;
        mp.Override = true;
        mp.Name = param->Name;
        mp.ShaderName = getParamName(_parameters.Count());
        mp.Type = MaterialParameterType::Bool;
        mp.AsBool = false;
        switch (param->Type.Type)
        {
        case VariantType::Bool:
            mp.Type = MaterialParameterType::Bool;
            mp.AsBool = param->Value.AsBool;
            break;
        case VariantType::Int:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = param->Value.AsInt;
            break;
        case VariantType::Enum:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.MaterialSceneTextures") == 0)
                mp.Type = MaterialParameterType::SceneTexture;
            else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.ChannelMask") == 0)
                mp.Type = MaterialParameterType::ChannelMask;
            else
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
            mp.AsInteger = (int32)param->Value.AsUint64;
            break;
        case VariantType::Float:
            mp.Type = MaterialParameterType::Float;
            mp.AsFloat = param->Value.AsFloat;
            break;
        case VariantType::Float2:
            mp.Type = MaterialParameterType::Vector2;
            mp.AsFloat2 = param->Value.AsFloat2();
            break;
        case VariantType::Float3:
            mp.Type = MaterialParameterType::Vector3;
            mp.AsFloat3 = param->Value.AsFloat3();
            break;
        case VariantType::Float4:
        case VariantType::Quaternion:
            mp.Type = MaterialParameterType::Vector4;
            *(Float4*)&mp.AsData = param->Value.AsFloat4();
            break;
        case VariantType::Double2:
            mp.Type = MaterialParameterType::Float;
            mp.AsFloat2 = (Float2)param->Value.AsDouble2();
            break;
        case VariantType::Double3:
            mp.Type = MaterialParameterType::Vector3;
            mp.AsFloat3 = (Float3)param->Value.AsDouble3();
            break;
        case VariantType::Double4:
            mp.Type = MaterialParameterType::Vector4;
            *(Float4*)&mp.AsData = (Float4)param->Value.AsDouble4();
            break;
        case VariantType::Color:
            mp.Type = MaterialParameterType::Color;
            mp.AsColor = param->Value.AsColor();
            break;
        case VariantType::Asset:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.Texture") == 0)
                mp.Type = MaterialParameterType::Texture;
            else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.CubeTexture") == 0)
                mp.Type = MaterialParameterType::CubeTexture;
            else
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
            mp.AsGuid = (Guid)param->Value;
            break;
        case VariantType::Object:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.GPUTexture") == 0)
                mp.Type = MaterialParameterType::GPUTexture;
            else
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
            mp.AsGuid = (Guid)param->Value;
            break;
        case VariantType::Matrix:
            mp.Type = MaterialParameterType::Matrix;
            *(Matrix*)&mp.AsData = (Matrix)param->Value;
            break;
        default:
            OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported particle parameter type {0}."), param->Type));
            break;
        }
    }
}

void ParticleEmitterGPUGenerator::UseKill()
{
    if (!_contextUsesKill)
    {
        _contextUsesKill = true;
        _writer.Write(TEXT("\tbool kill = false;\n"));
    }
}

void ParticleEmitterGPUGenerator::WriteReturnOnKill()
{
    if (_contextUsesKill)
    {
        _contextUsesKill = false;
        _writer.Write(TEXT("\tif (kill)\n\t\treturn;\n"));
    }
}

#endif
