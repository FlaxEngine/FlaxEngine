// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"
#include "Engine/Visject/ShaderGraphUtilities.h"
#include "Engine/Platform/File.h"
#include "Engine/Graphics/Materials/MaterialShader.h"
#include "Engine/Graphics/Materials/MaterialShaderFeatures.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Threading/Threading.h"

/// <summary>
/// Material shader source code template has special marks for generated code.
/// Each starts with '@' char and index of the mapped string.
/// </summary>
enum MaterialTemplateInputsMapping
{
    In_VersionNumber = 0,
    In_Constants = 1,
    In_ShaderResources = 2,
    In_Defines = 3,
    In_GetMaterialPS = 4,
    In_GetMaterialVS = 5,
    In_GetMaterialDS = 6,
    In_Includes = 7,
    In_Utilities = 8,
    In_Shaders = 9,

    In_MAX
};

/// <summary>
/// Material shader feature source code template has special marks for generated code. Each starts with '@' char and index of the mapped string.
/// </summary>
enum class FeatureTemplateInputsMapping
{
    Defines = 0,
    Includes = 1,
    Constants = 2,
    Resources = 3,
    Utilities = 4,
    Shaders = 5,
    MAX
};

struct FeatureData
{
    MaterialShaderFeature::GeneratorData Data;
    String Inputs[(int32)FeatureTemplateInputsMapping::MAX];

    bool Init();
};

namespace
{
    // Loaded and parsed features data cache
    Dictionary<StringAnsi, FeatureData> Features;
    CriticalSection FeaturesLock;
}

bool FeatureData::Init()
{
    // Load template file
    const String path = Globals::EngineContentFolder / TEXT("Editor/MaterialTemplates/") + Data.Template;
    String contents;
    if (File::ReadAllText(path, contents))
    {
        LOG(Error, "Cannot open file {0}", path);
        return true;
    }

    int32 i = 0;
    const int32 length = contents.Length();

    // Skip until input start
    for (; i < length; i++)
    {
        if (contents[i] == '@')
            break;
    }

    // Load all inputs
    do
    {
        // Parse input type
        i++;
        const int32 inIndex = contents[i++] - '0';
        ASSERT_LOW_LAYER(Math::IsInRange(inIndex, 0, (int32)FeatureTemplateInputsMapping::MAX - 1));

        // Read until next input start
        const Char* start = &contents[i];
        for (; i < length; i++)
        {
            const auto c = contents[i];
            if (c == '@')
                break;
        }
        const Char* end = &contents[i];

        // Set input
        Inputs[inIndex].Set(start, (int32)(end - start));
    } while (i < length);

    return false;
}

MaterialValue MaterialGenerator::getUVs(VariantType::Vector2, TEXT("input.TexCoord"));
MaterialValue MaterialGenerator::getTime(VariantType::Float, TEXT("TimeParam"));
MaterialValue MaterialGenerator::getNormal(VariantType::Vector3, TEXT("input.TBN[2]"));
MaterialValue MaterialGenerator::getNormalZero(VariantType::Vector3, TEXT("float3(0, 0, 1)"));
MaterialValue MaterialGenerator::getVertexColor(VariantType::Vector4, TEXT("GetVertexColor(input)"));

MaterialGenerator::MaterialGenerator()
{
    // Register per group type processing events
    // Note: index must match group id
    _perGroupProcessCall[1].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupMaterial>(this);
    _perGroupProcessCall[3].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupMath>(this);
    _perGroupProcessCall[5].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupTextures>(this);
    _perGroupProcessCall[6].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupParameters>(this);
    _perGroupProcessCall[7].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupTools>(this);
    _perGroupProcessCall[8].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupLayers>(this);
    _perGroupProcessCall[14].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupParticles>(this);
    _perGroupProcessCall[16].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupFunction>(this);
}

MaterialGenerator::~MaterialGenerator()
{
    _layers.ClearDelete();
}

