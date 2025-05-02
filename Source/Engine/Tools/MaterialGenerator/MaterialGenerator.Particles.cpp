// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"

MaterialValue MaterialGenerator::AccessParticleAttribute(Node* caller, const StringView& name, ParticleAttributeValueTypes valueType, const Char* index, ParticleAttributeSpace space)
{
    // TODO: cache the attribute value during material tree execution (eg. reuse the same Particle Color read for many nodes in graph)

    String mappingName = TEXT("Particle.");
    mappingName += name;
    SerializedMaterialParam* attributeMapping = nullptr;

    // Find if this attribute has been already accessed
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::Integer && param.Name == mappingName)
        {
            // Reuse attribute mapping
            attributeMapping = &param;
            break;
        }
    }
    if (!attributeMapping)
    {
        // Create
        SerializedMaterialParam& param = _parameters.AddOne();
        param.Type = MaterialParameterType::Integer;
        param.IsPublic = false;
        param.Override = true;
        param.Name = mappingName;
        param.ShaderName = getParamName(_parameters.Count());
        param.AsInteger = 0;
        param.ID = Guid::New();
        attributeMapping = &param;
    }

    // Read particle data from the buffer
    VariantType::Types type;
    const Char* format;
    switch (valueType)
    {
    case ParticleAttributeValueTypes::Float:
        type = VariantType::Float;
        format = TEXT("GetParticleFloat({1}, {0})");
        break;
    case ParticleAttributeValueTypes::Float2:
        type = VariantType::Float2;
        format = TEXT("GetParticleVec2({1}, {0})");
        break;
    case ParticleAttributeValueTypes::Float3:
        type = VariantType::Float3;
        format = TEXT("GetParticleVec3({1}, {0})");
        break;
    case ParticleAttributeValueTypes::Float4:
        type = VariantType::Float4;
        format = TEXT("GetParticleVec4({1}, {0})");
        break;
    case ParticleAttributeValueTypes::Int:
        type = VariantType::Int;
        format = TEXT("GetParticleInt({1}, {0})");
        break;
    case ParticleAttributeValueTypes::Uint:
        type = VariantType::Uint;
        format = TEXT("GetParticleUint({1}, {0})");
        break;
    default:
        return MaterialValue::Zero;
    }
    auto result = writeLocal(type, String::Format(format, attributeMapping->ShaderName, index ? index : TEXT("input.ParticleIndex")), caller);

    // Apply transformation to world-space
    switch (space)
    {
    case ParticleAttributeSpace::AsIs:
        break;
    case ParticleAttributeSpace::LocalPosition:
        _writer.Write(TEXT("\t{0} = TransformParticlePosition({0});\n"), result.Value);
        break;
    case ParticleAttributeSpace::LocalDirection:
        _writer.Write(TEXT("\t{0} = TransformParticleVector({0});\n"), result.Value);
        break;
    default: ;
    }

    return result;
}

void MaterialGenerator::ProcessGroupParticles(Box* box, Node* node, Value& value)
{
    // Only particle shaders can access particles data
    if (GetRootLayer()->Domain != MaterialDomain::Particle && GetRootLayer()->Domain != MaterialDomain::VolumeParticle)
    {
        value = MaterialValue::Zero;
        return;
    }

    switch (node->TypeID)
    {
    // Particle Attribute
    case 100:
    {
        value = AccessParticleAttribute(node, (StringView)node->Values[0], static_cast<ParticleAttributeValueTypes>(node->Values[1].AsInt));
        break;
    }
    // Particle Attribute (by index)
    case 303:
    {
        const auto particleIndex = Value::Cast(tryGetValue(node->GetBox(1), Value(VariantType::Uint, TEXT("input.ParticleIndex"))), VariantType::Uint);
        value = AccessParticleAttribute(node, (StringView)node->Values[0], static_cast<ParticleAttributeValueTypes>(node->Values[1].AsInt), particleIndex.Value.Get());
        break;
    }
    // Particle Position
    case 101:
    {
        value = AccessParticleAttribute(node, TEXT("Position"), ParticleAttributeValueTypes::Float3, nullptr, ParticleAttributeSpace::LocalPosition);
        break;
    }
    // Particle Lifetime
    case 102:
    {
        value = AccessParticleAttribute(node, TEXT("Lifetime"), ParticleAttributeValueTypes::Float);
        break;
    }
    // Particle Age
    case 103:
    {
        value = AccessParticleAttribute(node, TEXT("Age"), ParticleAttributeValueTypes::Float);
        break;
    }
    // Particle Color
    case 104:
    {
        value = AccessParticleAttribute(node, TEXT("Color"), ParticleAttributeValueTypes::Float4);
        break;
    }
    // Particle Velocity
    case 105:
    {
        value = AccessParticleAttribute(node, TEXT("Velocity"), ParticleAttributeValueTypes::Float3, nullptr, ParticleAttributeSpace::LocalDirection);
        break;
    }
    // Particle Sprite Size
    case 106:
    {
        value = AccessParticleAttribute(node, TEXT("SpriteSize"), ParticleAttributeValueTypes::Float2);
        break;
    }
    // Particle Mass
    case 107:
    {
        value = AccessParticleAttribute(node, TEXT("Mass"), ParticleAttributeValueTypes::Float);
        break;
    }
    // Particle Rotation
    case 108:
    {
        value = AccessParticleAttribute(node, TEXT("Rotation"), ParticleAttributeValueTypes::Float3);
        break;
    }
    // Particle Angular Velocity
    case 109:
    {
        value = AccessParticleAttribute(node, TEXT("AngularVelocity"), ParticleAttributeValueTypes::Float3);
        break;
    }
    // Particle Normalized Age
    case 110:
    {
        const auto age = AccessParticleAttribute(node, TEXT("Age"), ParticleAttributeValueTypes::Float);
        const auto lifetime = AccessParticleAttribute(node, TEXT("Lifetime"), ParticleAttributeValueTypes::Float);
        value = writeOperation2(node, age, lifetime, '/');
        break;
    }
    // Particle Radius
    case 111:
    {
        value = AccessParticleAttribute(node, TEXT("Radius"), ParticleAttributeValueTypes::Float);
        break;
    }
    default:
        break;
    }
}

#endif
