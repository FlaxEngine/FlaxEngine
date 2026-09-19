// Copyright (c) Wojciech Figat. All rights reserved.

#include "CreateMaterial.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#define COMPILE_WITH_MATERIAL_GRAPH 1
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Tools/MaterialGenerator/Types.h"
#include "Engine/Tools/MaterialGenerator/MaterialLayer.h"
#include "Engine/Tools/MaterialGenerator/MaterialGenerator.h"
#include "Engine/Serialization/MemoryWriteStream.h"

#define SET_POS(node, pos) meta.Position = pos; node->Meta.AddEntry(11, (byte*)&meta, sizeof(meta));
#define CONNECT(boxA, boxB) boxA.Connections.Add(&boxB); boxB.Connections.Add(&boxA)

namespace
{
    template<typename T>
    ShaderGraphNode<>* AddValueNode(MaterialLayer* layer, const float& value, const float& defaultValue)
    {
        if (Math::NearEqual(value, defaultValue))
            return nullptr;
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(2, 3);
        node.Boxes.Resize(1);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Float); // Value
        node.Values.Resize(1);
        node.Values[0] = value;
        return &node;
    }

    template<typename T>
    ShaderGraphNode<>* AddValueNode(MaterialLayer* layer, const Color& value, const Color& defaultValue)
    {
        if (value == defaultValue)
            return nullptr;
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(2, 7);
        node.Boxes.Resize(5);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Float4); // Color
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float); // R
        node.Boxes[2] = MaterialGraphBox(&node, 2, VariantType::Float); // G
        node.Boxes[3] = MaterialGraphBox(&node, 3, VariantType::Float); // B
        node.Boxes[4] = MaterialGraphBox(&node, 4, VariantType::Float); // A
        node.Values.Resize(1);
        node.Values[0] = value;
        return &node;
    }

    ShaderGraphNode<>* AddMultiplyNode(MaterialLayer* layer)
    {
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(3, 3);
        node.Boxes.Resize(3);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Float4); // A
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float4); // B
        node.Boxes[2] = MaterialGraphBox(&node, 2, VariantType::Float4); // Result
        node.Values.Resize(2);
        node.Values[0] = 1.0f;
        node.Values[1] = 1.0f;
        return &node;
    }

    struct Meta11 // TypeID: 11, for nodes
    {
        Float2 Position;
        bool Selected;
    };

    ShaderGraphNode<>* AddColorParameterNode(MaterialLayer* layer, const Color& value, const String& name, const Float2& pos)
    {
        Meta11 meta;
        meta.Selected = false;
        auto& param = layer->Graph.Parameters.AddOne();
        // Unique id so each parameter resolves to itself in the generator; with the default
        // empty Guid, GetMappedParamId would map every parameter node to the first parameter
        // and all references in the generated shader would collapse onto a single input.
        param.Identifier = Guid::New();
        param.Type = VariantType(VariantType::Color);
        param.Name = name;
        param.Value = value;
        param.IsPublic = true;

        // Get Parameter node (group 6, type 1) referencing the parameter by id
        auto* node = &layer->Graph.Nodes.AddOne();
        node->ID = layer->Graph.Nodes.Count();
        node->Type = GRAPH_NODE_MAKE_TYPE(6, 1);
        node->Boxes.Resize(5);
        node->Boxes[0] = MaterialGraphBox(node, 0, VariantType::Float4); // Color
        node->Boxes[1] = MaterialGraphBox(node, 1, VariantType::Float); // R
        node->Boxes[2] = MaterialGraphBox(node, 2, VariantType::Float); // G
        node->Boxes[3] = MaterialGraphBox(node, 3, VariantType::Float); // B
        node->Boxes[4] = MaterialGraphBox(node, 4, VariantType::Float); // A
        node->Values.Resize(1);
        node->Values[0] = param.Identifier;
        SET_POS(node, pos + Float2(-467.7404, 91.41332));
        return node;
    }

    template<typename T>
    ShaderGraphNode<>* AddColorValueNode(MaterialLayer* layer, const T& value, const String& name, const Float2& pos)
    {
        return AddColorParameterNode(layer, static_cast<Color>(value), name, pos);
    }

    ShaderGraphNode<>* AddTextureNode(MaterialLayer* layer, const Guid& textureId, bool normalMap = false)
    {
        if (!textureId.IsValid())
            return nullptr;
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(5, normalMap ? 4 : 1);
        node.Boxes.Resize(7);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Float2); // UVs
        node.Boxes[6] = MaterialGraphBox(&node, 6, VariantType::Object); // Texture Reference
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float4); // Color
        node.Boxes[2] = MaterialGraphBox(&node, 2, VariantType::Float); // R
        node.Boxes[3] = MaterialGraphBox(&node, 3, VariantType::Float); // G
        node.Boxes[4] = MaterialGraphBox(&node, 4, VariantType::Float); // B
        node.Boxes[5] = MaterialGraphBox(&node, 5, VariantType::Float); // A
        node.Values.Resize(1);
        node.Values[0] = textureId;
        return &node;
    }

    ShaderGraphNode<>* AddFloatParameterNode(MaterialLayer* layer, float value, const String& name, const Float2& pos)
    {
        Meta11 meta;
        meta.Selected = false;
        auto& param = layer->Graph.Parameters.AddOne();
        // Unique id so each parameter resolves to itself in the generator (see AddColorParameterNode).
        param.Identifier = Guid::New();
        param.Type = VariantType(VariantType::Float);
        param.Name = name;
        param.Value = value;
        param.IsPublic = true;

        // Get Parameter node (group 6, type 1) referencing the parameter by id
        auto* node = &layer->Graph.Nodes.AddOne();
        node->ID = layer->Graph.Nodes.Count();
        node->Type = GRAPH_NODE_MAKE_TYPE(6, 1);
        node->Boxes.Resize(1);
        node->Boxes[0] = MaterialGraphBox(node, 0, VariantType::Float); // Value
        node->Values.Resize(1);
        node->Values[0] = param.Identifier;
        SET_POS(node, pos);
        return node;
    }

    ShaderGraphNode<>* AddMaskXYNode(MaterialLayer* layer)
    {
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(4, 44);
        node.Boxes.Resize(2);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Null);   // Input
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float2); // XY
        node.Values.Resize(0);
        return &node;
    }

    ShaderGraphNode<>* AddMaskZNode(MaterialLayer* layer)
    {
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(4, 42);
        node.Boxes.Resize(2);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Null);  // Input
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float); // Z
        node.Values.Resize(0);
        return &node;
    }

    ShaderGraphNode<>* AddUnpackFloat2Node(MaterialLayer* layer)
    {
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(4, 30);
        node.Boxes.Resize(3);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Float2); // Value
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float);  // X
        node.Boxes[2] = MaterialGraphBox(&node, 2, VariantType::Float);  // Y
        node.Values.Resize(0);
        return &node;
    }

    ShaderGraphNode<>* AddPackFloat3Node(MaterialLayer* layer)
    {
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(4, 21);
        node.Boxes.Resize(4);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Float3); // Result
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Float);  // X
        node.Boxes[2] = MaterialGraphBox(&node, 2, VariantType::Float);  // Y
        node.Boxes[3] = MaterialGraphBox(&node, 3, VariantType::Float);  // Z
        node.Values.Resize(3);
        node.Values[0] = 0.0f;
        node.Values[1] = 0.0f;
        node.Values[2] = 0.0f;
        return &node;
    }

    ShaderGraphNode<>* AddNormalizeNode(MaterialLayer* layer)
    {
        auto& node = layer->Graph.Nodes.AddOne();
        node.ID = layer->Graph.Nodes.Count();
        node.Type = GRAPH_NODE_MAKE_TYPE(3, 12);
        node.Boxes.Resize(2);
        node.Boxes[0] = MaterialGraphBox(&node, 0, VariantType::Null); // Value
        node.Boxes[1] = MaterialGraphBox(&node, 1, VariantType::Null); // Result
        node.Values.Resize(0);
        return &node;
    }

    template<typename T>
    void AddInput(MaterialLayer* layer, Meta11 meta, MaterialGraphBoxes box, const Guid& texture, const T& value, const T& defaultValue, const Float2& pos, ShaderGraphNode<>** outTextureNode = nullptr, uint8 channel = MAX_uint8, bool asParameter = false, const String& parameterName = TEXT("Color"))
    {
        auto textureNode = AddTextureNode(layer, texture);
        auto valueNode = asParameter ? AddColorValueNode<T>(layer, value, parameterName, pos) : AddValueNode<T>(layer, value, defaultValue);
        auto textureNodeBox = channel == MAX_uint8 ? 1 : channel + 2; // Color or specific channel (RGBA)
        if (textureNode && valueNode)
        {
            auto diffuseMultiply = AddMultiplyNode(layer);
            CONNECT(diffuseMultiply->Boxes[0], textureNode->Boxes[textureNodeBox]);
            CONNECT(diffuseMultiply->Boxes[1], valueNode->Boxes[0]);
            CONNECT(layer->Root->Boxes[static_cast<int32>(box)], diffuseMultiply->Boxes[2]);
            SET_POS(valueNode, pos + Float2(-467.7404, 91.41332));
            SET_POS(textureNode, pos + Float2(-538.096, -103.9724));
            SET_POS(diffuseMultiply, pos + Float2(-293.5272f, -2.926111f));
        }
        else if (textureNode)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(box)], textureNode->Boxes[textureNodeBox]);
            SET_POS(textureNode, pos + Float2(-293.5272f, -2.926111f));
        }
        else if (valueNode)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(box)], valueNode->Boxes[0]);
            SET_POS(valueNode, pos + Float2(-293.5272f, -2.926111f));
        }
        if (outTextureNode)
            *outTextureNode = textureNode;
    }
}