bool MaterialGenerator::Generate(WriteStream& source, MaterialInfo& materialInfo, BytesContainer& parametersData)
{
    ASSERT_LOW_LAYER(_layers.Count() > 0);

    String inputs[In_MAX];
    Array<StringAnsiView, FixedAllocation<8>> features;

    // Setup and prepare layers
    _writer.Clear();
    _includes.Clear();
    _callStack.Clear();
    _parameters.Clear();
    _localIndex = 0;
    _vsToPsInterpolants.Clear();
    _treeLayer = nullptr;
    _graphStack.Clear();
    for (int32 i = 0; i < _layers.Count(); i++)
    {
        auto layer = _layers[i];

        // Prepare
        layer->Prepare();
        prepareLayer(_layers[i], true);

        // Assign layer variable name for initial layers
        layer->Usage[0].VarName = TEXT("material");
        if (i != 0)
            layer->Usage[0].VarName += StringUtils::ToString(i);
    }
    inputs[In_VersionNumber] = StringUtils::ToString(MATERIAL_GRAPH_VERSION);

    // Cache data
    MaterialLayer* baseLayer = GetRootLayer();
    MaterialGraphNode* baseNode = baseLayer->Root;
    _treeLayerVarName = baseLayer->GetVariableName(nullptr);
    _treeLayer = baseLayer;
    _graphStack.Add(&_treeLayer->Graph);
    const MaterialGraphBox* layerInputBox = baseLayer->Root->GetBox(0);
    const bool isLayered = layerInputBox->HasConnection();

    // Initialize features
#define ADD_FEATURE(type) \
    { \
        StringAnsiView typeName(#type, ARRAY_COUNT(#type) - 1); \
        features.Add(typeName); \
        if (!Features.ContainsKey(typeName)) \
        { \
            ScopeLock lock(FeaturesLock); \
            auto& feature = Features[typeName]; \
            type::Generate(feature.Data); \
            if (feature.Init()) \
                return true; \
        } \
    }
    switch (baseLayer->Domain)
    {
    case MaterialDomain::Surface:
        if (materialInfo.TessellationMode != TessellationMethod::None)
        ADD_FEATURE(TessellationFeature);
        if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
        ADD_FEATURE(MotionVectorsFeature);
        if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
        ADD_FEATURE(LightmapFeature);
        if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
        ADD_FEATURE(DeferredShadingFeature);
        if (materialInfo.BlendMode != MaterialBlendMode::Opaque && (materialInfo.FeaturesFlags & MaterialFeaturesFlags::DisableDistortion) == 0)
        ADD_FEATURE(DistortionFeature);
        if (materialInfo.BlendMode != MaterialBlendMode::Opaque)
        ADD_FEATURE(ForwardShadingFeature);
        break;
    case MaterialDomain::Terrain:
        if (materialInfo.TessellationMode != TessellationMethod::None)
        ADD_FEATURE(TessellationFeature);
        ADD_FEATURE(LightmapFeature);
        ADD_FEATURE(DeferredShadingFeature);
        break;
    case MaterialDomain::Particle:
        if (materialInfo.BlendMode != MaterialBlendMode::Opaque && (materialInfo.FeaturesFlags & MaterialFeaturesFlags::DisableDistortion) == 0)
        ADD_FEATURE(DistortionFeature);
        ADD_FEATURE(ForwardShadingFeature);
        break;
    case MaterialDomain::Deformable:
        if (materialInfo.TessellationMode != TessellationMethod::None)
        ADD_FEATURE(TessellationFeature);
        if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
        ADD_FEATURE(DeferredShadingFeature);
        if (materialInfo.BlendMode != MaterialBlendMode::Opaque)
        ADD_FEATURE(ForwardShadingFeature);
        break;
    default:
        break;
    }
#undef ADD_FEATURE

    // Check if material is using special features and update the metadata flags
    if (!isLayered)
    {
        baseLayer->UpdateFeaturesFlags();
    }

    // Pixel Shader
    _treeType = MaterialTreeType::PixelShader;
    Value materialVarPS;
    if (isLayered)
    {
        materialVarPS = eatBox(baseNode, layerInputBox->FirstConnection());
    }
    else
    {
        materialVarPS = Value(VariantType::Void, baseLayer->GetVariableName(nullptr));
        _writer.Write(TEXT("\tMaterial {0} = (Material)0;\n"), materialVarPS.Value);
        if (baseLayer->Domain == MaterialDomain::Surface || baseLayer->Domain == MaterialDomain::Terrain || baseLayer->Domain == MaterialDomain::Particle || baseLayer->Domain == MaterialDomain::Deformable)
        {
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Normal);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Color);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Metalness);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Specular);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Roughness);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Refraction);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);
        }
        else if (baseLayer->Domain == MaterialDomain::Decal)
        {
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Normal);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Color);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Metalness);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Specular);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Roughness);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);

            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
        }
        else if (baseLayer->Domain == MaterialDomain::PostProcess)
        {
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);

            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Normal);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Color);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Metalness);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Specular);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Roughness);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Mask);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
        }
        else if (baseLayer->Domain == MaterialDomain::GUI)
        {
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);

            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Normal);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Color);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Metalness);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Specular);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Roughness);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
        }
        else if (baseLayer->Domain == MaterialDomain::VolumeParticle)
        {
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Color);

            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Normal);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Metalness);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Specular);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Roughness);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
            eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
        }
        else
        {
            CRASH;
        }
    }
    {
        // Flip normal for inverted triangles (used by two sided materials)
        _writer.Write(TEXT("\t{0}.TangentNormal *= input.TwoSidedSign;\n"), materialVarPS.Value);

        // Normalize and transform to world space if need to
        _writer.Write(TEXT("\t{0}.TangentNormal = normalize({0}.TangentNormal);\n"), materialVarPS.Value);
        if ((baseLayer->FeaturesFlags & MaterialFeaturesFlags::InputWorldSpaceNormal) == 0)
        {
            _writer.Write(TEXT("\t{0}.WorldNormal = normalize(TransformTangentVectorToWorld(input, {0}.TangentNormal));\n"), materialVarPS.Value);
        }
        else
        {
            _writer.Write(TEXT("\t{0}.WorldNormal = {0}.TangentNormal;\n"), materialVarPS.Value);
            _writer.Write(TEXT("\t{0}.TangentNormal = normalize(TransformWorldVectorToTangent(input, {0}.WorldNormal));\n"), materialVarPS.Value);
        }

        // Clamp values
        _writer.Write(TEXT("\t{0}.Metalness = saturate({0}.Metalness);\n"), materialVarPS.Value);
        _writer.Write(TEXT("\t{0}.Roughness = max(0.04, {0}.Roughness);\n"), materialVarPS.Value);
        _writer.Write(TEXT("\t{0}.AO = saturate({0}.AO);\n"), materialVarPS.Value);
        _writer.Write(TEXT("\t{0}.Opacity = saturate({0}.Opacity);\n"), materialVarPS.Value);

        // Return result
        _writer.Write(TEXT("\treturn {0};"), materialVarPS.Value);
    }
    inputs[In_GetMaterialPS] = _writer.ToString();
    _writer.Clear();
    clearCache();

    // Domain Shader
    _treeType = MaterialTreeType::DomainShader;
    if (isLayered)
    {
        const Value layer = eatBox(baseNode, layerInputBox->FirstConnection());
        _writer.Write(TEXT("\treturn {0};"), layer.Value);
    }
    else
    {
        _writer.Write(TEXT("\tMaterial material = (Material)0;\n"));
        eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::WorldDisplacement);
        _writer.Write(TEXT("\treturn material;"));
    }
    inputs[In_GetMaterialDS] = _writer.ToString();
    _writer.Clear();
    clearCache();

    // Vertex Shader
    _treeType = MaterialTreeType::VertexShader;
    if (isLayered)
    {
        const Value layer = eatBox(baseNode, layerInputBox->FirstConnection());
        _writer.Write(TEXT("\treturn {0};"), layer.Value);
    }
    else
    {
        _writer.Write(TEXT("\tMaterial material = (Material)0;\n"));
        eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::PositionOffset);
        eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::TessellationMultiplier);
        for (int32 i = 0; i < _vsToPsInterpolants.Count(); i++)
        {
            const auto value = tryGetValue(_vsToPsInterpolants[i], Value::Zero).AsVector4().Value;
            _writer.Write(TEXT("\tmaterial.CustomVSToPS[{0}] = {1};\n"), i, value);
        }
        _writer.Write(TEXT("\treturn material;"));
    }
    inputs[In_GetMaterialVS] = _writer.ToString();
    _writer.Clear();
    clearCache();

    // Update material usage based on material generator outputs
    materialInfo.UsageFlags = baseLayer->UsageFlags;

