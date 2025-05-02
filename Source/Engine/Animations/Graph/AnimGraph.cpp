// Copyright (c) Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Animations/Animations.h"
#include "Engine/Animations/AnimEvent.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include "Engine/Scripting/Scripting.h"

extern void RetargetSkeletonPose(const SkeletonData& sourceSkeleton, const SkeletonData& targetSkeleton, const SkinnedModel::SkeletonMapping& mapping, const Transform* sourceNodes, Transform* targetNodes);

ThreadLocal<AnimGraphContext*> AnimGraphExecutor::Context;

Transform AnimGraphImpulse::GetNodeModelTransformation(SkeletonData& skeleton, int32 nodeIndex) const
{
    const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
    if (parentIndex == -1)
    {
        return Nodes[nodeIndex];
    }

    const Transform parentTransform = GetNodeModelTransformation(skeleton, parentIndex);
    return parentTransform.LocalToWorld(Nodes[nodeIndex]);
}

void AnimGraphImpulse::SetNodeModelTransformation(SkeletonData& skeleton, int32 nodeIndex, const Transform& value)
{
    const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
    if (parentIndex == -1)
    {
        Nodes[nodeIndex] = value;
        return;
    }

    const Transform parentTransform = GetNodeModelTransformation(skeleton, parentIndex);
    parentTransform.WorldToLocal(value, Nodes[nodeIndex]);
}

void AnimGraphInstanceData::Clear()
{
    ClearState();
    Slots.Clear();
    Parameters.Resize(0);
}

void AnimGraphInstanceData::ClearState()
{
    for (const auto& e : ActiveEvents)
        OutgoingEvents.Add(e.End((AnimatedModel*)Object));
    ActiveEvents.Clear();
    InvokeAnimEvents();
    Version = 0;
    LastUpdateTime = -1;
    CurrentFrame = 0;
    RootTransform = Transform::Identity;
    RootMotion = Transform::Identity;
    State.Resize(0);
    NodesPose.Resize(0);
    TraceEvents.Clear();
}

void AnimGraphInstanceData::Invalidate()
{
    LastUpdateTime = -1;
    CurrentFrame = 0;
}

void AnimGraphInstanceData::InvokeAnimEvents()
{
    const bool all = IsInMainThread();
    for (int32 i = 0; i < OutgoingEvents.Count(); i++)
    {
        const OutgoingEvent e = OutgoingEvents[i];
        if (all || e.Instance->Async)
        {
            OutgoingEvents.RemoveAtKeepOrder(i);
            switch (e.Type)
            {
            case OutgoingEvent::OnEvent:
                e.Instance->OnEvent(e.Actor, e.Anim, e.Time, e.DeltaTime);
                break;
            case OutgoingEvent::OnBegin:
                ((AnimContinuousEvent*)e.Instance)->OnBegin(e.Actor, e.Anim, e.Time, e.DeltaTime);
                break;
            case OutgoingEvent::OnEnd:
                ((AnimContinuousEvent*)e.Instance)->OnEnd(e.Actor, e.Anim, e.Time, e.DeltaTime);
                break;
            }
        }
    }
}

AnimGraphInstanceData::OutgoingEvent AnimGraphInstanceData::ActiveEvent::End(AnimatedModel* actor) const
{
    OutgoingEvent out;
    out.Instance = Instance;
    out.Actor = actor;
    out.Anim = Anim;
    out.Time = 0.0f;
    out.DeltaTime = 0.0f;
    out.Type = OutgoingEvent::OnEnd;
    return out;
}

AnimGraphNode::~AnimGraphNode()
{
    // Free allocated memory
    switch (GroupID)
    {
    // Animation
    case 9:
        switch (TypeID)
        {
        // Multi Blend 1D
        case 12:
            Allocator::Free(Data.MultiBlend1D.IndicesSorted);
            break;
        // Multi Blend 2D
        case 13:
            Allocator::Free(Data.MultiBlend2D.Triangles);
            break;
        // State
        case 20:
        // Any State
        case 34:
            Allocator::Free(Data.State.Transitions);
            break;
        }
        break;
    }
}

AnimGraphImpulse* AnimGraphNode::GetNodes(AnimGraphExecutor* executor)
{
    auto& context = *AnimGraphExecutor::Context.Get();
    const int32 count = executor->_skeletonNodesCount;
    if (context.PoseCacheSize == context.PoseCache.Count())
        context.PoseCache.AddOne();
    auto& nodes = context.PoseCache[context.PoseCacheSize++];
    nodes.Nodes.Resize(count, false);
    return &nodes;
}

