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

    struct Meta11 // TypeID: 11, for nodes
    {
        Float2 Position;
        bool Selected;
    };

    template<typename T>
    void AddInput(MaterialLayer* layer, Meta11 meta, MaterialGraphBoxes box, const Guid& texture, const T& value, const T& defaultValue, const Float2& pos, ShaderGraphNode<>** outTextureNode = nullptr)
    {
        auto textureNode = AddTextureNode(layer, texture);
        auto valueNode = AddValueNode<T>(layer, value, defaultValue);
        if (textureNode && valueNode)
        {
            auto diffuseMultiply = AddMultiplyNode(layer);
            CONNECT(diffuseMultiply->Boxes[0], textureNode->Boxes[1]);
            CONNECT(diffuseMultiply->Boxes[1], valueNode->Boxes[0]);
            CONNECT(layer->Root->Boxes[static_cast<int32>(box)], diffuseMultiply->Boxes[2]);
            SET_POS(valueNode, pos + Float2(-467.7404, 91.41332));
            SET_POS(textureNode, pos + Float2(-538.096, -103.9724));
            SET_POS(diffuseMultiply, pos + Float2(-293.5272f, -2.926111f));
        }
        else if (textureNode)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(box)], textureNode->Boxes[1]);
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
        AddInput(layer, meta, MaterialGraphBoxes::Color, options.Diffuse.Texture, options.Diffuse.Color, Color::Black, Float2::Zero, &diffuseTextureNode);
        if (diffuseTextureNode && options.Diffuse.HasAlphaMask)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Mask)], diffuseTextureNode->Boxes[5]);
        }

        // Emissive
        AddInput(layer, meta, MaterialGraphBoxes::Emissive, options.Emissive.Texture, options.Emissive.Color, Color::Black, Float2(0, 200));

        // Opacity
        AddInput(layer, meta, MaterialGraphBoxes::Opacity, options.Opacity.Texture, options.Opacity.Value, 1.0f, Float2(0, 400));

        // Opacity
        AddInput(layer, meta, MaterialGraphBoxes::Roughness, options.Roughness.Texture, options.Roughness.Value, 0.5f, Float2(200, 400));

        // Normal
        auto normalMap = AddTextureNode(layer, options.Normals.Texture, true);
        if (normalMap)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Normal)], normalMap->Boxes[1]);
            SET_POS(normalMap, Float2(-893.5272f, -200.926111f));
        }

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
