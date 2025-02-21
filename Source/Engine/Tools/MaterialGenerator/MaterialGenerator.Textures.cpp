// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"

MaterialValue* MaterialGenerator::sampleTextureRaw(Node* caller, Value& value, Box* box, SerializedMaterialParam* texture)
{
    ASSERT(texture && box);

    // Cache data
    const auto parent = box->GetParent<ShaderGraphNode<>>();
    const bool isCubemap = texture->Type == MaterialParameterType::CubeTexture;
    const bool isArray = texture->Type == MaterialParameterType::GPUTextureArray;
    const bool isVolume = texture->Type == MaterialParameterType::GPUTextureVolume;
    const bool isNormalMap = texture->Type == MaterialParameterType::NormalMap;
    const bool canUseSample = CanUseSample(_treeType);
    MaterialGraphBox* valueBox = parent->GetBox(1);

    // Check if has variable assigned
    if (texture->Type != MaterialParameterType::Texture
        && texture->Type != MaterialParameterType::NormalMap
        && texture->Type != MaterialParameterType::SceneTexture
        && texture->Type != MaterialParameterType::GPUTexture
        && texture->Type != MaterialParameterType::GPUTextureVolume
        && texture->Type != MaterialParameterType::GPUTextureCube
        && texture->Type != MaterialParameterType::GPUTextureArray
        && texture->Type != MaterialParameterType::CubeTexture)
    {
        OnError(caller, box, TEXT("No parameter for texture sample node."));
        return nullptr;
    }

    // Check if it's 'Object' box that is using only texture object without sampling
    if (box->ID == 6)
    {
        // Return texture object
        value.Value = texture->ShaderName;
        value.Type = VariantType::Object;
        return nullptr;
    }

    // Check if hasn't been sampled during that tree eating
    if (valueBox->Cache.IsInvalid())
    {
        // Check if use custom UVs
        String uv;
        MaterialGraphBox* uvBox = parent->GetBox(0);
        bool useCustomUVs = uvBox->HasConnection();
        bool use3dUVs = isCubemap || isArray || isVolume;
        if (useCustomUVs)
        {
            // Get custom UVs
            auto textureParamId = texture->ID;
            ASSERT(textureParamId.IsValid());
            MaterialValue v = tryGetValue(uvBox, getUVs);
            uv = MaterialValue::Cast(v, use3dUVs ? VariantType::Float3 : VariantType::Float2).Value;

            // Restore texture (during tryGetValue pointer could go invalid)
            texture = findParam(textureParamId);
            ASSERT(texture);
        }
        else
        {
            // Use default UVs
            uv = use3dUVs ? TEXT("float3(input.TexCoord.xy, 0)") : TEXT("input.TexCoord.xy");
        }

        // Select sampler
        // TODO: add option for texture groups and per texture options like wrap mode etc.
        // TODO: changing texture sampler option
        const Char* sampler = TEXT("SamplerLinearWrap");

        // Sample texture
        if (isNormalMap)
        {
            const Char* format = canUseSample ? TEXT("{0}.Sample({1}, {2}).xyz") : TEXT("{0}.SampleLevel({1}, {2}, 0).xyz");

            // Sample encoded normal map
            const String sampledValue = String::Format(format, texture->ShaderName, sampler, uv);
            const auto normalVector = writeLocal(VariantType::Float3, sampledValue, parent);

            // Decode normal vector
            _writer.Write(TEXT("\t{0}.xy = {0}.xy * 2.0 - 1.0;\n"), normalVector.Value);
            _writer.Write(TEXT("\t{0}.z = sqrt(saturate(1.0 - dot({0}.xy, {0}.xy)));\n"), normalVector.Value);
            valueBox->Cache = normalVector;
        }
        else
        {
            // Select format string based on texture type
            const Char* format;
            /*if (isCubemap)
            {
            MISSING_CODE("sampling cube maps and 3d texture in material generator");
            //format = TEXT("SAMPLE_CUBEMAP({0}, {1})");
            }
            else*/
            {
                /*if (useCustomUVs)
                {
                createGradients(writer, parent);
                format = TEXT("SAMPLE_TEXTURE_GRAD({0}, {1}, {2}, {3})");
                }
                else*/
                {
                    format = canUseSample ? TEXT("{0}.Sample({1}, {2})") : TEXT("{0}.SampleLevel({1}, {2}, 0)");
                }
            }

            // Sample texture
            String sampledValue = String::Format(format, texture->ShaderName, sampler, uv, _ddx.Value, _ddy.Value);
            valueBox->Cache = writeLocal(VariantType::Float4, sampledValue, parent);
        }
    }

    return &valueBox->Cache;
}

