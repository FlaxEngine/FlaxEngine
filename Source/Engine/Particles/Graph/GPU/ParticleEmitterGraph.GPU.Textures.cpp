// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PARTICLE_GPU_GRAPH

#include "ParticleEmitterGraph.GPU.h"
#include "Engine/Graphics/Materials/MaterialInfo.h"

bool ParticleEmitterGPUGenerator::loadTexture(Node* caller, Box* box, const SerializedMaterialParam& texture, Value& result)
{
    ASSERT(caller && box && texture.ID.IsValid());

    // Cache data
    auto parent = box->GetParent<Node>();
    const bool isCubemap = texture.Type == MaterialParameterType::CubeTexture;
    const bool isVolume = texture.Type == MaterialParameterType::GPUTextureVolume;
    const bool isArray = texture.Type == MaterialParameterType::GPUTextureArray;

    // Check if has variable assigned and it's a valid type
    if (texture.Type != MaterialParameterType::Texture
        && texture.Type != MaterialParameterType::SceneTexture
        && texture.Type != MaterialParameterType::GPUTexture
        && texture.Type != MaterialParameterType::GPUTextureVolume
        && texture.Type != MaterialParameterType::GPUTextureCube
        && texture.Type != MaterialParameterType::GPUTextureArray
        && texture.Type != MaterialParameterType::CubeTexture)
    {
        result = Value::Zero;
        OnError(caller, box, TEXT("No parameter for texture load or invalid type."));
        return true;
    }

    // Get the location to load
    Box* locationBox = parent->GetBox(2);
    Value location = tryGetValue(locationBox, Value::InitForZero(VariantType::Float2));

    // Convert into a proper type
    if (isVolume || isArray)
        location = Value::Cast(location, VariantType::Float4);
    else
        location = Value::Cast(location, VariantType::Float3);

    // Load texture
    const Char* format = TEXT("{0}.Load({1})");
    const String sampledValue = String::Format(format, texture.ShaderName, location.Value);
    result = writeLocal(VariantType::Float4, sampledValue, parent);

    return false;
}

bool ParticleEmitterGPUGenerator::sampleSceneTexture(Node* caller, Box* box, const SerializedMaterialParam& texture, Value& result)
{
    ASSERT(caller && box && texture.ID.IsValid());

    // Cache data
    auto parent = box->GetParent<Node>();
    const bool isCubemap = texture.Type == MaterialParameterType::CubeTexture;
    const bool isVolume = texture.Type == MaterialParameterType::GPUTextureVolume;
    const bool isArray = texture.Type == MaterialParameterType::GPUTextureArray;

    // Check if has variable assigned and it's a valid type
    if (texture.Type != MaterialParameterType::Texture
        && texture.Type != MaterialParameterType::SceneTexture
        && texture.Type != MaterialParameterType::GPUTexture
        && texture.Type != MaterialParameterType::GPUTextureVolume
        && texture.Type != MaterialParameterType::GPUTextureCube
        && texture.Type != MaterialParameterType::GPUTextureArray
        && texture.Type != MaterialParameterType::CubeTexture)
    {
        result = Value::Zero;
        OnError(caller, box, TEXT("No parameter for texture load or invalid type."));
        return true;
    }

    // Check if return the texture reference
    if (box->ID == 6)
    {
        result = Value(VariantType::Object, texture.ShaderName);
        return false;
    }

    Box* valueBox = parent->GetBox(1);
    if (valueBox->Cache.IsInvalid())
    {
        // Get the UVs to sample
        Box* uvsBox = parent->GetBox(0);
        Value uvs = tryGetValue(uvsBox, Value::InitForZero(VariantType::Float2));

        // Convert into a proper type
        uvs = Value::Cast(uvs, VariantType::Float2);

        // Load texture
        const Char* format = TEXT("{0}.Load(uint3({1} * ScreenSize.xy, 0))");
        const String sampledValue = String::Format(format, texture.ShaderName, uvs.Value);
        valueBox->Cache = writeLocal(VariantType::Float4, sampledValue, parent);
    }

    // Check if reuse cached value
    if (valueBox->Cache.IsValid())
    {
        result = valueBox->Cache;
        return false;
    }

    // Set result values based on box ID
    switch (box->ID)
    {
    case 1:
        result = valueBox->Cache;
        break;
    case 2:
        result = Value(VariantType::Float, valueBox->Cache.Value + _subs[0]);
        break;
    case 3:
        result = Value(VariantType::Float, valueBox->Cache.Value + _subs[1]);
        break;
    case 4:
        result = Value(VariantType::Float, valueBox->Cache.Value + _subs[2]);
        break;
    case 5:
        result = Value(VariantType::Float, valueBox->Cache.Value + _subs[3]);
        break;
    default:
        CRASH;
        break;
    }

    return false;
}