bool AnimGraph::Load(ReadStream* stream, bool loadMeta)
{
    Version++;
    _bucketsCounter = 0;
    _bucketInitializerList.Clear();
    _customNodes.Clear();

    // Base
    if (AnimGraphBase::Load(stream, loadMeta))
        return true;

    if (!_isFunction)
    {
        // Check if has a proper parameters setup
        if (Parameters.IsEmpty())
        {
            LOG(Warning, "Missing Animation Graph parameters.");
            return true;
        }
        if (_rootNode == nullptr)
        {
            LOG(Warning, "Missing Animation Graph output node.");
            return true;
        }
        if (BaseModel == nullptr)
        {
            LOG(Warning, "Missing Base Model asset for the Animation Graph. Animation won't be played.");
        }
    }

    // Register for scripts reloading events (only if using any custom nodes)
    // Handle load event always because anim graph asset may be loaded before game scripts
    if (_customNodes.HasItems() && !_isRegisteredForScriptingEvents)
    {
        _isRegisteredForScriptingEvents = true;
#if USE_EDITOR
        Scripting::ScriptsReloading.Bind<AnimGraph, &AnimGraph::OnScriptsReloading>(this);
        Scripting::ScriptsReloaded.Bind<AnimGraph, &AnimGraph::OnScriptsReloaded>(this);
#endif
        Scripting::ScriptsLoaded.Bind<AnimGraph, &AnimGraph::OnScriptsLoaded>(this);
    }

    return false;
}

AnimGraph::~AnimGraph()
{
    // Unregister for scripts reloading events (only if using any custom nodes)
    if (_isRegisteredForScriptingEvents)
    {
#if USE_EDITOR
        Scripting::ScriptsReloading.Unbind<AnimGraph, &AnimGraph::OnScriptsReloading>(this);
        Scripting::ScriptsReloaded.Unbind<AnimGraph, &AnimGraph::OnScriptsReloaded>(this);
#endif
        Scripting::ScriptsLoaded.Unbind<AnimGraph, &AnimGraph::OnScriptsLoaded>(this);
    }
}

bool AnimGraph::onParamCreated(Parameter* p)
{
    if (p->Identifier == ANIM_GRAPH_PARAM_BASE_MODEL_ID)
    {
        // Perform validation
        if ((p->Type.Type != VariantType::Asset && p->Type.Type != VariantType::Null) || p->IsPublic)
        {
            LOG(Warning, "Invalid Base Model parameter from the Animation Graph.");
            return true;
        }

        // Peek the value
        BaseModel = (Guid)p->Value;
    }

    return Graph::onParamCreated(p);
}

AnimGraphExecutor::AnimGraphExecutor(AnimGraph& graph)
    : _graph(graph)
{
    _perGroupProcessCall[6] = (ProcessBoxHandler)&AnimGraphExecutor::ProcessGroupParameters;
    _perGroupProcessCall[7] = (ProcessBoxHandler)&AnimGraphExecutor::ProcessGroupTools;
    _perGroupProcessCall[9] = (ProcessBoxHandler)&AnimGraphExecutor::ProcessGroupAnimation;
    _perGroupProcessCall[13] = (ProcessBoxHandler)&AnimGraphExecutor::ProcessGroupCustom;
    _perGroupProcessCall[16] = (ProcessBoxHandler)&AnimGraphExecutor::ProcessGroupFunction;
}

