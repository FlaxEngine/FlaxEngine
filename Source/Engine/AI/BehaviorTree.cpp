// Copyright (c) Wojciech Figat. All rights reserved.

#include "BehaviorTree.h"
#include "BehaviorTreeNode.h"
#include "BehaviorTreeNodes.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Serialization/JsonSerializer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Threading.h"
#include "FlaxEngine.Gen.h"
#if USE_EDITOR
#include "Engine/Level/Level.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#endif

REGISTER_BINARY_ASSET(BehaviorTree, "FlaxEngine.BehaviorTree", false);

#define IS_BT_NODE(n) (n.GroupID == 19 && (n.TypeID == 1 || n.TypeID == 2 || n.TypeID == 3))

bool SortBehaviorTreeChildren(GraphBox* const& a, GraphBox* const& b)
{
    // Sort by node X coordinate on surface
    auto aNode = (BehaviorTreeGraph::Node*)a->Parent;
    auto bNode = (BehaviorTreeGraph::Node*)b->Parent;
    auto aEntry = aNode->Meta.GetEntry(11);
    auto bEntry = bNode->Meta.GetEntry(11);
    auto aX = aEntry && aEntry->Data.HasItems() ? ((Float2*)aEntry->Data.Get())->X : (float)aNode->ID;
    auto bX = bEntry && bEntry->Data.HasItems() ? ((Float2*)bEntry->Data.Get())->X : (float)bNode->ID;
    return aX < bX;
}

BehaviorTreeGraphNode::~BehaviorTreeGraphNode()
{
    SAFE_DELETE(Instance);
}

void BehaviorTreeGraph::Clear()
{
    VisjectGraph<BehaviorTreeGraphNode>::Clear();

    Root = nullptr;
    NodesCount = 0;
    NodesStatesSize = 0;
}

bool BehaviorTreeGraph::onNodeLoaded(Node* n)
{
    const Node& node = *n;
    if (IS_BT_NODE(node))
    {
        // Create node instance object
        ScriptingTypeHandle type = Scripting::FindScriptingType((StringAnsiView)n->Values[0]);
        if (!type)
            type = Scripting::FindScriptingType(StringAnsi((StringView)n->Values[0]));
        if (type)
        {
            n->Instance = (BehaviorTreeNode*)Scripting::NewObject(type);
            const Variant& data = n->Values[1];
            if (data.Type == VariantType::Blob)
                JsonSerializer::LoadFromBytes(n->Instance, Span<byte>((byte*)data.AsBlob.Data, data.AsBlob.Length), FLAXENGINE_VERSION_BUILD);
        }
        else
        {
            const String name = n->Values[0].ToString();
            if (name.HasChars())
                LOG(Error, "Missing type '{0}'", name);
        }
    }

    return VisjectGraph<BehaviorTreeGraphNode>::onNodeLoaded(n);
}

void BehaviorTreeGraph::Setup(BehaviorTree* tree)
{
    // Find root node
    Node* root = nullptr;
    Root = nullptr;
    for (Node& node : Nodes)
    {
        if (node.Instance && node.GroupID == 19 && node.TypeID == 2)
        {
            // Find root node
            if (node.Instance->GetTypeHandle() == BehaviorTreeRootNode::TypeInitializer)
                Root = (BehaviorTreeRootNode*)node.Instance;
            root = &node;
            break;
        }
    }
    if (!Root)
        return;

    // Setup nodes hierarchy
    NodesCount = 0;
    NodesStatesSize = 0;
    SetupRecursive(*root);

    // Init graph with asset
    Root->Init(tree);
}

void BehaviorTreeGraph::SetupRecursive(Node& node)
{
    // Count total states memory size
    ASSERT_LOW_LAYER(node.Instance);
    node.Instance->_memoryOffset = NodesStatesSize;
    node.Instance->_executionIndex = NodesCount;
    NodesStatesSize += node.Instance->GetStateSize();
    NodesCount++;

    if (node.TypeID == 1 && node.Values.Count() >= 3)
    {
        // Load node decorators
        const auto& decoratorIds = node.Values[2];
        if (decoratorIds.Type.Type == VariantType::Blob && decoratorIds.AsBlob.Length)
        {
            const Span<uint32> ids((uint32*)decoratorIds.AsBlob.Data, decoratorIds.AsBlob.Length / sizeof(uint32));
            for (int32 i = 0; i < ids.Length(); i++)
            {
                Node* decorator = GetNode(ids[i]);
                if (decorator && decorator->Instance && decorator->Instance->Is<BehaviorTreeDecorator>())
                {
                    node.Instance->_decorators.Add((BehaviorTreeDecorator*)decorator->Instance);
                    decorator->Instance->_parent = node.Instance;
                    SetupRecursive(*decorator);
                }
            }
        }
    }
    if (auto* nodeCompound = ScriptingObject::Cast<BehaviorTreeCompoundNode>(node.Instance))
    {
        auto& children = node.Boxes[1].Connections;

        // Sort children from left to right (based on placement on a graph surface)
        Sorting::QuickSort(children.Get(), children.Count(), SortBehaviorTreeChildren);

        // Find all children (of output box)
        for (const GraphBox* childBox : children)
        {
            Node* child = childBox ? (Node*)childBox->Parent : nullptr;
            if (child && child->Instance)
            {
                nodeCompound->Children.Add(child->Instance);
                child->Instance->_parent = nodeCompound;
                SetupRecursive(*child);
            }
        }
    }
}

