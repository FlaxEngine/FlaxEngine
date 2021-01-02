// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"

void MaterialGenerator::ProcessGroupTextures(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Texture
    case 1:
    {
        // Check if texture has been selected
        Guid textureId = (Guid)node->Values[0];
        if (textureId.IsValid())
        {
            // Get or create parameter for that texture
            auto param = findOrAddTexture(textureId);

            // Sample texture
            sampleTexture(node, value, box, &param);
        }
        else
        {
            // Use default value
            value = Value::Zero;
        }
        break;
    }
        // TexCoord
    case 2:
        value = getUVs;
        break;
        // Cube Texture
    case 3:
    {
        // Check if texture has been selected
        Guid textureId = (Guid)node->Values[0];
        if (textureId.IsValid())
        {
            // Get or create parameter for that cube texture
            auto param = findOrAddCubeTexture(textureId);

            // Sample texture
            sampleTexture(node, value, box, &param);
        }
        else
        {
            // Use default value
            value = Value::Zero;
        }
        break;
    }
        // Normal Map
    case 4:
    {
        // Check if texture has been selected
        Guid textureId = (Guid)node->Values[0];
        if (textureId.IsValid())
        {
            // Get or create parameter for that texture
            auto param = findOrAddNormalMap(textureId);

            // Sample texture
            sampleTexture(node, value, box, &param);
        }
        else
        {
            // Use default value
            value = Value::Zero;
        }
        break;
    }
        // Parallax Occlusion Mapping
    case 5:
    {
        auto heightTextureBox = node->GetBox(4);
        if (!heightTextureBox->HasConnection())
        {
            value = Value::Zero;
            // TODO: handle missing texture error
            //OnError("No Variable Entry for height texture.", node);
            break;
        }
        auto heightTexture = eatBox(heightTextureBox->GetParent<Node>(), heightTextureBox->FirstConnection());
        if (heightTexture.Type != VariantType::Object)
        {
            value = Value::Zero;
            // TODO: handle invalid connection data error
            //OnError("No Variable Entry for height texture.", node);
            break;
        }
        Value uvs = tryGetValue(node->GetBox(0), getUVs).AsVector2();
        if (_treeType != MaterialTreeType::PixelShader)
        {
            // Required ddx/ddy instructions are only supported in Pixel Shader
            value = uvs;
            break;
        }
        Value scale = tryGetValue(node->GetBox(1), node->Values[0]);
        Value minSteps = tryGetValue(node->GetBox(2), node->Values[1]);
        Value maxSteps = tryGetValue(node->GetBox(3), node->Values[2]);
        Value result = writeLocal(VariantType::Vector2, uvs.Value, node);
        createGradients(node);
        ASSERT(node->Values[3].Type == VariantType::Int && Math::IsInRange(node->Values[3].AsInt, 0, 3));
        auto channel = _subs[node->Values[3].AsInt];
        Value cameraVectorWS = getCameraVector(node);
        Value cameraVectorTS = writeLocal(VariantType::Vector3, String::Format(TEXT("TransformWorldVectorToTangent(input, {0})"), cameraVectorWS.Value), node);
        auto code = String::Format(TEXT(
            "	{{\n"
            "	float vLength = length({8}.rg);\n"
            "	float coeff0 = vLength / {8}.b;\n"
            "	float coeff1 = coeff0 * (-({4}));\n"
            "	float2 vNorm = {8}.rg / vLength;\n"
            "	float2 maxOffset = (vNorm * coeff1);\n"

            "	float numSamples = lerp({0}, {3}, saturate(dot({9}, input.TBN[2])));\n"
            "	float stepSize = 1.0 / numSamples;\n"

            "	float2 currOffset = 0;\n"
            "	float2 lastOffset = 0;\n"
            "	float currRayHeight = 1.0;\n"
            "	float lastSampledHeight = 1;\n"
            "	int currSample = 0;\n"

            "	while (currSample < (int)numSamples)\n"
            "	{{\n"
            "		float currSampledHeight = {1}.SampleGrad(SamplerLinearWrap, {10} + currOffset, {5}, {6}){7};\n"

            "		if (currSampledHeight > currRayHeight)\n"
            "		{{\n"
            "			float delta1 = currSampledHeight - currRayHeight;\n"
            "			float delta2 = (currRayHeight + stepSize) - lastSampledHeight;\n"
            "			float ratio = delta1 / max(delta1 + delta2, 0.00001f);\n"
            "			currOffset = ratio * lastOffset + (1.0 - ratio) * currOffset;\n"
            "			break;\n"
            "		}}\n"

            "		currRayHeight -= stepSize;\n"
            "		lastOffset = currOffset;\n"
            "		currOffset += stepSize * maxOffset;\n"
            "		lastSampledHeight = currSampledHeight;\n"
            "		currSample++;\n"
            "	}}\n"

            "	{2} = {10} + currOffset;\n"
            "	}}\n"
        ),
                                   minSteps.Value, // {0}
                                   heightTexture.Value, // {1}
                                   result.Value, // {2}
                                   maxSteps.Value, // {3}
                                   scale.Value, // {4}
                                   _ddx.Value, // {5}
                                   _ddy.Value, // {6}
                                   channel, // {7}
                                   cameraVectorTS.Value, // {8}
                                   cameraVectorWS.Value, // {9}
                                   uvs.Value // {10}   
        );
        _writer.Write(*code);
        value = result;
        break;
    }
        // Scene Texture
    case 6:
    {
        // Get texture type
        auto type = (MaterialSceneTextures)(int32)node->Values[0];

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
            auto gBuffer0Sample = sampleTextureRaw(node, value, box, &gBuffer0Param);
            if (gBuffer0Sample == nullptr)
                break;
            auto gBuffer2Sample = sampleTextureRaw(node, value, box, &gBuffer2Param);
            if (gBuffer2Sample == nullptr)
                break;
            value = writeLocal(VariantType::Vector3, String::Format(TEXT("GetDiffuseColor({0}.rgb, {1}.g)"), gBuffer0Sample->Value, gBuffer2Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::SpecularColor:
        {
            auto gBuffer0Param = findOrAddSceneTexture(MaterialSceneTextures::BaseColor);
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Metalness);
            auto gBuffer0Sample = sampleTextureRaw(node, value, box, &gBuffer0Param);
            if (gBuffer0Sample == nullptr)
                break;
            auto gBuffer2Sample = sampleTextureRaw(node, value, box, &gBuffer2Param);
            if (gBuffer2Sample == nullptr)
                break;
            value = writeLocal(VariantType::Vector3, String::Format(TEXT("GetSpecularColor({0}.rgb, {1}.b, {1}.g)"), gBuffer0Sample->Value, gBuffer2Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::WorldNormal:
        {
            auto gBuffer1Param = findOrAddSceneTexture(MaterialSceneTextures::WorldNormal);
            auto gBuffer1Sample = sampleTextureRaw(node, value, box, &gBuffer1Param);
            if (gBuffer1Sample == nullptr)
                break;
            value = writeLocal(VariantType::Vector3, String::Format(TEXT("DecodeNormal({0}.rgb)"), gBuffer1Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::AmbientOcclusion:
        {
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::AmbientOcclusion);
            auto gBuffer2Sample = sampleTextureRaw(node, value, box, &gBuffer2Param);
            if (gBuffer2Sample == nullptr)
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.a"), gBuffer2Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::Metalness:
        {
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Metalness);
            auto gBuffer2Sample = sampleTextureRaw(node, value, box, &gBuffer2Param);
            if (gBuffer2Sample == nullptr)
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.g"), gBuffer2Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::Roughness:
        {
            auto gBuffer0Param = findOrAddSceneTexture(MaterialSceneTextures::Roughness);
            auto gBuffer0Sample = sampleTextureRaw(node, value, box, &gBuffer0Param);
            if (gBuffer0Sample == nullptr)
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.r"), gBuffer0Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::Specular:
        {
            auto gBuffer2Param = findOrAddSceneTexture(MaterialSceneTextures::Specular);
            auto gBuffer2Sample = sampleTextureRaw(node, value, box, &gBuffer2Param);
            if (gBuffer2Sample == nullptr)
                break;
            value = writeLocal(VariantType::Float, String::Format(TEXT("{0}.b"), gBuffer2Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::ShadingModel:
        {
            auto gBuffer1Param = findOrAddSceneTexture(MaterialSceneTextures::WorldNormal);
            auto gBuffer1Sample = sampleTextureRaw(node, value, box, &gBuffer1Param);
            if (gBuffer1Sample == nullptr)
                break;
            value = writeLocal(VariantType::Int, String::Format(TEXT("(int)({0}.a * 3.999)"), gBuffer1Sample->Value), node);
            break;
        }
        default:
        {
            // Sample single texture
            auto param = findOrAddSceneTexture(type);
            sampleTexture(node, value, box, &param);
            break;
        }
        }
        break;
    }
        // Scene Color
    case 7:
    {
        // Sample scene color texture
        auto param = findOrAddSceneTexture(MaterialSceneTextures::SceneColor);
        sampleTexture(node, value, box, &param);
        break;
    }
        // Scene Depth
    case 8:
    {
        sampleSceneDepth(node, value, box);
        break;
    }
        // Sample Texture
    case 9:
    {
        enum CommonSamplerType
        {
            LinearClamp = 0,
            PointClamp = 1,
            LinearWrap = 2,
            PointWrap = 3,
        };
        const Char* SamplerNames[]
        {
            TEXT("SamplerLinearClamp"),
            TEXT("SamplerPointClamp"),
            TEXT("SamplerLinearWrap"),
            TEXT("SamplerPointWrap"),
        };

        // Get input boxes
        auto textureBox = node->GetBox(0);
        auto uvsBox = node->GetBox(1);
        auto levelBox = node->GetBox(2);
        auto offsetBox = node->GetBox(3);
        if (!textureBox->HasConnection())
        {
            // No texture to sample
            value = Value::Zero;
            break;
        }
        const bool canUseSample = CanUseSample(_treeType);
        const auto texture = eatBox(textureBox->GetParent<Node>(), textureBox->FirstConnection());

        // Get UVs
        Value uvs;
        const bool useCustomUVs = uvsBox->HasConnection();
        if (useCustomUVs)
        {
            // Get custom UVs
            uvs = eatBox(uvsBox->GetParent<Node>(), uvsBox->FirstConnection());
        }
        else
        {
            // Use default UVs
            uvs = getUVs;
        }
        const auto textureParam = findParam(texture.Value);
        if (!textureParam)
        {
            // Missing texture
            value = Value::Zero;
            break;
        }
        const bool isCubemap = textureParam->Type == MaterialParameterType::CubeTexture || textureParam->Type == MaterialParameterType::GPUTextureCube;
        const bool isArray = textureParam->Type == MaterialParameterType::GPUTextureArray;
        const bool isVolume = textureParam->Type == MaterialParameterType::GPUTextureVolume;
        const bool isNormalMap = textureParam->Type == MaterialParameterType::NormalMap;
        const bool use3dUVs = isCubemap || isArray || isVolume;
        uvs = Value::Cast(uvs, use3dUVs ? VariantType::Vector3 : VariantType::Vector2);

        // Get other inputs
        const auto level = tryGetValue(levelBox, node->Values[1]);
        const bool useLevel = levelBox->HasConnection() || (int32)node->Values[1] != -1;
        const bool useOffset = offsetBox->HasConnection();
        const auto offset = useOffset ? eatBox(offsetBox->GetParent<Node>(), offsetBox->FirstConnection()) : Value::Zero;
        const int32 samplerIndex = node->Values[0].AsInt;
        if (samplerIndex < 0 || samplerIndex >= ARRAY_COUNT(SamplerNames))
        {
            OnError(node, box, TEXT("Invalid texture sampler."));
            return;
        }

        // Pick a property format string
        const Char* format;
        if (useLevel || !canUseSample)
        {
            if (useOffset)
                format = TEXT("{0}.SampleLevel({1}, {2}, {3}, {4})");
            else
                format = TEXT("{0}.SampleLevel({1}, {2}, {3})");
        }
        else
        {
            if (useOffset)
                format = TEXT("{0}.Sample({1}, {2}, {4})");
            else
                format = TEXT("{0}.Sample({1}, {2})");
        }

        // Sample texture
        const String sampledValue = String::Format(format,
                                                   texture.Value, // {0}
                                                   SamplerNames[samplerIndex], // {1}
                                                   uvs.Value, // {2}
                                                   level.Value, // {3}
                                                   offset.Value // {4}
        );
        textureBox->Cache = writeLocal(VariantType::Vector4, sampledValue, node);

        // Decode normal map vector
        if (isNormalMap)
        {
            // TODO: maybe we could use helper function for UnpackNormalTexture() and unify unpacking?
            _writer.Write(TEXT("\t{0}.xy = {0}.xy * 2.0 - 1.0;\n"), textureBox->Cache.Value);
            _writer.Write(TEXT("\t{0}.z = sqrt(saturate(1.0 - dot({0}.xy, {0}.xy)));\n"), textureBox->Cache.Value);
        }

        value = textureBox->Cache;
        break;
    }
        // Flipbook
    case 10:
    {
        // Get input values
        auto uv = Value::Cast(tryGetValue(node->GetBox(0), getUVs), VariantType::Vector2);
        auto frame = Value::Cast(tryGetValue(node->GetBox(1), node->Values[0]), VariantType::Float);
        auto framesXY = Value::Cast(tryGetValue(node->GetBox(2), node->Values[1]), VariantType::Vector2);
        auto invertX = Value::Cast(tryGetValue(node->GetBox(3), node->Values[2]), VariantType::Float);
        auto invertY = Value::Cast(tryGetValue(node->GetBox(4), node->Values[3]), VariantType::Float);

        // Write operations
        auto framesCount = writeLocal(VariantType::Float, String::Format(TEXT("{0}.x * {1}.y"), framesXY.Value, framesXY.Value), node);
        frame = writeLocal(VariantType::Float, String::Format(TEXT("fmod(floor({0}), {1})"), frame.Value, framesCount.Value), node);
        auto framesXYInv = writeOperation2(node, Value::One, framesXY, '/');
        auto frameY = writeLocal(VariantType::Float, String::Format(TEXT("abs({0} * {1}.y - (floor({2} * {3}.x) + {0} * 1))"), invertY.Value, framesXY.Value, frame.Value, framesXYInv.Value), node);
        auto frameX = writeLocal(VariantType::Float, String::Format(TEXT("abs({0} * {1}.x - (({2} - {1}.x * floor({2} * {3}.x)) + {0} * 1))"), invertX.Value, framesXY.Value, frame.Value, framesXYInv.Value), node);
        value = writeLocal(VariantType::Vector2, String::Format(TEXT("({3} + float2({0}, {1})) * {2}"), frameX.Value, frameY.Value, framesXYInv.Value, uv.Value), node);
        break;
    }
    default:
        break;
    }
}

#endif