#define WRITE_FEATURES(input) FeaturesLock.Lock(); for (auto f : features) _writer.Write(Features[f].Inputs[(int32)FeatureTemplateInputsMapping::input]); FeaturesLock.Unlock();
    // Defines
    {
        _writer.Write(TEXT("#define MATERIAL_MASK_THRESHOLD ({0})\n"), baseLayer->MaskThreshold);
        _writer.Write(TEXT("#define CUSTOM_VERTEX_INTERPOLATORS_COUNT ({0})\n"), _vsToPsInterpolants.Count());
        _writer.Write(TEXT("#define MATERIAL_OPACITY_THRESHOLD ({0})\n"), baseLayer->OpacityThreshold);
        WRITE_FEATURES(Defines);
        inputs[In_Defines] = _writer.ToString();
        _writer.Clear();
    }

    // Includes
    {
        for (auto& include : _includes)
            _writer.Write(TEXT("#include \"{0}\"\n"), include.Item);
        WRITE_FEATURES(Includes);
        inputs[In_Includes] = _writer.ToString();
        _writer.Clear();
    }

    // Constants
    {
        WRITE_FEATURES(Constants);
        if (_parameters.HasItems())
            ShaderGraphUtilities::GenerateShaderConstantBuffer(_writer, _parameters);
        inputs[In_Constants] = _writer.ToString();
        _writer.Clear();
    }

    // Resources
    {
        int32 srv = 0, sampler = GPU_STATIC_SAMPLERS_COUNT;
        switch (baseLayer->Domain)
        {
        case MaterialDomain::Surface:
            srv = 2; // Skinning Bones + Prev Bones
            break;
        case MaterialDomain::Decal:
            srv = 1; // Depth buffer
            break;
        case MaterialDomain::Terrain:
            srv = 3; // Heightmap + 2 splatmaps
            break;
        case MaterialDomain::Particle:
            srv = 2; // Particles data + Sorted indices/Ribbon segments
            break;
        case MaterialDomain::Deformable:
            srv = 1; // Mesh deformation buffer
            break;
        case MaterialDomain::VolumeParticle:
            srv = 1; // Particles data
            break;
        }
        for (auto f : features)
        {
            const auto& text = Features[f].Inputs[(int32)FeatureTemplateInputsMapping::Resources];
            const Char* str = text.Get();
            int32 prevIdx = 0, idx = 0;
            while (true)
            {
                idx = text.Find(TEXT("__SRV__"), StringSearchCase::CaseSensitive, prevIdx);
                if (idx == -1)
                    break;
                int32 len = idx - prevIdx;
                _writer.Write(StringView(str, len));
                str += len;
                _writer.Write(StringUtils::ToString(srv));
                srv++;
                str += ARRAY_COUNT("__SRV__") - 1;
                prevIdx = idx + ARRAY_COUNT("__SRV__") - 1;
            }
            _writer.Write(StringView(str));
        }
        if (_parameters.HasItems())
        {
            auto error = ShaderGraphUtilities::GenerateShaderResources(_writer, _parameters, srv);
            if (!error)
                error = ShaderGraphUtilities::GenerateSamplers(_writer, _parameters, sampler);
            if (error)
            {
                OnError(nullptr, nullptr, error);
                return true;
            }
        }
        inputs[In_ShaderResources] = _writer.ToString();
        _writer.Clear();
    }

    // Utilities
    {
        WRITE_FEATURES(Utilities);
        inputs[In_Utilities] = _writer.ToString();
        _writer.Clear();
    }

    // Shaders
    {
        WRITE_FEATURES(Shaders);
        inputs[In_Shaders] = _writer.ToString();
        _writer.Clear();
    }

    // Save material parameters data
    if (_parameters.HasItems())
        MaterialParams::Save(parametersData, &_parameters);
    else
        parametersData.Release();
    _parameters.Clear();

    // Create source code
    {
        // Open template file
        String path = Globals::EngineContentFolder / TEXT("Editor/MaterialTemplates/");
        switch (materialInfo.Domain)
        {
        case MaterialDomain::Surface:
            path /= TEXT("Surface.shader");
            break;
        case MaterialDomain::PostProcess:
            path /= TEXT("PostProcess.shader");
            break;
        case MaterialDomain::GUI:
            path /= TEXT("GUI.shader");
            break;
        case MaterialDomain::Decal:
            path /= TEXT("Decal.shader");
            break;
        case MaterialDomain::Terrain:
            path /= TEXT("Terrain.shader");
            break;
        case MaterialDomain::Particle:
            path /= TEXT("Particle.shader");
            break;
        case MaterialDomain::Deformable:
            path /= TEXT("Deformable.shader");
            break;
        case MaterialDomain::VolumeParticle:
            path /= TEXT("VolumeParticle.shader");
            break;
        default:
            LOG(Warning, "Unknown material domain.");
            return true;
        }
        auto file = FileReadStream::Open(path);
        if (file == nullptr)
        {
            LOG(Error, "Cannot open file {0}", path);
            return true;
        }

        // Format template
        const uint32 length = file->GetLength();
        Array<char> tmp;
        for (uint32 i = 0; i < length; i++)
        {
            char c = file->ReadByte();

            if (c != '@')
            {
                source.WriteByte(c);
            }
            else
            {
                i++;
                int32 inIndex = file->ReadByte() - '0';
                ASSERT_LOW_LAYER(Math::IsInRange(inIndex, 0, In_MAX - 1));

                const String& in = inputs[inIndex];
                if (in.Length() > 0)
                {
                    tmp.EnsureCapacity(in.Length() + 1, false);
                    StringUtils::ConvertUTF162ANSI(*in, tmp.Get(), in.Length());
                    source.WriteBytes(tmp.Get(), in.Length());
                }
            }
        }

        // Close file
        Delete(file);

        // Ensure to have null-terminated source code
        source.WriteByte(0);
    }

    return false;
}

