// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Visject/VisjectGraph.h"

class BehaviorKnowledge;
class BehaviorTreeNode;
class BehaviorTreeRootNode;

/// <summary>
/// Behavior Tree graph node.
/// </summary>
class BehaviorTreeGraphNode : public VisjectGraphNode<>
{
public:
    // Instance of the graph node.
    BehaviorTreeNode* Instance = nullptr;

    ~BehaviorTreeGraphNode();
};

/// <summary>
/// Behavior Tree graph.
/// </summary>
class BehaviorTreeGraph : public VisjectGraph<BehaviorTreeGraphNode>
{
public:
    // Instance of the graph root node.
    BehaviorTreeRootNode* Root = nullptr;
    // Total count of used nodes.
    int32 NodesCount = 0;
    // Total size of the nodes states memory.
    int32 NodesStatesSize = 0;

    // [VisjectGraph]
    bool Load(ReadStream* stream, bool loadMeta) override;
    void Clear() override;
    bool onNodeLoaded(Node* n) override;

private:
    void LoadRecursive(Node& node);
};

/// <summary>
/// Behavior Tree graph executor runtime.
/// </summary>
class BehaviorTreeExecutor : public VisjectExecutor
{
};

/// <summary>
/// Behavior Tree asset with AI logic graph.
/// </summary>
/// <seealso cref="BinaryAsset" />
API_CLASS(NoSpawn, Sealed) class FLAXENGINE_API BehaviorTree : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(BehaviorTree, 1);

public:
    /// <summary>
    /// The Behavior Tree graph.
    /// </summary>
    BehaviorTreeGraph Graph;

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The surface data or empty if failed to load it.</returns>
    API_FUNCTION() BytesContainer LoadSurface();

#if USE_EDITOR
    /// <summary>
    /// Updates the graph surface (save new one, discard cached data, reload asset).
    /// </summary>
    /// <param name="data">Stream with graph data.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data);
#endif

public:
    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& output) const override;
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