BehaviorTree::BehaviorTree(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

BehaviorTreeNode* BehaviorTree::GetNodeInstance(uint32 id)
{
    for (const auto& node : Graph.Nodes)
    {
        if (node.ID == id && node.Instance && IS_BT_NODE(node))
            return node.Instance;
    }
    return nullptr;
}

BytesContainer BehaviorTree::LoadSurface() const
{
    if (WaitForLoaded())
        return BytesContainer();
    ScopeLock lock(Locker);
    if (!LoadChunks(GET_CHUNK_FLAG(0)))
    {
        const auto data = GetChunk(0);
        BytesContainer result;
        result.Copy(data->Data);
        return result;
    }
    LOG(Warning, "\'{0}\' surface data is missing.", ToString());
    return BytesContainer();
}

#if USE_EDITOR

bool BehaviorTree::SaveSurface(const BytesContainer& data) const
{
    if (OnCheckSave())
        return true;
    ScopeLock lock(Locker);

    // Set Visject Surface data
    GetOrCreateChunk(0)->Data.Copy(data);

    // Save
    AssetInitData assetData;
    assetData.SerializedVersion = 1;
    if (SaveAsset(assetData))
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

void BehaviorTree::OnScriptsReloadStart()
{
    // Include all node instances in hot-reload
    for (BehaviorTreeGraphNode& n : Graph.Nodes)
    {
        Level::ScriptsReloadRegisterObject((ScriptingObject*&)n.Instance);
    }

    // Clear state
    Graph.Root = nullptr;
    Graph.NodesCount = 0;
    Graph.NodesStatesSize = 0;
}

void BehaviorTree::OnScriptsReloadEnd()
{
    // Node instances were restored so update the graph cached structure (root, children, decorators, etc.)
    Graph.Setup(this);
}

void BehaviorTree::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    // Base
    BinaryAsset::GetReferences(assets, files);

    Graph.GetReferences(assets);

    // Extract refs from serialized nodes data
    for (const BehaviorTreeGraphNode& n : Graph.Nodes)
    {
        if (n.Instance == nullptr)
            continue;
        const Variant& data = n.Values[1];
        if (data.Type == VariantType::Blob)
            JsonAssetBase::GetReferences(StringAnsiView((char*)data.AsBlob.Data, data.AsBlob.Length), assets);
    }
}

bool BehaviorTree::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);
    MemoryWriteStream stream;
    if (Graph.Save(&stream, true))
        return true;
    BytesContainer data;
    data.Link(ToSpan(stream));
    return SaveSurface(data);
}

#endif

void BehaviorTree::OnScriptingDispose()
{
    // Dispose any node instances to prevent crashes (scripting is released before content)
    for (BehaviorTreeGraphNode& n : Graph.Nodes)
    {
        SAFE_DELETE(n.Instance);
    }

    BinaryAsset::OnScriptingDispose();
}

Asset::LoadResult BehaviorTree::load()
{
    // Load graph
    const auto surfaceChunk = GetChunk(0);
    if (surfaceChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream surfaceStream(surfaceChunk->Get(), surfaceChunk->Size());
    if (Graph.Load(&surfaceStream, true))
    {
        LOG(Warning, "Failed to load graph \'{0}\'", ToString());
        return LoadResult::Failed;
    }
    Graph.Setup(this);

#if USE_EDITOR
    Level::ScriptsReloadStart.Bind<BehaviorTree, &BehaviorTree::OnScriptsReloadStart>(this);
    Level::ScriptsReloadEnd.Bind<BehaviorTree, &BehaviorTree::OnScriptsReloadEnd>(this);
#endif

    return LoadResult::Ok;
}

void BehaviorTree::unload(bool isReloading)
{
#if USE_EDITOR
    Level::ScriptsReloadStart.Unbind<BehaviorTree, &BehaviorTree::OnScriptsReloadStart>(this);
    Level::ScriptsReloadEnd.Unbind<BehaviorTree, &BehaviorTree::OnScriptsReloadEnd>(this);
#endif

    // Clear resources
    Graph.Clear();
}

AssetChunksFlag BehaviorTree::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