void MaterialGenerator::clearCache()
{
    for (int32 i = 0; i < _layers.Count(); i++)
        _layers[i]->ClearCache();
    for (auto& e : _functions)
    {
        for (auto& node : e.Value->Nodes)
        {
            for (int32 j = 0; j < node.Boxes.Count(); j++)
                node.Boxes[j].Cache.Clear();
        }
    }
    _ddx = Value();
    _ddy = Value();
    _cameraVector = Value();
}

void MaterialGenerator::writeBlending(MaterialGraphBoxes box, Value& result, const Value& bottom, const Value& top, const Value& alpha)
{
    const auto& boxInfo = GetMaterialRootNodeBox(box);
    _writer.Write(TEXT("\t{0}.{1} = lerp({2}.{1}, {3}.{1}, {4});\n"), result.Value, boxInfo.SubName, bottom.Value, top.Value, alpha.Value);
    if (box == MaterialGraphBoxes::Normal)
    {
        _writer.Write(TEXT("\t{0}.{1} = normalize({0}.{1});\n"), result.Value, boxInfo.SubName);
    }
}

SerializedMaterialParam* MaterialGenerator::findParam(const Guid& id, MaterialLayer* layer)
{
    // Use per material layer params mapping
    return findParam(layer->GetMappedParamId(id));
}

