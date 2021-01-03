// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Engine/Time.h"
#include "Engine/Scripting/Scripting.h"

RootMotionData RootMotionData::Identity = { Vector3(0.0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f) };

RootMotionData& RootMotionData::operator+=(const RootMotionData& b)
{
    Translation += b.Translation;
    Rotation *= b.Rotation;
    return *this;
}

RootMotionData& RootMotionData::operator+=(const Transform& b)
{
    Translation += b.Translation;
    Rotation *= b.Orientation;
    return *this;
}

RootMotionData& RootMotionData::operator-=(const Transform& b)
{
    Translation -= b.Translation;
    Quaternion invRotation = Rotation;
    invRotation.Invert();
    Quaternion::Multiply(invRotation, b.Orientation, Rotation);
    return *this;
}

RootMotionData RootMotionData::operator+(const RootMotionData& b) const
{
    RootMotionData result;

    result.Translation = Translation + b.Translation;
    result.Rotation = Rotation * b.Rotation;

    return result;
}

RootMotionData RootMotionData::operator-(const RootMotionData& b) const
{
    RootMotionData result;

    result.Rotation = Rotation;
    result.Rotation.Invert();
    Vector3::Transform(b.Translation - Translation, result.Rotation, result.Translation);
    Quaternion::Multiply(result.Rotation, b.Rotation, result.Rotation);

    return result;
}

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

AnimGraphImpulse* AnimGraphNode::GetNodes(AnimGraphExecutor* executor)
{
    // Ensure to have memory
    const int32 count = executor->_skeletonNodesCount;
    if (Nodes.Nodes.Count() != count)
    {
        Nodes.Nodes.Resize(count, false);
    }

    return &Nodes;
}

bool AnimGraph::Load(ReadStream* stream, bool loadMeta)
{
    Version++;
    _bucketsCounter = 0;
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

const Matrix* AnimGraphExecutor::Update(AnimGraphInstanceData& data, float dt)
{
    ASSERT(data.Parameters.Count() == _graph.Parameters.Count());

    // Initialize
    auto& skeleton = _graph.BaseModel->Skeleton;
    {
        ANIM_GRAPH_PROFILE_EVENT("Init");

        // Prepare graph data for the evaluation
        _skeletonBonesCount = skeleton.Bones.Count();
        _skeletonNodesCount = skeleton.Nodes.Count();
        _graphStack.Clear();
        _graphStack.Push((Graph*)&_graph);
        _data = &data;
        _deltaTime = dt;
        _rootMotionMode = (RootMotionMode)(int32)_graph._rootNode->Values[0];
        _currentFrameIndex = ++data.CurrentFrame;
        _callStack.Clear();
        _functions.Clear();
        _graph.ClearCache();

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
            ResetBuckets(&_graph);
        }

        // Init empty nodes data
        _emptyNodes.RootMotion = RootMotionData::Identity;
        _emptyNodes.Position = 0.0f;
        _emptyNodes.Length = 0.0f;
        _emptyNodes.Nodes.Resize(_skeletonNodesCount, false);
        for (int32 i = 0; i < _skeletonNodesCount; i++)
        {
            auto& node = skeleton.Nodes[i];
            _emptyNodes.Nodes[i] = node.LocalTransform;
        }
    }

    // Update the animation graph and gather skeleton nodes transformations in nodes local space
    AnimGraphImpulse* animResult;
    {
        ANIM_GRAPH_PROFILE_EVENT("Evaluate");

        auto result = eatBox((Node*)_graph._rootNode, &_graph._rootNode->Boxes[0]);
        if (result.Type.Type != VariantType::Pointer)
        {
            result = VariantType::Null;
            LOG(Warning, "Invalid animation update result");
        }
        animResult = (AnimGraphImpulse*)result.AsPointer;
        if (animResult == nullptr)
            animResult = GetEmptyNodes();
    }
    Transform* nodesTransformations = animResult->Nodes.Get();

    // Calculate the global poses for the skeleton nodes
    {
        ANIM_GRAPH_PROFILE_EVENT("Global Pose");

        _data->NodesPose.Resize(_skeletonNodesCount, false);

        // Note: this assumes that nodes are sorted (parents first)
        for (int32 nodeIndex = 0; nodeIndex < _skeletonNodesCount; nodeIndex++)
        {
            const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
            if (parentIndex != -1)
            {
                nodesTransformations[nodeIndex] = nodesTransformations[parentIndex].LocalToWorld(nodesTransformations[nodeIndex]);
            }
            nodesTransformations[nodeIndex].GetWorld(_data->NodesPose[nodeIndex]);
        }
    }

    // Process the root node transformation and the motion
    {
        _data->RootTransform = nodesTransformations[0];
        _data->RootMotion = animResult->RootMotion;
    }

    // Calculate the final bones transformations
    {
        ANIM_GRAPH_PROFILE_EVENT("Final Pose");

        _bonesTransformations.Resize(_skeletonBonesCount, false);

        for (int32 boneIndex = 0; boneIndex < _skeletonBonesCount; boneIndex++)
        {
            auto& bone = skeleton.Bones[boneIndex];
            _bonesTransformations[boneIndex] = bone.OffsetMatrix * _data->NodesPose[bone.NodeIndex];
        }
    }

    // Cleanup
    _data = nullptr;

    return _bonesTransformations.Get();
}

void AnimGraphExecutor::ResetBuckets(AnimGraphBase* graph)
{
    if (graph == nullptr)
        return;

    ASSERT(_data);
    for (int32 i = 0; i < graph->BucketsCountTotal; i++)
    {
        const int32 bucketIndex = graph->BucketsStart + i;
        _graph._bucketInitializerList[bucketIndex](_data->State[bucketIndex]);
    }
}

VisjectExecutor::Value AnimGraphExecutor::eatBox(Node* caller, Box* box)
{
    // Check if graph is looped or is too deep
    if (_callStack.Count() >= ANIM_GRAPH_MAX_CALL_STACK)
    {
        OnError(caller, box, TEXT("Graph is looped or too deep!"));
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
    _callStack.Add(caller);

#if USE_EDITOR
    DebugFlow(_graph._owner, _data->Object, box->GetParent<Node>()->ID, box->ID);
#endif

    // Call per group custom processing event
    Value value;
    const auto parentNode = box->GetParent<Node>();
    const ProcessBoxHandler func = _perGroupProcessCall[parentNode->GroupID];
    (this->*func)(box, parentNode, value);

    // Remove from the calling stack
    _callStack.RemoveLast();

    return value;
}

VisjectExecutor::Graph* AnimGraphExecutor::GetCurrentGraph() const
{
    return _graphStack.Peek();
}