void AnimGraphExecutor::Update(AnimGraphInstanceData& data, float dt)
{
    ASSERT(data.Parameters.Count() == _graph.Parameters.Count());

    // Initialize
    auto& skeleton = _graph.BaseModel->Skeleton;
    auto& contextPtr = Context.Get();
    if (!contextPtr)
        contextPtr = New<AnimGraphContext>();
    auto& context = *contextPtr;
    {
        ANIM_GRAPH_PROFILE_EVENT("Init");

        // Init data from base model
        _skeletonNodesCount = skeleton.Nodes.Count();
        _rootMotionMode = (RootMotionExtraction)(int32)_graph._rootNode->Values[0];

        // Prepare context data for the evaluation
        context.GraphStack.Clear();
        context.GraphStack.Push((Graph*)&_graph);
        context.NodePath.Clear();
        context.Data = &data;
        context.DeltaTime = dt;
        context.StackOverFlow = false;
        context.CurrentFrameIndex = ++data.CurrentFrame;
        context.CallStack.Clear();
        context.Functions.Clear();
        context.PoseCacheSize = 0;
        context.ValueCache.Clear();

        // Prepare instance data
        if (data.Version != _graph.Version)
        {
            data.ClearState();
            data.Version = _graph.Version;
        }
        if (data.State.Count() != _graph.BucketsCountTotal)
        {
            // Prepare memory for buckets state information
            data.State.Resize(_graph.BucketsCountTotal, false);

            // Initialize buckets
            ResetBuckets(context, &_graph);
        }
        for (auto& e : data.ActiveEvents)
            e.Hit = false;
        data.TraceEvents.Clear();

        // Init empty nodes data
        context.EmptyNodes.RootMotion = Transform::Identity;
        context.EmptyNodes.Position = 0.0f;
        context.EmptyNodes.Length = 0.0f;
        context.EmptyNodes.Nodes.Resize(_skeletonNodesCount, false);
        for (int32 i = 0; i < _skeletonNodesCount; i++)
            context.EmptyNodes.Nodes.Get()[i] = skeleton.Nodes.Get()[i].LocalTransform;
    }

    // Update the animation graph and gather skeleton nodes transformations in nodes local space
    AnimGraphImpulse* animResult = nullptr;
    {
        ANIM_GRAPH_PROFILE_EVENT("Evaluate");

        const Variant result = eatBox((Node*)_graph._rootNode, &_graph._rootNode->Boxes[0]);
        switch (result.Type.Type)
        {
        case VariantType::Pointer:
            animResult = (AnimGraphImpulse*)result.AsPointer;
            break;
        case ValueType::Null:
            break;
        default:
            //LOG(Warning, "Invalid animation update result type {0}", result.Type.ToString());
            break;
        }
        if (animResult == nullptr)
            animResult = GetEmptyNodes();
    }
    if (data.ActiveEvents.Count() != 0)
    {
        ANIM_GRAPH_PROFILE_EVENT("Events");
        for (int32 i = data.ActiveEvents.Count() - 1; i >= 0; i--)
        {
            const auto& e = data.ActiveEvents[i];
            if (!e.Hit)
            {
                // Remove active event that was not hit during this frame (eg. animation using it was not used in blending)
                data.OutgoingEvents.Add(e.End((AnimatedModel*)context.Data->Object));
                data.ActiveEvents.RemoveAt(i);
            }
        }
    }
#if !BUILD_RELEASE
    {
        // Perform sanity check on nodes pose to prevent crashes due to NaNs
        bool anyInvalid = animResult->RootMotion.IsNanOrInfinity();
        for (int32 i = 0; i < animResult->Nodes.Count(); i++)
            anyInvalid |= animResult->Nodes.Get()[i].IsNanOrInfinity();
        if (anyInvalid)
        {
            LOG(Error, "Animated Model pose contains NaNs due to animations sampling/blending bug.");
            context.Data = nullptr;
            return;
        }
    }
#endif
    SkeletonData* animResultSkeleton = &skeleton;

    // Retarget animation when using output pose from other skeleton
    AnimGraphImpulse retargetNodes;
    if (_graph.BaseModel != data.NodesSkeleton)
    {
        ANIM_GRAPH_PROFILE_EVENT("Retarget");
        auto& targetSkeleton = data.NodesSkeleton->Skeleton;
        retargetNodes = *animResult;
        retargetNodes.Nodes.Resize(targetSkeleton.Nodes.Count());
        Transform* targetNodes = retargetNodes.Nodes.Get();

        // Attempt to retarget output pose for the target skeleton
        const SkinnedModel::SkeletonMapping mapping = data.NodesSkeleton->GetSkeletonMapping(_graph.BaseModel);
        if (mapping.NodesMapping.IsValid())
        {
            // Use skeleton mapping
            const auto& sourceSkeleton = _graph.BaseModel->Skeleton;
            Transform* sourceNodes = animResult->Nodes.Get();
            RetargetSkeletonPose(sourceSkeleton, targetSkeleton, mapping, sourceNodes, targetNodes);
        }
        else
        {
            // Use T-pose as a fallback
            for (int32 i = 0; i < retargetNodes.Nodes.Count(); i++)
                targetNodes[i] = targetSkeleton.Nodes[i].LocalTransform;
        }

        animResult = &retargetNodes;
        animResultSkeleton = &targetSkeleton;
    }

    // Allow for external override of the local pose (eg. by the ragdoll)
    data.LocalPoseOverride(animResult);

    // Calculate the global poses for the skeleton nodes
    {
        ANIM_GRAPH_PROFILE_EVENT("Global Pose");

        ASSERT(animResultSkeleton->Nodes.Count() == animResult->Nodes.Count());
        data.NodesPose.Resize(animResultSkeleton->Nodes.Count(), false);
        Transform* nodesTransformations = animResult->Nodes.Get();

        // Note: this assumes that nodes are sorted (parents first)
        for (int32 nodeIndex = 0; nodeIndex < animResultSkeleton->Nodes.Count(); nodeIndex++)
        {
            const int32 parentIndex = animResultSkeleton->Nodes[nodeIndex].ParentIndex;
            if (parentIndex != -1)
            {
                nodesTransformations[parentIndex].LocalToWorld(nodesTransformations[nodeIndex], nodesTransformations[nodeIndex]);
            }
            nodesTransformations[nodeIndex].GetWorld(data.NodesPose[nodeIndex]);
        }

        // Process the root node transformation and the motion
        data.RootTransform = nodesTransformations[0];
        data.RootMotion = animResult->RootMotion;
    }

    // Invoke any async anim events
    context.Data->InvokeAnimEvents();

    // Cleanup
    context.Data = nullptr;
}