void MaterialGenerator::sampleTexture(Node* caller, Value& value, Box* box, SerializedMaterialParam* texture)
{
    const auto sample = sampleTextureRaw(caller, value, box, texture);
    if (sample == nullptr)
        return;

    // Set result values based on box ID
    switch (box->ID)
    {
    case 1:
        value = *sample;
        break;
    case 2:
        value.Value = sample->Value + _subs[0];
        break;
    case 3:
        value.Value = sample->Value + _subs[1];
        break;
    case 4:
        value.Value = sample->Value + _subs[2];
        break;
    case 5:
        value.Value = sample->Value + _subs[3];
        break;
    default: CRASH;
        break;
    }
    value.Type = box->Type.Type;
}

void MaterialGenerator::sampleSceneDepth(Node* caller, Value& value, Box* box)
{
    // Sample depth buffer
    auto param = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
    const auto depthSample = sampleTextureRaw(caller, value, box, &param);
    if (depthSample == nullptr)
        return;

    // Linearize raw device depth
    linearizeSceneDepth(caller, *depthSample, value);
}

void MaterialGenerator::linearizeSceneDepth(Node* caller, const Value& depth, Value& value)
{
    value = writeLocal(VariantType::Float, String::Format(TEXT("ViewInfo.w / ({0}.x - ViewInfo.z)"), depth.Value), caller);
}

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
        Value uvs = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        if (_treeType != MaterialTreeType::PixelShader)
        {
            // Required ddx/ddy instructions are only supported in Pixel Shader
            value = uvs;
            break;
        }
        Value scale = tryGetValue(node->GetBox(1), node->Values[0]);
        Value minSteps = tryGetValue(node->GetBox(2), node->Values[1]);
        Value maxSteps = tryGetValue(node->GetBox(3), node->Values[2]);
        Value result = writeLocal(VariantType::Float2, uvs.Value, node);
        createGradients(node);
        ASSERT(node->Values[3].Type == VariantType::Int && Math::IsInRange(node->Values[3].AsInt, 0, 3));
        auto channel = _subs[node->Values[3].AsInt];
        Value cameraVectorWS = getCameraVector(node);
        Value cameraVectorTS = writeLocal(VariantType::Float3, String::Format(TEXT("TransformWorldVectorToTangent(input, {0})"), cameraVectorWS.Value), node);
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
            value = writeLocal(VariantType::Float3, String::Format(TEXT("GetDiffuseColor({0}.rgb, {1}.g)"), gBuffer0Sample->Value, gBuffer2Sample->Value), node);
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
            value = writeLocal(VariantType::Float3, String::Format(TEXT("GetSpecularColor({0}.rgb, {1}.b, {1}.g)"), gBuffer0Sample->Value, gBuffer2Sample->Value), node);
            break;
        }
        case MaterialSceneTextures::WorldNormal:
        {
            auto gBuffer1Param = findOrAddSceneTexture(MaterialSceneTextures::WorldNormal);
            auto gBuffer1Sample = sampleTextureRaw(node, value, box, &gBuffer1Param);
            if (gBuffer1Sample == nullptr)
                break;
            value = writeLocal(VariantType::Float3, String::Format(TEXT("DecodeNormal({0}.rgb)"), gBuffer1Sample->Value), node);
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
        case MaterialSceneTextures::WorldPosition:
        {
            auto depthParam = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
            auto depthSample = sampleTextureRaw(node, value, box, &depthParam);
            if (depthSample == nullptr)
                break;
            const auto parent = box->GetParent<ShaderGraphNode<>>();
            MaterialGraphBox* uvBox = parent->GetBox(0);
            bool useCustomUVs = uvBox->HasConnection();
            String uv;
            if (useCustomUVs)
                uv = MaterialValue::Cast(tryGetValue(uvBox, getUVs), VariantType::Float2).Value;
            else
                uv = TEXT("input.TexCoord.xy");
            value = writeLocal(VariantType::Float3, String::Format(TEXT("GetWorldPos({1}, {0}.rgb)"), depthSample->Value, uv), node);
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

        // Channel masking
        switch (box->ID)
        {
        case 2:
            value = value.GetX();
            break;
        case 3:
            value = value.GetY();
            break;
        case 4:
            value = value.GetZ();
            break;
        case 5:
            value = value.GetW();
            break;
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
    // Procedural Texture Sample
    case 17:
    {
        enum CommonSamplerType
        {
            LinearClamp = 0,
            PointClamp = 1,
            LinearWrap = 2,
            PointWrap = 3,
            TextureGroup = 4,
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
        auto levelBox = node->TryGetBox(2);
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
        uvs = Value::Cast(uvs, use3dUVs ? VariantType::Float3 : VariantType::Float2);

        // Get other inputs
        const auto level = tryGetValue(levelBox, node->Values[1]);
        const bool useLevel = (levelBox && levelBox->HasConnection()) || (int32)node->Values[1] != -1;
        const bool useOffset = offsetBox->HasConnection();
        const auto offset = useOffset ? eatBox(offsetBox->GetParent<Node>(), offsetBox->FirstConnection()) : Value::Zero;
        const Char* samplerName;
        const int32 samplerIndex = node->Values[0].AsInt;
        if (samplerIndex == TextureGroup)
        {
            auto& textureGroupSampler = findOrAddTextureGroupSampler(node->Values[2].AsInt);
            samplerName = *textureGroupSampler.ShaderName;
        }
        else if (samplerIndex >= 0 && samplerIndex < ARRAY_COUNT(SamplerNames))
        {
            samplerName = SamplerNames[samplerIndex];
        }
        else
        {
            OnError(node, box, TEXT("Invalid texture sampler."));
            return;
        }

        // Create texture sampling code
        if (node->TypeID == 9)
        {
            // Sample Texture
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
            const String sampledValue = String::Format(format, texture.Value, samplerName, uvs.Value, level.Value, offset.Value);
            textureBox->Cache = writeLocal(VariantType::Float4, sampledValue, node);
        }
        else
        {
            // Procedural Texture Sample
            textureBox->Cache = writeLocal(Value::InitForZero(ValueType::Float4), node);
            auto proceduralSample = String::Format(TEXT(
                "   {{\n"
                "   float3 weights;\n"
                "   float2 vertex1, vertex2, vertex3;\n"
                "   float2 uv = {0} * 3.464; // 2 * sqrt (3);\n"
                "   float2 uv1, uv2, uv3;\n"
                "   const float2x2 gridToSkewedGrid = float2x2(1.0, 0.0, -0.57735027, 1.15470054);\n"
                "   float2 skewedCoord = mul(gridToSkewedGrid, uv);\n"
                "   int2 baseId = int2(floor(skewedCoord));\n"
                "   float3 temp = float3(frac(skewedCoord), 0);\n"
                "   temp.z = 1.0 - temp.x - temp.y;\n"
                "   if (temp.z > 0.0)\n"
                "   {{\n"
                "   	weights = float3(temp.z, temp.y, temp.x);\n"
                "   	vertex1 = baseId;\n"
                "   	vertex2 = baseId + int2(0, 1);\n"
                "   	vertex3 = baseId + int2(1, 0);\n"
                "   }}\n"
                "   else\n"
                "   {{\n"
                "   	weights = float3(-temp.z, 1.0 - temp.y, 1.0 - temp.x);\n"
                "   	vertex1 = baseId + int2(1, 1);\n"
                "   	vertex2 = baseId + int2(1, 0);\n"
                "   	vertex3 = baseId + int2(0, 1);\n"
                "   }}\n"
                "   uv1 = {0} + frac(sin(mul(float2x2(127.1, 311.7, 269.5, 183.3), vertex1)) * 43758.5453);\n"
                "   uv2 = {0} + frac(sin(mul(float2x2(127.1, 311.7, 269.5, 183.3), vertex2)) * 43758.5453);\n"
                "   uv3 = {0} + frac(sin(mul(float2x2(127.1, 311.7, 269.5, 183.3), vertex3)) * 43758.5453);\n"
                "   float2 fdx = ddx({0});\n"
                "   float2 fdy = ddy({0});\n"
                "   float4 tex1 = {1}.SampleGrad({2}, uv1, fdx, fdy, {4}) * weights.x;\n"
                "   float4 tex2 = {1}.SampleGrad({2}, uv2, fdx, fdy, {4}) * weights.y;\n"
                "   float4 tex3 = {1}.SampleGrad({2}, uv3, fdx, fdy, {4}) * weights.z;\n"
                "   {3} = tex1 + tex2 + tex3;\n"
                "   }}\n"
            ),
                                                   uvs.Value, // {0}
                                                   texture.Value, // {1}
                                                   samplerName, // {2}
                                                   textureBox->Cache.Value, // {3}
                                                   offset.Value // {4}
            );

            _writer.Write(*proceduralSample);
        }

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
        auto uv = Value::Cast(tryGetValue(node->GetBox(0), getUVs), VariantType::Float2);
        auto frame = Value::Cast(tryGetValue(node->GetBox(1), node->Values[0]), VariantType::Float);
        auto framesXY = Value::Cast(tryGetValue(node->GetBox(2), node->Values[1]), VariantType::Float2);
        auto invertX = Value::Cast(tryGetValue(node->GetBox(3), node->Values[2]), VariantType::Float);
        auto invertY = Value::Cast(tryGetValue(node->GetBox(4), node->Values[3]), VariantType::Float);
        value = writeLocal(VariantType::Float2, String::Format(TEXT("Flipbook({0}, {1}, {2}, float2({3}, {4}))"), uv.Value, frame.Value, framesXY.Value, invertX.Value, invertY.Value), node);
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
    // World Triplanar Texture
    case 16:
    {
        auto textureBox = node->GetBox(0);
        auto scaleBox = node->GetBox(1);
        auto blendBox = node->GetBox(2);
        if (!textureBox->HasConnection())
        {
            // No texture to sample
            value = Value::Zero;
            break;
        }
        const bool canUseSample = CanUseSample(_treeType);
        const auto texture = eatBox(textureBox->GetParent<Node>(), textureBox->FirstConnection());
        const auto scale = tryGetValue(scaleBox, node->Values[0]).AsFloat4();
        const auto blend = tryGetValue(blendBox, node->Values[1]).AsFloat();
        auto result = writeLocal(Value::InitForZero(ValueType::Float4), node);
        const String triplanarTexture = String::Format(TEXT(
            "	{{\n"
            "   float tiling = {1} * 0.001f;\n"
            "   float3 worldPos = (input.WorldPosition.xyz + GetLargeWorldsTileOffset(1.0f / tiling)) * tiling;\n"
            "   float3 normal = abs(input.TBN[2]);\n"
            "   normal = pow(normal, {2});\n"
            "   normal = normalize(normal);\n"
            "   {3} += {0}.{4}(SamplerLinearWrap, worldPos.yz{5}) * normal.x;\n"
            "   {3} += {0}.{4}(SamplerLinearWrap, worldPos.xz{5}) * normal.y;\n"
            "   {3} += {0}.{4}(SamplerLinearWrap, worldPos.xy{5}) * normal.z;\n"
            "	}}\n"
        ),
                                                       texture.Value, //  {0}
                                                       scale.Value, //  {1}
                                                       blend.Value, //  {2}
                                                       result.Value, //  {3}
                                                       canUseSample ? TEXT("Sample") : TEXT("SampleLevel"), //  {4}
                                                       canUseSample ? TEXT("") : TEXT(", 0") //  {5}
        );
        _writer.Write(*triplanarTexture);
        value = result;
        break;
    }
    // Get Lightmap UV
    case 18: 
    {
        auto output = writeLocal(Value::InitForZero(ValueType::Float2), node);
        auto lightmapUV = String::Format(TEXT(
            "{{\n"
            "#if USE_LIGHTMAP\n"
            "\t {0} = input.LightmapUV;\n"
            "#else\n"
            "\t {0} = float2(0,0);\n"
            "#endif\n"
            "}}\n"
        ), output.Value);
        _writer.Write(*lightmapUV);
        value = output;
        break;
    }
    default:
        break;
    }
}

#endif
