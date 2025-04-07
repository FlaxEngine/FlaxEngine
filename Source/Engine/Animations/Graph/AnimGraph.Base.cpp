// Copyright (c) Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Content/Assets/SkeletonMask.h"
#include "Engine/Content/Assets/AnimationGraphFunction.h"
#include "Engine/Content/Content.h"
#include "Engine/Utilities/Delaunay2D.h"
#include "Engine/Serialization/MemoryReadStream.h"

AnimSubGraph* AnimGraphBase::LoadSubGraph(const void* data, int32 dataLength, const Char* name)
{
    if (data == nullptr || dataLength == 0)
    {
        // No graph
        return nullptr;
    }
    name = name ? name : TEXT("?");

    // Allocate graph
    // TODO: use shared allocations for graphs? eg. block allocator in AnimGraph for better performance
    auto subGraph = New<AnimSubGraph>(_graph);

    // Load graph
    MemoryReadStream stream((const byte*)data, dataLength);
    if (subGraph->Load(&stream, false))
    {
        // Load failed
        LOG(Warning, "Failed to load sub graph {0}.", name);
        return nullptr;
    }

    // Done
    BucketsCountTotal += subGraph->BucketsCountTotal;
    SubGraphs.Add(subGraph);
    return subGraph;
}

bool AnimGraphBase::Load(ReadStream* stream, bool loadMeta)
{
    ASSERT(_graph);
    _rootNode = nullptr;
    BucketsCountSelf = 0;
    BucketsCountTotal = 0;
    BucketsStart = _graph->_bucketsCounter;

    // Base
    if (VisjectGraph::Load(stream, loadMeta))
        return true;

    BucketsCountTotal += BucketsCountSelf;

    return false;
}

void AnimGraphBase::Clear()
{
    // Release memory
    SubGraphs.ClearDelete();
    StateTransitions.Resize(0);

    // Base
    GraphType::Clear();
}

#if USE_EDITOR

void AnimGraphBase::GetReferences(Array<Guid>& output) const
{
    GraphType::GetReferences(output);

    // Collect references from nested graph (assets used in state machines)
    for (const auto* subGraph : SubGraphs)
    {
        subGraph->GetReferences(output);
    }
}

#endif

void AnimationBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    bucket.Animation.TimePosition = 0.0f;
    bucket.Animation.LastUpdateFrame = 0;
}

void MultiBlendBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    Platform::MemoryClear(&bucket.MultiBlend, sizeof(bucket.MultiBlend));
}

void BlendPoseBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    bucket.BlendPose.TransitionPosition = 0.0f;
    bucket.BlendPose.PreviousBlendPoseIndex = -1;
}

void StateMachineBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    Platform::MemoryClear(&bucket.StateMachine, sizeof(bucket.StateMachine));
}

void SlotBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    bucket.Slot.Index = -1;
    bucket.Slot.TimePosition = 0.0f;
    bucket.Slot.BlendInPosition = 0.0f;
    bucket.Slot.BlendOutPosition = 0.0f;
    bucket.Slot.LoopsDone = 0;
    bucket.Slot.LoopsLeft = 0;
}

void InstanceDataBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    bucket.InstanceData.Init = true;
}

bool SortMultiBlend1D(const ANIM_GRAPH_MULTI_BLEND_INDEX& a, const ANIM_GRAPH_MULTI_BLEND_INDEX& b, AnimGraphNode* n)
{
    // Sort items by X location from the lowest to the highest
    const auto aX = a == ANIM_GRAPH_MULTI_BLEND_INVALID ? MAX_float : n->Values[4 + a * 2].AsFloat4().X;
    const auto bX = b == ANIM_GRAPH_MULTI_BLEND_INVALID ? MAX_float : n->Values[4 + b * 2].AsFloat4().X;
    return aX < bX;
}

#define ADD_BUCKET(initializer) \
	BucketsCountSelf++; \
	n->BucketIndex = _graph->_bucketsCounter++; \
	_graph->_bucketInitializerList.Add(initializer)