MaterialGraphParameter* MaterialGenerator::findGraphParam(const Guid& id)
{
    MaterialGraphParameter* result = nullptr;

    for (int32 i = 0; i < _layers.Count(); i++)
    {
        result = _layers[i]->Graph.GetParameter(id);
        if (result)
            break;
    }

    return result;
}

void MaterialGenerator::createGradients(Node* caller)
{
    if (_ddx.IsInvalid())
        _ddx = writeLocal(VariantType::Vector2, TEXT("ddx(input.TexCoord.xy)"), caller);
    if (_ddy.IsInvalid())
        _ddy = writeLocal(VariantType::Vector2, TEXT("ddy(input.TexCoord.xy)"), caller);
}

MaterialGenerator::Value MaterialGenerator::getCameraVector(Node* caller)
{
    if (_cameraVector.IsInvalid())
        _cameraVector = writeLocal(VariantType::Vector3, TEXT("normalize(ViewPos.xyz - input.WorldPosition.xyz)"), caller);
    return _cameraVector;
}

void MaterialGenerator::eatMaterialGraphBox(String& layerVarName, MaterialGraphBox* nodeBox, MaterialGraphBoxes box)
{
    // Cache data
    auto& boxInfo = GetMaterialRootNodeBox(box);

    // Get value
    const auto value = Value::Cast(tryGetValue(nodeBox, boxInfo.DefaultValue), boxInfo.DefaultValue.Type);

    // Write formatted value
    _writer.WriteLine(TEXT("\t{0}.{1} = {2};"), layerVarName, boxInfo.SubName, value.Value);
}

void MaterialGenerator::eatMaterialGraphBox(MaterialLayer* layer, MaterialGraphBoxes box)
{
    auto& boxInfo = GetMaterialRootNodeBox(box);
    const auto nodeBox = layer->Root->GetBox(boxInfo.ID);
    eatMaterialGraphBox(_treeLayerVarName, nodeBox, box);
}

