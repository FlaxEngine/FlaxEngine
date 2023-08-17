// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "BehaviorTree.h"
#include "BehaviorTreeNode.h"
#include "BehaviorTreeNodes.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Serialization/JsonSerializer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Threading.h"
#include "FlaxEngine.Gen.h"

REGISTER_BINARY_ASSET(BehaviorTree, "FlaxEngine.BehaviorTree", false);

BehaviorTreeGraphNode::~BehaviorTreeGraphNode()
{
    SAFE_DELETE(Instance);
}

bool BehaviorTreeGraph::Load(ReadStream* stream, bool loadMeta)
{
    if (VisjectGraph<BehaviorTreeGraphNode>::Load(stream, loadMeta))
        return true;

    // Build node instances hierarchy
    for (Node& node : Nodes)
    {
        if (auto* nodeCompound = ScriptingObject::Cast<BehaviorTreeCompoundNode>(node.Instance))
        {
            for (const GraphBox* childBox : node.Boxes[1].Connections)
            {
                const Node* child = childBox ? (Node*)childBox->Parent : nullptr;
                if (child && child->Instance)
                {
                    nodeCompound->Children.Add(child->Instance);
                }
            }
        }
        if (node.Instance)
        {
            // Count total states memory size
            node.Instance->_memoryOffset = NodesStatesSize;
            NodesStatesSize += node.Instance->GetStateSize();
        }
    }

    return false;
}

void BehaviorTreeGraph::Clear()
{
    VisjectGraph<BehaviorTreeGraphNode>::Clear();

    Root = nullptr;
    NodesStatesSize = 0;
}

bool BehaviorTreeGraph::onNodeLoaded(Node* n)
{
    if (n->GroupID == 19 && (n->TypeID == 1 || n->TypeID == 2))
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

            // Find root node
            if (!Root && n->Instance && BehaviorTreeRootNode::TypeInitializer == type)
                Root = (BehaviorTreeRootNode*)n->Instance;
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

BehaviorTree::BehaviorTree(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

BytesContainer BehaviorTree::LoadSurface()
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

bool BehaviorTree::SaveSurface(const BytesContainer& data)
{
    // Wait for asset to be loaded or don't if last load failed
    if (LastLoadFailed())
    {
        LOG(Warning, "Saving asset that failed to load.");
    }
    else if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }

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

#endif

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

    // Init graph
    if (Graph.Root)
    {
        Graph.Root->Init(this);
    }

    return LoadResult::Ok;
}

void BehaviorTree::unload(bool isReloading)
{
    // Clear resources
    Graph.Clear();
}

AssetChunksFlag BehaviorTree::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