void AnimGraphExecutor::GetInputValue(Box* box, Value& result)
{
    result = eatBox(box->GetParent<Node>(), box->FirstConnection());
}

AnimGraphImpulse* AnimGraphExecutor::GetEmptyNodes()
{
    return &Context.Get()->EmptyNodes;
}

void AnimGraphExecutor::InitNodes(AnimGraphImpulse* nodes) const
{
    const auto& emptyNodes = Context.Get()->EmptyNodes;
    Platform::MemoryCopy(nodes->Nodes.Get(), emptyNodes.Nodes.Get(), sizeof(Transform) * _skeletonNodesCount);
    nodes->RootMotion = emptyNodes.RootMotion;
    nodes->Position = emptyNodes.Position;
    nodes->Length = emptyNodes.Length;
}

void AnimGraphExecutor::ResetBuckets(AnimGraphContext& context, AnimGraphBase* graph)
{
    if (graph == nullptr)
        return;

    auto& state = context.Data->State;
    for (int32 i = 0; i < graph->BucketsCountTotal; i++)
    {
        const int32 bucketIndex = graph->BucketsStart + i;
        _graph._bucketInitializerList[bucketIndex](state[bucketIndex]);
    }
}

VisjectExecutor::Value AnimGraphExecutor::eatBox(Node* caller, Box* box)
{
    auto& context = *Context.Get();

    // Check if graph is looped or is too deep
    if (context.StackOverFlow)
        return Value::Zero;
    if (context.CallStack.Count() >= ANIM_GRAPH_MAX_CALL_STACK)
    {
        OnError(caller, box, TEXT("Graph is looped or too deep!"));
        context.StackOverFlow = true;
        return Value::Zero;
    }
#if !BUILD_RELEASE
    if (box == nullptr)
    {
        OnError(caller, box, TEXT("Null graph box!"));
        return Value::Zero;
    }
#endif

    // Add to the calling stack
    context.CallStack.Add(caller);

#if USE_EDITOR
    Animations::DebugFlowInfo flowInfo;
    flowInfo.Asset = _graph._owner;
    flowInfo.Instance = context.Data->Object;
    flowInfo.NodeId = box->GetParent<Node>()->ID;
    flowInfo.BoxId = box->ID;
    const auto* nodePath = context.NodePath.Get();
    for (int32 i = 0; i < context.NodePath.Count(); i++)
        flowInfo.NodePath[i] = nodePath[i];
    Animations::DebugFlow(flowInfo);
#endif

    // Call per group custom processing event
    Value value;
    const auto parentNode = box->GetParent<Node>();
    const ProcessBoxHandler func = _perGroupProcessCall[parentNode->GroupID];
    (this->*func)(box, parentNode, value);

    // Remove from the calling stack
    context.CallStack.RemoveLast();

    return value;
}

VisjectExecutor::Graph* AnimGraphExecutor::GetCurrentGraph() const
{
    auto& context = *Context.Get();
    return context.GraphStack.Peek();
}