void MaterialGenerator::eatMaterialGraphBoxWithDefault(MaterialLayer* layer, MaterialGraphBoxes box)
{
    auto& boxInfo = GetMaterialRootNodeBox(box);
    _writer.WriteLine(TEXT("\t{0}.{1} = {2};"), _treeLayerVarName, boxInfo.SubName, boxInfo.DefaultValue.Value);
}

void MaterialGenerator::ProcessGroupMath(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Vector Transform
    case 30:
    {
        // Get input vector
        auto v = tryGetValue(node->GetBox(0), Value::InitForZero(VariantType::Vector3));

        // Select transformation spaces
        ASSERT(node->Values[0].Type == VariantType::Int && node->Values[1].Type == VariantType::Int);
        ASSERT(Math::IsInRange(node->Values[0].AsInt, 0, (int32)TransformCoordinateSystem::MAX - 1));
        ASSERT(Math::IsInRange(node->Values[1].AsInt, 0, (int32)TransformCoordinateSystem::MAX - 1));
        auto inputType = static_cast<TransformCoordinateSystem>(node->Values[0].AsInt);
        auto outputType = static_cast<TransformCoordinateSystem>(node->Values[1].AsInt);
        if (inputType == outputType)
        {
            // No space change at all
            value = v;
        }
        else
        {
            // Switch by source space type
            const Char* format = nullptr;
            switch (inputType)
            {
            case TransformCoordinateSystem::Tangent:
                switch (outputType)
                {
                case TransformCoordinateSystem::Tangent:
                    format = TEXT("{0}");
                    break;
                case TransformCoordinateSystem::World:
                    format = TEXT("TransformTangentVectorToWorld(input, {0})");
                    break;
                case TransformCoordinateSystem::View:
                    format = TEXT("TransformWorldVectorToView(input, TransformTangentVectorToWorld(input, {0}))");
                    break;
                case TransformCoordinateSystem::Local:
                    format = TEXT("TransformWorldVectorToLocal(input, TransformTangentVectorToWorld(input, {0}))");
                    break;
                }
                break;
            case TransformCoordinateSystem::World:
                switch (outputType)
                {
                case TransformCoordinateSystem::Tangent:
                    format = TEXT("TransformWorldVectorToTangent(input, {0})");
                    break;
                case TransformCoordinateSystem::World:
                    format = TEXT("{0}");
                    break;
                case TransformCoordinateSystem::View:
                    format = TEXT("TransformWorldVectorToView(input, {0})");
                    break;
                case TransformCoordinateSystem::Local:
                    format = TEXT("TransformWorldVectorToLocal(input, {0})");
                    break;
                }
                break;
            case TransformCoordinateSystem::View:
                switch (outputType)
                {
                case TransformCoordinateSystem::Tangent:
                    format = TEXT("TransformWorldVectorToTangent(input, TransformViewVectorToWorld(input, {0}))");
                    break;
                case TransformCoordinateSystem::World:
                    format = TEXT("TransformViewVectorToWorld(input, {0})");
                    break;
                case TransformCoordinateSystem::View:
                    format = TEXT("{0}");
                    break;
                case TransformCoordinateSystem::Local:
                    format = TEXT("TransformWorldVectorToLocal(input, TransformViewVectorToWorld(input, {0}))");
                    break;
                }
                break;
            case TransformCoordinateSystem::Local:
                switch (outputType)
                {
                case TransformCoordinateSystem::Tangent:
                    format = TEXT("TransformWorldVectorToTangent(input, TransformLocalVectorToWorld(input, {0}))");
                    break;
                case TransformCoordinateSystem::World:
                    format = TEXT("TransformLocalVectorToWorld(input, {0})");
                    break;
                case TransformCoordinateSystem::View:
                    format = TEXT("TransformWorldVectorToView(input, TransformLocalVectorToWorld(input, {0}))");
                    break;
                case TransformCoordinateSystem::Local:
                    format = TEXT("{0}");
                    break;
                }
                break;
            }
            ASSERT(format != nullptr);

            // Write operation
            value = writeLocal(VariantType::Vector3, String::Format(format, v.Value), node);
        }
        break;
    }
    default:
        ShaderGenerator::ProcessGroupMath(box, node, value);
        break;
    }
}

#endif