CreateMaterial::Options::Options()
{
    Info.Domain = MaterialDomain::Surface;
    Info.BlendMode = MaterialBlendMode::Opaque;
    Info.ShadingModel = MaterialShadingModel::Lit;
    Info.UsageFlags = MaterialUsageFlags::None;
    Info.FeaturesFlags = MaterialFeaturesFlags::None;
    Info.DecalBlendingMode = MaterialDecalBlendingMode::Translucent;
    Info.TransparentLightingMode = MaterialTransparentLightingMode::Surface;
    Info.PostFxLocation = MaterialPostFxLocation::AfterPostProcessingPass;
    Info.CullMode = CullMode::Normal;
    Info.MaskThreshold = 0.3f;
    Info.OpacityThreshold = 0.12f;
    Info.TessellationMode = TessellationMethod::None;
    Info.MaxTessellationFactor = 15;
}

CreateAssetResult CreateMaterial::Create(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(Material, 20);
    context.SkipMetadata = true;

    ShaderStorage::Header20 shaderHeader;
    Platform::MemoryClear(&shaderHeader, sizeof(shaderHeader));
    if (context.CustomArg)
    {
        // Use custom material properties
        const Options& options = *(Options*)context.CustomArg;
        shaderHeader.Material.Info = options.Info;

        // Generate Visject Surface with custom material properties setup
        auto layer = MaterialLayer::CreateDefault(context.Data.Header.ID);
        if (context.AllocateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
            return CreateAssetResult::CannotAllocateChunk;
        layer->Graph.Nodes.EnsureCapacity(32);
        layer->Root = &layer->Graph.Nodes[0];
        for (auto& box : layer->Root->Boxes)
            box.Parent = layer->Root;
        Meta11 meta;
        meta.Selected = false;

        // Diffuse + Mask
        ShaderGraphNode<>* diffuseTextureNode;
        AddInput(layer, meta, MaterialGraphBoxes::Color, options.Diffuse.Texture, options.Diffuse.Color, Color::Black, Float2::Zero, &diffuseTextureNode, MAX_uint8, options.Diffuse.ColorAsParameter);
        if (diffuseTextureNode && options.Diffuse.HasAlphaMask)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Mask)], diffuseTextureNode->Boxes[5]);
        }

        // Emissive
        AddInput(layer, meta, MaterialGraphBoxes::Emissive, options.Emissive.Texture, options.Emissive.Color, Color::Black, Float2(0, 200), nullptr, MAX_uint8, options.Emissive.ColorAsParameter, TEXT("EmissiveColor"));

        // Opacity
        AddInput(layer, meta, MaterialGraphBoxes::Opacity, options.Opacity.Texture, options.Opacity.Value, 1.0f, Float2(0, 400));

        // Roughness + Metalness
        AddInput(layer, meta, MaterialGraphBoxes::Roughness, options.Roughness.Texture, options.Roughness.Value, 0.5f, Float2(200, 400), nullptr, options.Roughness.Channel);
        AddInput(layer, meta, MaterialGraphBoxes::Metalness, options.Metalness.Texture, options.Metalness.Value, 0.0f, Float2(200, 600), nullptr, options.Metalness.Channel);

        // Normal
        auto normalMap = AddTextureNode(layer, options.Normals.Texture, true);
        if (normalMap)
        {
            SET_POS(normalMap, Float2(-893.5272f, -200.926111f));
            if (options.Normals.StrengthAsParameter)
            {
                // normalize(float3(normal.xy * strength, normal.z))
                auto strength = AddFloatParameterNode(layer, 1.0f, TEXT("NormalStrength"), Float2(-760.0f, -60.0f));
                auto maskXY = AddMaskXYNode(layer);
                auto maskZ = AddMaskZNode(layer);
                auto scale = AddMultiplyNode(layer);
                auto unpack = AddUnpackFloat2Node(layer);
                auto pack = AddPackFloat3Node(layer);
                auto normalize = AddNormalizeNode(layer);

                CONNECT(maskXY->Boxes[0], normalMap->Boxes[1]);
                CONNECT(maskZ->Boxes[0], normalMap->Boxes[1]);
                CONNECT(scale->Boxes[0], maskXY->Boxes[1]);
                CONNECT(scale->Boxes[1], strength->Boxes[0]);
                CONNECT(unpack->Boxes[0], scale->Boxes[2]);
                CONNECT(pack->Boxes[1], unpack->Boxes[1]); // X
                CONNECT(pack->Boxes[2], unpack->Boxes[2]); // Y
                CONNECT(pack->Boxes[3], maskZ->Boxes[1]);  // Z
                CONNECT(normalize->Boxes[0], pack->Boxes[0]);
                CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Normal)], normalize->Boxes[1]);

                SET_POS(maskXY, Float2(-760.0f, -250.0f));
                SET_POS(maskZ, Float2(-760.0f, -350.0f));
                SET_POS(scale, Float2(-600.0f, -250.0f));
                SET_POS(unpack, Float2(-480.0f, -300.0f));
                SET_POS(pack, Float2(-360.0f, -300.0f));
                SET_POS(normalize, Float2(-240.0f, -300.0f));
            }
            else
            {
                CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Normal)], normalMap->Boxes[1]);
            }
        }

        // Ambient Occlusion
        AddInput(layer, meta, MaterialGraphBoxes::AmbientOcclusion, options.AmbientOcclusion.Texture, options.AmbientOcclusion.Value, 1.0f, Float2(400, 400), nullptr, options.AmbientOcclusion.Channel);

        // Height (displacement)
        AddInput(layer, meta, MaterialGraphBoxes::PositionOffset, options.Height.Texture, options.Height.Value, Color::Black, Float2(400, 600), nullptr, options.Height.Channel);

        MemoryWriteStream stream(512);
        layer->Graph.Save(&stream, true);
        context.Data.Header.Chunks[SHADER_FILE_CHUNK_VISJECT_SURFACE]->Data.Copy(ToSpan(stream));
        Delete(layer);
    }
    else
    {
        // Use default material properties and don't create Visject Surface because during material loading it will be generated
        const Options options;
        shaderHeader.Material.Info = options.Info;
    }
    context.Data.CustomData.Copy(&shaderHeader);

    return CreateAssetResult::Ok;
}

#undef CONNECT
#undef SET_POS

#endif