void ParticleEmitterGPUGenerator::sampleSceneDepth(Node* caller, Value& value, Box* box)
{
    // Sample depth buffer
    const auto param = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
    Value depthSample;
    if (sampleSceneTexture(caller, box, param, depthSample))
    {
        value = Value::Zero;
        return;
    }

    // Linearize raw device depth
    linearizeSceneDepth(caller, depthSample, value);
}

void ParticleEmitterGPUGenerator::linearizeSceneDepth(Node* caller, const Value& depth, Value& value)
{
    value = writeLocal(VariantType::Float, String::Format(TEXT("ViewInfo.w / ({0}.x - ViewInfo.z)"), depth.Value), caller);
}

void ParticleEmitterGPUGenerator::ProcessGroupTextures(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Scene Texture
    case 6:
    {
        // Get texture type
        auto type = (MaterialSceneTextures)node->Values[0].AsInt;

        // Some types need more logic
        switch (type)
        {
        case MaterialSceneTextures::SceneDepth:
        {
            sampleSceneDepth(node, value, box);
            break;
        }
        case MaterialSceneTextures::DiffuseColor:
        {
            auto gBuffer0Param = findOrAddSceneTexture(MaterialSceneTextures::BaseColor);
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Metalness);
            Value gBuffer0Sample;
            if (sampleSceneTexture(node, box, gBuffer0Param, gBuffer0Sample))
                break;
            Value gBuffer2Sample;
            if (sampleSceneTexture(node, box, gBuffer2Param, gBuffer2Sample))
                break;
            value = writeLocal(VariantType::Float3, String::Format(TEXT("GetDiffuseColor({0}.rgb, {1}.g)"), gBuffer0Sample.Value, gBuffer2Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::SpecularColor:
        {
            auto gBuffer0Param = findOrAddSceneTexture(MaterialSceneTextures::BaseColor);
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Metalness);
            Value gBuffer0Sample;
            if (sampleSceneTexture(node, box, gBuffer0Param, gBuffer0Sample))
                break;
            Value gBuffer2Sample;
            if (sampleSceneTexture(node, box, gBuffer2Param, gBuffer2Sample))
                break;
            value = writeLocal(VariantType::Float3, String::Format(TEXT("GetSpecularColor({0}.rgb, {1}.b, {1}.g)"), gBuffer0Sample.Value, gBuffer2Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::WorldNormal:
        {
            auto gBuffer1Param = findOrAddSceneTexture(MaterialSceneTextures::WorldNormal);
            Value gBuffer1Sample;
            if (sampleSceneTexture(node, box, gBuffer1Param, gBuffer1Sample))
                break;
            value = writeLocal(VariantType::Float3, String::Format(TEXT("DecodeNormal({0}.rgb)"), gBuffer1Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::AmbientOcclusion:
        {
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::AmbientOcclusion);
            Value gBuffer2Sample;
            if (sampleSceneTexture(node, box, gBuffer2Param, gBuffer2Sample))
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.a"), gBuffer2Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::Metalness:
        {
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Metalness);
            Value gBuffer2Sample;
            if (sampleSceneTexture(node, box, gBuffer2Param, gBuffer2Sample))
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.g"), gBuffer2Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::Roughness:
        {
            auto gBuffer0Param = findOrAddSceneTexture(MaterialSceneTextures::Roughness);
            Value gBuffer0Sample;
            if (sampleSceneTexture(node, box, gBuffer0Param, gBuffer0Sample))
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.r"), gBuffer0Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::Specular:
        {
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Specular);
            Value gBuffer2Sample;
            if (sampleSceneTexture(node, box, gBuffer2Param, gBuffer2Sample))
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.b"), gBuffer2Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::ShadingModel:
        {
            auto gBuffer1Param = findOrAddSceneTexture(MaterialSceneTextures::WorldNormal);
            Value gBuffer1Sample;
            if (sampleSceneTexture(node, box, gBuffer1Param, gBuffer1Sample))
                break;
            value = writeLocal(VariantType::Int, String::Format(TEXT("(int)({0}.a * 3.999)"), gBuffer1Sample.Value), node);
            break;
        }
        case MaterialSceneTextures::WorldPosition:
            value = Value::Zero; // Not implemented
            break;
        default:
        {
            // Sample single texture
            auto param = findOrAddSceneTexture(type);
            sampleSceneTexture(node, box, param, value);
            break;
        }
        }
        break;
    }
    // Scene Depth
    case 8:
        sampleSceneDepth(node, value, box);
        break;
    // Texture
    case 11:
    {
        // Check if texture has been selected
        Guid textureId = (Guid)node->Values[0];
        if (textureId.IsValid())
        {
            auto param = findOrAddTexture(textureId);
            value = Value(VariantType::Object, param.ShaderName);
        }
        else
        {
            // Use default value
            value = Value::Zero;
        }
        break;
    }
    // Load Texture
    case 13:
    {
        // Get input texture
        auto textureBox = node->GetBox(1);
        if (!textureBox->HasConnection())
        {
            // No texture to load
            value = Value::Zero;
            break;
        }
        const auto texture = eatBox(textureBox->GetParent<Node>(), textureBox->FirstConnection());
        const auto textureParam = findParam(texture.Value);
        if (!textureParam)
        {
            // Missing texture
            value = Value::Zero;
            break;
        }

        // Copy data on stack to prevent issues when changing the parameters array (gathered pointer is weak)
        const auto copy = *textureParam;

        // Load texture
        loadTexture(node, box, copy, value);
        break;
    }
    // Sample Global SDF
    case 14:
    {
        auto param = findOrAddGlobalSDF();
        Value worldPosition = tryGetValue(node->GetBox(1), Value(VariantType::Float3, TEXT("input.WorldPosition.xyz"))).Cast(VariantType::Float3);
        Value startCascade = tryGetValue(node->TryGetBox(2), 0, Value::Zero).Cast(VariantType::Uint);
        value = writeLocal(VariantType::Float, String::Format(TEXT("SampleGlobalSDF({0}, {0}_Tex, {0}_Mip, {1}, {2})"), param.ShaderName, worldPosition.Value, startCascade.Value), node);
        _includes.Add(TEXT("./Flax/GlobalSignDistanceField.hlsl"));
        break;
    }
    // Sample Global SDF Gradient
    case 15:
    {
        auto gradientBox = node->GetBox(0);
        auto distanceBox = node->GetBox(2);
        auto param = findOrAddGlobalSDF();
        Value worldPosition = tryGetValue(node->GetBox(1), Value(VariantType::Float3, TEXT("input.WorldPosition.xyz"))).Cast(VariantType::Float3);
        Value startCascade = tryGetValue(node->TryGetBox(3), 0, Value::Zero).Cast(VariantType::Uint);
        auto distance = writeLocal(VariantType::Float, node);
        auto gradient = writeLocal(VariantType::Float3, String::Format(TEXT("SampleGlobalSDFGradient({0}, {0}_Tex, {0}_Mip, {1}, {2}, {3})"), param.ShaderName, worldPosition.Value, distance.Value, startCascade.Value), node);
        _includes.Add(TEXT("./Flax/GlobalSignDistanceField.hlsl"));
        gradientBox->Cache = gradient;
        distanceBox->Cache = distance;
        value = box == gradientBox ? gradient : distance;
        break;
    }
    // Texture Size
    case 24:
    {
        value = Value::Zero;
        auto textureBox = node->GetBox(0);
        if (!textureBox->HasConnection())
            break;
        const auto texture = eatBox(textureBox->GetParent<Node>(), textureBox->FirstConnection());
        const auto textureParam = findParam(texture.Value);
        if (!textureParam)
            break;
        value = writeLocal(VariantType::Float2, node);
        _writer.Write(TEXT("\t{0}.GetDimensions({1}.x, {1}.y);\n"), textureParam->ShaderName, value.Value);
        break;
    }
    default:
        break;
    }
}

#endif
