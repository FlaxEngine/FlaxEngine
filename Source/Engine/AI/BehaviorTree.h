// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Visject/VisjectGraph.h"

class BehaviorKnowledge;
class BehaviorTree;
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
    friend BehaviorTree;
public:
    // Instance of the graph root node.
    BehaviorTreeRootNode* Root = nullptr;
    // Total count of used nodes.
    int32 NodesCount = 0;
    // Total size of the nodes states memory.
    int32 NodesStatesSize = 0;

    // [VisjectGraph]
    void Clear() override;
    bool onNodeLoaded(Node* n) override;

private:
    void Setup(BehaviorTree* tree);
    void SetupRecursive(Node& node);
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
    /// Gets a specific node instance object from Behavior Tree.
    /// </summary>
    /// <param name="id">The unique node identifier (Visject surface).</param>
    /// <returns>The node instance or null if cannot get it.</returns>
    API_FUNCTION() BehaviorTreeNode* GetNodeInstance(uint32 id);

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The surface data or empty if failed to load it.</returns>
    API_FUNCTION() BytesContainer LoadSurface() const;

#if USE_EDITOR
    /// <summary>
    /// Updates the graph surface (save new one, discard cached data, reload asset).
    /// </summary>
    /// <param name="data">Stream with graph data.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data) const;
#endif

private:
#if USE_EDITOR
    void OnScriptsReloadStart();
    void OnScriptsReloadEnd();
#endif

public:
    // [BinaryAsset]
    void OnScriptingDispose() override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
