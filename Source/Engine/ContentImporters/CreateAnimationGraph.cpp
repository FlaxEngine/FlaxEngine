// Copyright (c) Wojciech Figat. All rights reserved.

#include "CreateAnimationGraph.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Content/Assets/AnimationGraph.h"
#include "Engine/Serialization/MemoryWriteStream.h"

CreateAssetResult CreateAnimationGraph::Create(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(AnimationGraph, 1);

    AnimGraph graph(nullptr);

    // Create empty surface with animation output node
    graph.Nodes.Resize(1);
    auto& rootNode = graph.Nodes[0];
    rootNode.Type = GRAPH_NODE_MAKE_TYPE(9, 1);
    rootNode.ID = 1;
    rootNode.Values.Resize(1);
    rootNode.Values[0] = (int32)RootMotionExtraction::NoExtraction;
    rootNode.Boxes.Resize(1);
    rootNode.Boxes[0] = AnimGraphBox(&rootNode, 0, VariantType::Void);

    // Add parameter (hidden) used to pass the skinned model asset (skeleton source)
    graph.Parameters.Resize(1);
    AnimGraphParameter& baseModelParam = graph.Parameters[0];
    baseModelParam.Identifier = ANIM_GRAPH_PARAM_BASE_MODEL_ID;
    baseModelParam.Type = VariantType::Asset;
    baseModelParam.IsPublic = false;
    baseModelParam.Value = Guid::Empty;

    // Serialize
    MemoryWriteStream stream(256);
    if (graph.Save(&stream, true))
        return CreateAssetResult::Error;

    // Copy to asset chunk
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

    return CreateAssetResult::Ok;
}

#endif