bool AnimGraphBase::onNodeLoaded(Node* n)
{
    ((AnimGraphNode*)n)->Graph = _graph;
    switch (n->GroupID)
    {
    // Tools
    case 7:
        switch (n->TypeID)
        {
        // Time
        case 5:
            ADD_BUCKET(AnimationBucketInit);
            break;
        }
        break;
    // Animation
    case 9:
        switch (n->TypeID)
        {
        // Output
        case 1:
            _rootNode = n;
            if (_rootNode->Values.Count() < 1)
            {
                _rootNode->Values.Resize(1);
                _rootNode->Values[0] = (int32)RootMotionExtraction::NoExtraction;
            }
            break;
        // Animation
        case 2:
            ADD_BUCKET(AnimationBucketInit);
            n->Assets.Resize(1);
            n->Assets[0] = (Asset*)Content::LoadAsync<Animation>((Guid)n->Values[0]);
            break;
        // Blend with Mask
        case 11:
            n->Assets.Resize(1);
            n->Assets[0] = (Asset*)Content::LoadAsync<SkeletonMask>((Guid)n->Values[1]);
            break;
        // Multi Blend 1D
        case 12:
            ADD_BUCKET(MultiBlendBucketInit);
            n->Data.MultiBlend1D.Count = (ANIM_GRAPH_MULTI_BLEND_INDEX)((n->Values.Count() - 4) / 2); // 4 node values + 2 per blend point
            n->Data.MultiBlend1D.Length = -1;
            n->Data.MultiBlend1D.IndicesSorted = (ANIM_GRAPH_MULTI_BLEND_INDEX*)Allocator::Allocate(sizeof(ANIM_GRAPH_MULTI_BLEND_INDEX) * n->Data.MultiBlend1D.Count);
            n->Assets.Resize(n->Data.MultiBlend1D.Count);
            for (int32 i = 0; i < n->Data.MultiBlend1D.Count; i++)
            {
                n->Assets[i] = Content::LoadAsync<Animation>((Guid)n->Values[i * 2 + 5]);
                n->Data.MultiBlend1D.IndicesSorted[i] = (ANIM_GRAPH_MULTI_BLEND_INDEX)(n->Assets[i] ? i : ANIM_GRAPH_MULTI_BLEND_INVALID);
            }
            Sorting::SortArray(n->Data.MultiBlend1D.IndicesSorted, n->Data.MultiBlend1D.Count, &SortMultiBlend1D, n);
            break;
        // Multi Blend 2D
        case 13:
        {
            ADD_BUCKET(MultiBlendBucketInit);
            n->Data.MultiBlend1D.Count = (ANIM_GRAPH_MULTI_BLEND_INDEX)((n->Values.Count() - 4) / 2); // 4 node values + 2 per blend point
            n->Data.MultiBlend2D.Length = -1;

            // Get blend points locations
            Array<Float2> vertices;
            Array<ANIM_GRAPH_MULTI_BLEND_INDEX> vertexToAnim;
            n->Assets.Resize(n->Data.MultiBlend1D.Count);
            for (int32 i = 0; i < n->Data.MultiBlend1D.Count; i++)
            {
                n->Assets[i] = Content::LoadAsync<Animation>((Guid)n->Values[i * 2 + 5]);
                if (n->Assets[i])
                {
                    vertices.Add(Float2(n->Values[i * 2 + 4].AsFloat4()));
                    vertexToAnim.Add((ANIM_GRAPH_MULTI_BLEND_INDEX)i);
                }
            }

            // Triangulate
            Array<Delaunay2D::Triangle> triangles;
            Delaunay2D::Triangulate(vertices, triangles);
            if (triangles.Count() == 0)
            {
                // Insert dummy triangles to have something working (eg. blend points are on the same axis)
                int32 verticesLeft = vertices.Count();
                while (verticesLeft >= 3)
                {
                    verticesLeft -= 3;
                    triangles.Add(Delaunay2D::Triangle(verticesLeft, verticesLeft + 1, verticesLeft + 2));
                }
                if (verticesLeft == 1)
                    triangles.Add(Delaunay2D::Triangle(0, 0, 0));
                else if (verticesLeft == 2)
                    triangles.Add(Delaunay2D::Triangle(0, 1, 0));
            }

            // Store triangles vertices indices (map the back to the anim node slots)
            n->Data.MultiBlend2D.TrianglesCount = triangles.Count();
            n->Data.MultiBlend2D.Triangles = (ANIM_GRAPH_MULTI_BLEND_INDEX*)Allocator::Allocate(triangles.Count() * 3 * sizeof(ANIM_GRAPH_MULTI_BLEND_INDEX));
            for (int32 i = 0, t = 0; i < triangles.Count(); i++)
            {
                n->Data.MultiBlend2D.Triangles[t++] = vertexToAnim[triangles[i].Indices[0]];
                n->Data.MultiBlend2D.Triangles[t++] = vertexToAnim[triangles[i].Indices[1]];
                n->Data.MultiBlend2D.Triangles[t++] = vertexToAnim[triangles[i].Indices[2]];
            }
            break;
        }
        // Blend Pose
        case 14:
        {
            ADD_BUCKET(BlendPoseBucketInit);
            break;
        }
        // State Machine
        case 18:
        {
            ADD_BUCKET(StateMachineBucketInit);

            // Load the graph
            auto& data = n->Data.StateMachine;
            const Value& name = n->Values[0];
            Value& surfaceData = n->Values[1];
            data.Graph = LoadSubGraph(surfaceData.AsBlob.Data, surfaceData.AsBlob.Length, (const Char*)name.AsBlob.Data);

            // Release data to don't use that memory
            surfaceData = Value::Null;

            break;
        }
        // Entry
        case 19:
        {
            _rootNode = GetNode((int32)n->Values[0]);
            break;
        }
        // State
        case 20:
        {
            // Load the graph
            auto& data = n->Data.State;
            const Value& name = n->Values[0];
            Value& surfaceData = n->Values[1];
            data.Graph = LoadSubGraph(surfaceData.AsBlob.Data, surfaceData.AsBlob.Length, (const Char*)name.AsBlob.Data);

            // Release data to don't use that memory
            surfaceData = Value::Null;

            // Initialize transitions
            LoadStateTransitions(data, n->Values[2]);

            break;
        }
        // State Output
        case 21:
        {
            _rootNode = n;
            break;
        }
        // Rule Output
        case 22:
        {
            _rootNode = n;
            break;
        }
        // Animation Graph Function
        case 24:
        {
            auto& data = n->Data.AnimationGraphFunction;

            // Load function asset
            const auto function = Content::LoadAsync<AnimationGraphFunction>((Guid)n->Values[0]);
            if (!function || function->WaitForLoaded())
            {
                data.Graph = nullptr;
                break;
            }
            n->Assets.Resize(1);
            n->Assets[0] = function;

            // Load the graph
            auto graphData = function->LoadSurface();
            data.Graph = LoadSubGraph(graphData.Get(), graphData.Length(), TEXT("Animation Graph Function"));
            break;
        }
        // Transform Node (local/model space)
        // Get Node Transform (local/model space)
        // IK Aim, Two Bone IK
        case 25:
        case 26:
        case 28:
        case 29:
        case 30:
        case 31:
        {
            auto& data = n->Data.TransformNode;
            if (_graph->BaseModel && !_graph->BaseModel->WaitForLoaded())
                data.NodeIndex = _graph->BaseModel->FindNode((StringView)n->Values[0]);
            else
                data.NodeIndex = -1;
            break;
        }
        // Copy Node
        case 27:
        {
            auto& data = n->Data.CopyNode;
            if (_graph->BaseModel && !_graph->BaseModel->WaitForLoaded())
            {
                data.SrcNodeIndex = _graph->BaseModel->FindNode((StringView)n->Values[0]);
                data.DstNodeIndex = _graph->BaseModel->FindNode((StringView)n->Values[1]);
            }
            else
            {
                data.SrcNodeIndex = -1;
                data.DstNodeIndex = -1;
            }
            break;
        }
        // Animation Slot
        case 32:
            ADD_BUCKET(SlotBucketInit);
            break;
        // Animation Instance Data
        case 33:
            ADD_BUCKET(InstanceDataBucketInit);
            break;
        // Any State
        case 34:
            LoadStateTransitions(n->Data.AnyState, n->Values[0]);
            break;
        }
        break;
    // Custom
    case 13:
    {
        // Clear data
        auto& data = n->Data.Custom;
        data.Evaluate = nullptr;
        data.Handle = 0;

        // Register node
        _graph->_customNodes.Add(n);

        // Try init node for the first time
        _graph->InitCustomNode(n);
        break;
    }
    }

    return VisjectGraph::onNodeLoaded(n);
}

