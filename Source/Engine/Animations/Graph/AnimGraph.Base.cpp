// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    MemoryReadStream stream((byte*)data, dataLength);
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
    bucket.MultiBlend.TimePosition = 0.0f;
    bucket.MultiBlend.LastUpdateFrame = 0;
}

void BlendPoseBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    bucket.BlendPose.TransitionPosition = 0.0f;
    bucket.BlendPose.PreviousBlendPoseIndex = -1;
}

void StateMachineBucketInit(AnimGraphInstanceData::Bucket& bucket)
{
    bucket.StateMachine.LastUpdateFrame = 0;
    bucket.StateMachine.CurrentState = nullptr;
    bucket.StateMachine.ActiveTransition = nullptr;
    bucket.StateMachine.TransitionPosition = 0.0f;
}

bool SortMultiBlend1D(const byte& a, const byte& b, AnimGraphNode* n)
{
    // Sort items by X location from the lowest to the highest
    const auto aX = a == ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS ? MAX_float : n->Values[4 + a * 2].AsVector4().X;
    const auto bX = b == ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS ? MAX_float : n->Values[4 + b * 2].AsVector4().X;
    return aX < bX;
}

#define ADD_BUCKET(initializer) \
	BucketsCountSelf++; \
	n->BucketIndex = _graph->_bucketsCounter++; \
	_graph->_bucketInitializerList.Add(initializer)

bool AnimGraphBase::onNodeLoaded(Node* n)
{
    ((AnimGraphNode*)n)->Graph = _graph;

    // Check if this node needs a state container
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
                _rootNode->Values[0] = (int32)RootMotionMode::NoExtraction;
            }
            break;
            // Animation
        case 2:
            ADD_BUCKET(AnimationBucketInit);
            n->Assets[0] = (Asset*)Content::LoadAsync<Animation>((Guid)n->Values[0]);
            break;
            // Blend with Mask
        case 11:
            n->Assets[0] = (Asset*)Content::LoadAsync<SkeletonMask>((Guid)n->Values[1]);
            break;
            // Multi Blend 1D
        case 12:
        {
            ADD_BUCKET(MultiBlendBucketInit);
            n->Data.MultiBlend1D.Length = -1;
            const Vector4 range = n->Values[0].AsVector4();
            for (int32 i = 0; i < ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS; i++)
            {
                auto data0 = n->Values[i * 2 + 4].AsVector4();
                data0.X = Math::Clamp(data0.X, range.X, range.Y);
                n->Assets[i] = Content::LoadAsync<Animation>((Guid)n->Values[i * 2 + 5]);
                n->Data.MultiBlend1D.IndicesSorted[i] = n->Assets[i] ? i : ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS;
            }
            Sorting::SortArray(n->Data.MultiBlend1D.IndicesSorted, ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS, &SortMultiBlend1D, n);
            break;
        }
            // Multi Blend 2D
        case 13:
        {
            ADD_BUCKET(MultiBlendBucketInit);
            n->Data.MultiBlend2D.Length = -1;

            // Get blend points locations
            Array<Vector2, FixedAllocation<ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS + 3>> vertices;
            byte vertexIndexToAnimIndex[ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS];
            const Vector4 range = n->Values[0].AsVector4();
            for (int32 i = 0; i < ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS; i++)
            {
                auto data0 = n->Values[i * 2 + 4].AsVector4();
                data0.X = Math::Clamp(data0.X, range.X, range.Y);
                data0.Y = Math::Clamp(data0.Y, range.Z, range.W);
                n->Assets[i] = (Asset*)Content::LoadAsync<Animation>((Guid)n->Values[i * 2 + 5]);
                if (n->Assets[i])
                {
                    const int32 vertexIndex = vertices.Count();
                    vertexIndexToAnimIndex[vertexIndex] = i;
                    vertices.Add(Vector2(n->Values[i * 2 + 4].AsVector4()));
                }
                else
                {
                    vertexIndexToAnimIndex[i] = -1;
                }
            }

            // Triangulate
            Array<Delaunay2D::Triangle, FixedAllocation<ANIM_GRAPH_MULTI_BLEND_2D_MAX_TRIS>> triangles;
            Delaunay2D::Triangulate(vertices, triangles);

            // Store triangles vertices indices (map the back to the anim node slots)
            for (int32 i = 0; i < triangles.Count(); i++)
            {
                n->Data.MultiBlend2D.TrianglesP0[i] = vertexIndexToAnimIndex[triangles[i].Indices[0]];
                n->Data.MultiBlend2D.TrianglesP1[i] = vertexIndexToAnimIndex[triangles[i].Indices[1]];
                n->Data.MultiBlend2D.TrianglesP2[i] = vertexIndexToAnimIndex[triangles[i].Indices[2]];
            }
            if (triangles.Count() < ANIM_GRAPH_MULTI_BLEND_2D_MAX_TRIS)
                n->Data.MultiBlend2D.TrianglesP0[triangles.Count()] = ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS;

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
            const auto entryTargetId = (int32)n->Values[0];
            const auto entryTarget = GetNode(entryTargetId);
            if (entryTarget == nullptr)
            {
                LOG(Warning, "Missing Entry node in animation state machine graph.");
            }
            _rootNode = entryTarget;
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

            // Initialize transitions
            Value& transitionsData = n->Values[2];
            int32 validTransitions = 0;
            if (transitionsData.Type == VariantType::Blob && transitionsData.AsBlob.Length)
            {
                MemoryReadStream stream((byte*)transitionsData.AsBlob.Data, transitionsData.AsBlob.Length);

                int32 version;
                stream.ReadInt32(&version);
                if (version != 1)
                {
                    LOG(Warning, "Invalid version of the Anim Graph state transitions data.");
                    return true;
                }

                int32 transitionsCount;
                stream.ReadInt32(&transitionsCount);

                StateTransitions.EnsureCapacity(StateTransitions.Count() + transitionsCount);

                AnimGraphStateTransition transition;
                for (int32 i = 0; i < transitionsCount; i++)
                {
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
                    const auto ruleBytes = (byte*)stream.Read(ruleSize);

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

                    if (validTransitions == ANIM_GRAPH_MAX_STATE_TRANSITIONS)
                    {
                        LOG(Warning, "State uses too many transitions.");
                        continue;
                    }

                    data.Transitions[validTransitions++] = (uint16)StateTransitions.Count();
                    StateTransitions.Add(transition);
                }
            }
            if (validTransitions != ANIM_GRAPH_MAX_STATE_TRANSITIONS)
                data.Transitions[validTransitions] = AnimGraphNode::StateData::InvalidTransitionIndex;

            // Release data to don't use that memory
            surfaceData = Value::Null;
            transitionsData = Value::Null;

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

#undef ADD_BUCKET
