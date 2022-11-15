// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "CreateMaterial.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#define COMPILE_WITH_MATERIAL_GRAPH 1
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Tools/MaterialGenerator/Types.h"
#include "Engine/Tools/MaterialGenerator/MaterialLayer.h"
#include "Engine/Tools/MaterialGenerator/MaterialGenerator.h"
#include "Engine/Serialization/MemoryWriteStream.h"

namespace
{
    ShaderGraphNode<>* AddFloatValue(MaterialLayer* layer, const float& value, const float& defaultValue)
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

    ShaderGraphNode<>* AddColorNode(MaterialLayer* layer, const Color& value, const Color& defaultValue)
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
    Info.MinDepth = 0.0f;
    Info.MaxDepth = 1.0f;
}

CreateAssetResult CreateMaterial::Create(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(Material, 21);
    context.SkipMetadata = true;

    ShaderStorage::Header21 shaderHeader;
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
#define SET_POS(node, pos) meta.Position = pos; node->Meta.AddEntry(11, (byte*)&meta, sizeof(meta));
#define CONNECT(boxA, boxB) boxA.Connections.Add(&boxB); boxB.Connections.Add(&boxA)
        auto diffuseTexture = AddTextureNode(layer, options.Diffuse.Texture);
        auto diffuseColor = AddColorNode(layer, options.Diffuse.Color, Color::White);
        if (diffuseTexture && diffuseColor)
        {
            auto diffuseMultiply = AddMultiplyNode(layer);
            CONNECT(diffuseMultiply->Boxes[0], diffuseTexture->Boxes[1]);
            CONNECT(diffuseMultiply->Boxes[1], diffuseColor->Boxes[0]);
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Color)], diffuseMultiply->Boxes[2]);
            SET_POS(diffuseColor, Float2(-467.7404, 91.41332));
            SET_POS(diffuseTexture, Float2(-538.096, -103.9724));
            SET_POS(diffuseMultiply, Float2(-293.5272f, -2.926111f));
        }
        else if (diffuseTexture)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Color)], diffuseTexture->Boxes[1]);
            SET_POS(diffuseTexture, Float2(-293.5272f, -2.926111f));
        }
        else if (diffuseColor)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Color)], diffuseColor->Boxes[0]);
            SET_POS(diffuseColor, Float2(-293.5272f, -2.926111f));
        }
        if (diffuseTexture && options.Diffuse.HasAlphaMask)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Mask)], diffuseTexture->Boxes[5]);
        }
        auto emissiveTexture = AddTextureNode(layer, options.Emissive.Texture);
        auto emissiveColor = AddColorNode(layer, options.Emissive.Color, Color::Transparent);
        if (emissiveTexture && emissiveColor)
        {
            auto emissiveMultiply = AddMultiplyNode(layer);
            CONNECT(emissiveMultiply->Boxes[0], emissiveTexture->Boxes[1]);
            CONNECT(emissiveMultiply->Boxes[1], emissiveColor->Boxes[0]);
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Emissive)], emissiveMultiply->Boxes[2]);
            SET_POS(emissiveTexture, Float2(-667.7404, 91.41332));
            SET_POS(emissiveTexture, Float2(-738.096, -103.9724));
            SET_POS(emissiveMultiply, Float2(-493.5272f, -2.926111f));
        }
        else if (emissiveTexture)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Emissive)], emissiveTexture->Boxes[1]);
            SET_POS(emissiveTexture, Float2(-493.5272f, -2.926111f));
        }
        else if (emissiveColor)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Emissive)], emissiveColor->Boxes[0]);
            SET_POS(emissiveColor, Float2(-493.5272f, -2.926111f));
        }
        auto normalMap = AddTextureNode(layer, options.Normals.Texture, true);
        if (normalMap)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Normal)], normalMap->Boxes[1]);
            SET_POS(normalMap, Float2(-893.5272f, -200.926111f));
        }
        auto opacityTexture = AddTextureNode(layer, options.Opacity.Texture);
        auto opacityValue = AddFloatValue(layer, options.Opacity.Value, 1.0f);
        if (opacityTexture && opacityValue)
        {
            auto opacityMultiply = AddMultiplyNode(layer);
            CONNECT(opacityMultiply->Boxes[0], opacityTexture->Boxes[1]);
            CONNECT(opacityMultiply->Boxes[1], opacityValue->Boxes[0]);
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Opacity)], opacityMultiply->Boxes[2]);
            SET_POS(opacityTexture, Float2(-867.7404, 91.41332));
            SET_POS(opacityTexture, Float2(-938.096, -103.9724));
            SET_POS(opacityMultiply, Float2(-693.5272f, -2.926111f));
        }
        else if (opacityTexture)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Opacity)], opacityTexture->Boxes[1]);
            SET_POS(opacityTexture, Float2(-693.5272f, -2.926111f));
        }
        else if (opacityValue)
        {
            CONNECT(layer->Root->Boxes[static_cast<int32>(MaterialGraphBoxes::Opacity)], opacityValue->Boxes[0]);
            SET_POS(opacityValue, Float2(-693.5272f, -2.926111f));
        }
#undef CONNECT
        MemoryWriteStream stream(512);
        layer->Graph.Save(&stream, true);
        context.Data.Header.Chunks[SHADER_FILE_CHUNK_VISJECT_SURFACE]->Data.Copy(stream.GetHandle(), stream.GetPosition());
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

#endif