void AnimGraphBase::LoadStateTransitions(AnimGraphNode::StateBaseData& data, Value& transitionsData)
{
    int32 validTransitions = 0;
    data.Transitions = nullptr;
    if (transitionsData.Type == VariantType::Blob && transitionsData.AsBlob.Length)
    {
        MemoryReadStream stream((byte*)transitionsData.AsBlob.Data, transitionsData.AsBlob.Length);

        int32 version;
        stream.ReadInt32(&version);
        if (version != 1)
        {
            LOG(Warning, "Invalid version of the Anim Graph state transitions data.");
            return;
        }

        int32 transitionsCount;
        stream.ReadInt32(&transitionsCount);
        if (transitionsCount == 0)
            return;

        StateTransitions.EnsureCapacity(StateTransitions.Count() + transitionsCount);
        data.Transitions = (uint16*)Allocator::Allocate(transitionsCount * sizeof(uint16));

        AnimGraphStateTransition transition;
        for (int32 i = 0; i < transitionsCount; i++)
        {
            // Must match StateMachineTransition.Data in C#
            struct Data
            {
                int32 Destination;
                int32 Flags;
                int32 Order;
                float BlendDuration;
                int32 BlendMode;
                int32 Unused0;
                int32 Unused1;
                int32 Unused2;
            };
            Data transitionData;
            stream.ReadBytes(&transitionData, sizeof(transitionData));

            transition.Flags = (AnimGraphStateTransition::FlagTypes)transitionData.Flags;
            transition.BlendDuration = transitionData.BlendDuration;
            transition.BlendMode = (AlphaBlendMode)transitionData.BlendMode;
            transition.Destination = GetNode(transitionData.Destination);
            transition.RuleGraph = nullptr;

            int32 ruleSize;
            stream.ReadInt32(&ruleSize);
            const auto ruleBytes = (byte*)stream.Move(ruleSize);

            if (static_cast<int32>(transition.Flags & AnimGraphStateTransition::FlagTypes::Enabled) == 0)
            {
                // Skip disabled transitions
                continue;
            }
            if (ruleSize != 0)
            {
                transition.RuleGraph = LoadSubGraph(ruleBytes, ruleSize, TEXT("Rule"));
                if (transition.RuleGraph && transition.RuleGraph->GetRootNode() == nullptr)
                {
                    LOG(Warning, "Missing root node for the state machine transition rule graph.");
                    continue;
                }
            }
            if (transition.Destination == nullptr)
            {
                LOG(Warning, "Missing target node for the state machine transition.");
                continue;
            }

            data.Transitions[validTransitions++] = (uint16)StateTransitions.Count();
            StateTransitions.Add(transition);
        }

        // Last entry is invalid to indicate end
        data.Transitions[validTransitions] = AnimGraphNode::StateData::InvalidTransitionIndex;
    }

    // Release data to don't use that memory
    transitionsData = AnimGraphExecutor::Value::Null;
}

#undef ADD_BUCKET
