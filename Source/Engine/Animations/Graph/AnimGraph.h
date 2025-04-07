// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Visject/VisjectGraph.h"
#include "Engine/Content/Assets/Animation.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Animations/AlphaBlend.h"
#include "Engine/Core/Math/Matrix.h"
#include "../Config.h"

#define ANIM_GRAPH_PARAM_BASE_MODEL_ID Guid(1000, 0, 0, 0)

#define ANIM_GRAPH_IS_VALID_PTR(value) (value.Type.Type == VariantType::Pointer && value.AsPointer != nullptr)
#define ANIM_GRAPH_MULTI_BLEND_INDEX byte
#define ANIM_GRAPH_MULTI_BLEND_INVALID 0xff
#define ANIM_GRAPH_MAX_CALL_STACK 100
#define ANIM_GRAPH_MAX_EVENTS 64

class AnimGraph;
class AnimSubGraph;
class AnimGraphBase;
class AnimGraphNode;
class AnimGraphExecutor;
class AnimatedModel;
class AnimEvent;
class AnimContinuousEvent;
class SkinnedModel;
class SkeletonData;

/// <summary>
/// The animation graph 'impulse' connections data container (the actual transfer is done via pointer as it gives better performance). 
/// Container for skeleton nodes transformation hierarchy and any other required data. 
/// Unified layout for both local and model transformation spaces.
/// </summary>
struct FLAXENGINE_API AnimGraphImpulse
{
    /// <summary>
    /// The skeleton nodes transformation hierarchy nodes. Size always matches the Anim Graph skeleton description.
    /// </summary>
    Array<Transform> Nodes;

    /// <summary>
    /// The root motion extracted from the animation to apply on animated object.
    /// </summary>
    Transform RootMotion = Transform::Identity;

    /// <summary>
    /// The animation time position (in seconds).
    /// </summary>
    float Position;

    /// <summary>
    /// The animation length (in seconds).
    /// </summary>
    float Length;

    FORCE_INLINE Transform GetNodeLocalTransformation(SkeletonData& skeleton, int32 nodeIndex) const
    {
        return Nodes[nodeIndex];
    }

    FORCE_INLINE void SetNodeLocalTransformation(SkeletonData& skeleton, int32 nodeIndex, const Transform& value)
    {
        Nodes[nodeIndex] = value;
    }

    Transform GetNodeModelTransformation(SkeletonData& skeleton, int32 nodeIndex) const;
    void SetNodeModelTransformation(SkeletonData& skeleton, int32 nodeIndex, const Transform& value);
};

/// <summary>
/// The bone transformation modes.
/// </summary>
enum class BoneTransformMode
{
    /// <summary>
    /// No transformation.
    /// </summary>
    None = 0,

    /// <summary>
    /// Applies the transformation.
    /// </summary>
    Add = 1,

    /// <summary>
    /// Replaces the transformation.
    /// </summary>
    Replace = 2,
};

/// <summary>
/// The animated model root motion extraction modes.
/// </summary>
enum class RootMotionExtraction
{
    /// <summary>
    /// Don't extract nor apply the root motion.
    /// </summary>
    NoExtraction = 0,

    /// <summary>
    /// Ignore root motion (remove from root node transform).
    /// </summary>
    Ignore = 1,

    /// <summary>
    /// Enable root motion (remove from root node transform and apply to the target).
    /// </summary>
    Enable = 2,
};

/// <summary>
/// Data container for the animation graph state machine transition between two states.
/// </summary>
class AnimGraphStateTransition
{
public:
    /// <summary>
    /// The transition flag types.
    /// </summary>
    enum class FlagTypes
    {
        None = 0,
        Enabled = 1,
        Solo = 2,
        UseDefaultRule = 4,
        InterruptionRuleRechecking = 8,
        InterruptionInstant = 16,
        InterruptionSourceState = 32,
        InterruptionDestinationState = 64,
    };

public:
    /// <summary>
    /// The destination state node.
    /// </summary>
    AnimGraphNode* Destination;

    /// <summary>
    /// The transition rule graph (optional).
    /// </summary>
    AnimSubGraph* RuleGraph;

    /// <summary>
    /// The flags.
    /// </summary>
    FlagTypes Flags;

    /// <summary>
    /// The blend mode.
    /// </summary>
    AlphaBlendMode BlendMode;

    /// <summary>
    /// The blend duration (in seconds).
    /// </summary>
    float BlendDuration;
};

DECLARE_ENUM_OPERATORS(AnimGraphStateTransition::FlagTypes);

/// <summary>
/// Animation graph parameter.
/// </summary>
/// <seealso cref="GraphParameter" />
API_CLASS() class AnimGraphParameter : public VisjectGraphParameter
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(AnimGraphParameter, VisjectGraphParameter);
};

/// <summary>
/// The animation graph slot-based animation.
/// </summary>
struct FLAXENGINE_API AnimGraphSlot
{
    String Name;
    AssetReference<Animation> Animation;
    float Speed = 1.0f;
    float BlendInTime = 0.0f;
    float BlendOutTime = 0.0f;
    int32 LoopCount = 0;
    bool Pause = false;
    bool Reset = false;
};

/// <summary>
/// The animation graph state container for a single node playback trace (eg. animation sample info or state transition). Can be used by Anim Graph debugging or custom scripting.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API AnimGraphTraceEvent
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AnimGraphTraceEvent);

    // Contextual asset used. For example, sampled animation.
    API_FIELD() Asset* Asset = nullptr;
    // Generic value contextual to playback type (eg. animation sample position).
    API_FIELD() float Value = 0;
    // Identifier of the node in the graph.
    API_FIELD() uint32 NodeId = 0;
    // Ids of graph nodes (call of hierarchy).
    API_FIELD(Internal, NoArray) uint32 NodePath[8] = {};
};

/// <summary>
/// The animation graph instance data storage. Required to update the animation graph.
/// </summary>
class FLAXENGINE_API AnimGraphInstanceData
{
    friend AnimGraphExecutor;
public:
    // ---- Quick documentation ----
    // AnimGraphInstanceData holds a single animation graph instance playback data.
    // It has parameters (the same layout as graph) that can be modified per instance (eg. by game scripts).
    // It has also Buckets!
    // Each bucket contains a state for a single graph node that requires state information.
    // For example, animation playback node needs to store the current playback position.
    // State machine nodes need some state index and transition info and so on.
    // So when loading the graph we need to capture the buckets metadata (which nodes use them, how init bucket, etc.).
    // Later we just sync buckets with the instance data.
    // The key of this solution: reduce allocations and redundancy between graph asset and the users.

    struct AnimationBucket
    {
        float TimePosition;
        uint64 LastUpdateFrame;
    };

    struct MultiBlendBucket
    {
        constexpr static int32 MaxCount = 3; // Up to 3 anims to be used at once in 2D blend space (triangle)
        float TimePositions[MaxCount];
        ANIM_GRAPH_MULTI_BLEND_INDEX Animations[MaxCount];
        byte Count;
        uint64 LastUpdateFrame;
    };

    struct BlendPoseBucket
    {
        float TransitionPosition;
        int32 PreviousBlendPoseIndex;
    };

    struct StateMachineBucket
    {
        uint64 LastUpdateFrame;
        AnimGraphNode* CurrentState;
        AnimGraphStateTransition* ActiveTransition;
        AnimGraphStateTransition* BaseTransition;
        AnimGraphNode* BaseTransitionState;
        float TransitionPosition;
        float BaseTransitionPosition;
    };

    struct SlotBucket
    {
        int32 Index;
        float TimePosition;
        float BlendInPosition;
        float BlendOutPosition;
        int32 LoopsDone;
        int32 LoopsLeft;
    };

    struct InstanceDataBucket
    {
        bool Init;
        float Data[4];
    };

    /// <summary>
    /// The single data storage bucket for the instanced animation graph node. Used to store the node state (playback position, state, transition data).
    /// </summary>
    struct Bucket
    {
        union
        {
            AnimationBucket Animation;
            MultiBlendBucket MultiBlend;
            BlendPoseBucket BlendPose;
            StateMachineBucket StateMachine;
            SlotBucket Slot;
            InstanceDataBucket InstanceData;
        };
    };

public:
    /// <summary>
    /// The instance data version number. Used to sync the Anim Graph data with the instance state. Handles Anim Graph reloads to ensure data is valid.
    /// </summary>
    uint32 Version = 0;

    /// <summary>
    /// The last game time when animation was updated.
    /// </summary>
    float LastUpdateTime = -1;

    /// <summary>
    /// The current animation update frame index. Increment on every update.
    /// </summary>
    int64 CurrentFrame = 0;

    /// <summary>
    /// The root node transformation. Cached after the animation update.
    /// </summary>
    Transform RootTransform = Transform::Identity;

    /// <summary>
    /// The current root motion delta to apply on a target object.
    /// </summary>
    Transform RootMotion = Transform::Identity;

    /// <summary>
    /// The animation graph parameters collection (instanced, override the default values).
    /// </summary>
    Array<AnimGraphParameter> Parameters;

    /// <summary>
    /// The animation state data.
    /// </summary>
    Array<Bucket> State;

    /// <summary>
    /// The per-node final transformations in actor local-space.
    /// </summary>
    Array<Matrix> NodesPose;

    /// <summary>
    /// The object that represents the instance data source (used by Custom Nodes and debug flows).
    /// </summary>
    ScriptingObject* Object;

    /// <summary>
    /// The output nodes pose skeleton asset to use. Allows to remap evaluated animation pose for Base Model of the Anim Graph to the target Animated Model that plays it. Nodes Pose will match its skeleton. Use null if disable retargetting.
    /// </summary>
    SkinnedModel* NodesSkeleton = nullptr;

    /// <summary>
    /// The custom event called after local pose evaluation and retargetting.
    /// </summary>
    Delegate<AnimGraphImpulse*> LocalPoseOverride;

    /// <summary>
    /// The slots animations.
    /// </summary>
    Array<AnimGraphSlot, InlinedAllocation<4>> Slots;

public:
    /// <summary>
    /// Clears this container data.
    /// </summary>
    void Clear();

    /// <summary>
    /// Clears this container state data.
    /// </summary>
    void ClearState();

    /// <summary>
    /// Invalidates the update timer.
    /// </summary>
    void Invalidate();

    /// <summary>
    /// Invokes any outgoing AnimEvent and AnimContinuousEvent collected during the last animation update. When called from non-main thread only Async events will be invoked.
    /// </summary>
    void InvokeAnimEvents();

public:
    // Anim Graph logic tracing feature that allows to collect insights of animations sampling and skeleton poses operations.
    bool EnableTracing = false;
    // Trace events collected when using EnableTracing option.
    Array<AnimGraphTraceEvent> TraceEvents;

private:
    struct OutgoingEvent
    {
        enum Types
        {
            OnEvent,
            OnBegin,
            OnEnd,
        };

        AnimEvent* Instance;
        AnimatedModel* Actor;
        Animation* Anim;
        float Time, DeltaTime;
        Types Type;
    };

    struct ActiveEvent
    {
        AnimContinuousEvent* Instance;
        Animation* Anim;
        AnimGraphNode* Node;
        bool Hit;

        OutgoingEvent End(AnimatedModel* actor) const;
    };

    Array<ActiveEvent, InlinedAllocation<8>> ActiveEvents;
    Array<OutgoingEvent, InlinedAllocation<8>> OutgoingEvents;
};

/// <summary>
/// The anim graph transition data cached for nodes that read it to calculate if can enter transition.
/// </summary>
struct AnimGraphTransitionData
{
    float Position;
    float Length;
};

typedef VisjectGraphBox AnimGraphBox;

class AnimGraphNode : public VisjectGraphNode<AnimGraphBox>
{
public:
    struct MultiBlendData
    {
        // Amount of blend points.
        ANIM_GRAPH_MULTI_BLEND_INDEX Count;
        // The computed length of the mixes animations. Shared for all blend points to provide more stabilization during looped playback.
        float Length;
    };

    struct MultiBlend1DData : MultiBlendData
    {
        // The indices of the animations to blend. Sorted from the lowest X to the highest X. Contains only valid used animations. Unused items are using index ANIM_GRAPH_MULTI_BLEND_INVALID which is invalid.
        ANIM_GRAPH_MULTI_BLEND_INDEX* IndicesSorted;
    };

    struct MultiBlend2DData : MultiBlendData
    {
        // Amount of triangles.
        int32 TrianglesCount;
        // Cached triangles vertices (3 bytes per triangle). Contains list of indices for triangles to use for blending.
        ANIM_GRAPH_MULTI_BLEND_INDEX* Triangles;
    };

    struct StateMachineData
    {
        /// <summary>
        /// The graph of the state machine. Contains all states and transitions between them. Its root node is the first state of the state machine pointed by the Entry node.
        /// </summary>
        AnimSubGraph* Graph;
    };

    struct StateBaseData
    {
        // The invalid transition valid used in Transitions to indicate invalid transition linkage.
        const static uint16 InvalidTransitionIndex = MAX_uint16;

        // The outgoing transitions from this state to the other states. Each array item contains index of the transition data from the state node graph transitions cache. Value InvalidTransitionIndex is used for last transition to indicate the transitions amount.
        uint16* Transitions;
    };

    struct StateData : StateBaseData
    {
        /// <summary>
        /// The graph of the state. Contains the state animation evaluation graph. Its root node is the state output node with an input box for the state blend pose sampling.
        /// </summary>
        AnimSubGraph* Graph;
    };

    struct AnyStateData : StateBaseData
    {
    };

    struct CustomData
    {
        /// <summary>
        /// The cached method to invoke on custom node evaluation.
        /// </summary>
        class MMethod* Evaluate;

        /// <summary>
        /// The GC handle to the managed instance of the node object.
        /// </summary>
        MGCHandle Handle;
    };

    struct CurveData
    {
        /// <summary>
        /// The curve index.
        /// </summary>
        int32 CurveIndex;
    };

    struct AnimationGraphFunctionData
    {
        /// <summary>
        /// The loaded sub-graph.
        /// </summary>
        AnimSubGraph* Graph;
    };

    struct TransformNodeData
    {
        int32 NodeIndex;
    };

    struct CopyNodeData
    {
        int32 SrcNodeIndex;
        int32 DstNodeIndex;
    };

    /// <summary>
    /// Custom cached data per node type. Compact to use as small amount of memory as possible.
    /// </summary>
    struct AdditionalData
    {
        union
        {
            MultiBlend1DData MultiBlend1D;
            MultiBlend2DData MultiBlend2D;
            StateMachineData StateMachine;
            StateData State;
            AnyStateData AnyState;
            CustomData Custom;
            CurveData Curve;
            AnimationGraphFunctionData AnimationGraphFunction;
            TransformNodeData TransformNode;
            CopyNodeData CopyNode;
        };
    };

public:
    /// <summary>
    /// The animation graph.
    /// </summary>
    AnimGraph* Graph = nullptr;

    /// <summary>
    /// The index of the animation state bucket used by this node.
    /// </summary>
    int32 BucketIndex = -1;

    /// <summary>
    /// The custom data (depends on node type). Used to cache data for faster usage at runtime.
    /// </summary>
    AdditionalData Data;

public:
    AnimGraphNode()
    {
    }

    ~AnimGraphNode();

public:
    /// <summary>
    /// Gets the per-node node transformations cache (cached).
    /// </summary>
    /// <param name="executor">The Graph execution context.</param>
    /// <returns>Nodes data.</returns>
    AnimGraphImpulse* GetNodes(AnimGraphExecutor* executor);
};

/// <summary>
/// The base class for Anim Graphs that supports nesting sub graphs.
/// </summary>
/// <seealso cref="VisjectGraph{AnimGraphNode, AnimGraphBox, AnimGraphParameter}" />
class AnimGraphBase : public VisjectGraph<AnimGraphNode, AnimGraphBox, AnimGraphParameter>
{
protected:
    AnimGraph* _graph;
    Node* _rootNode = nullptr;

    AnimGraphBase(AnimGraph* graph)
        : _graph(graph)
    {
    }

public:
    /// <summary>
    /// The sub graphs nested in this graph.
    /// </summary>
    Array<AnimSubGraph*, InlinedAllocation<32>> SubGraphs;

    /// <summary>
    /// The state transitions cached per-graph (that is a state machine).
    /// </summary>
    Array<AnimGraphStateTransition> StateTransitions;

    /// <summary>
    /// The zero-based index of the bucket used by this graph. Valid only if BucketsCountSelf is non zero.
    /// </summary>
    int32 BucketsStart;

    /// <summary>
    /// The amount of state buckets used by the this graph.
    /// </summary>
    int32 BucketsCountSelf;

    /// <summary>
    /// The amount of state buckets used by this graph including all sub-graphs.
    /// </summary>
    int32 BucketsCountTotal;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="AnimGraphBase"/> class.
    /// </summary>
    ~AnimGraphBase()
    {
        SubGraphs.ClearDelete();
    }

public:
    /// <summary>
    /// Gets the root node of the graph (cache don load).
    /// </summary>
    FORCE_INLINE Node* GetRootNode() const
    {
        return _rootNode;
    }

    /// <summary>
    /// Loads the sub-graph.
    /// </summary>
    /// <param name="data">The surface data.</param>
    /// <param name="dataLength">The length of the data (in bytes).</param>
    /// <param name="name">The name of the sub-graph (optional). Used in error message logs.</param>
    /// <returns>The loaded sub graph or null if there is no surface to load or if loading failed.</returns>
    AnimSubGraph* LoadSubGraph(const void* data, int32 dataLength, const Char* name);

public:
    // [Graph]
    bool Load(ReadStream* stream, bool loadMeta) override;
    void Clear() override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& output) const override;
#endif

protected:
    // [Graph]
    bool onNodeLoaded(Node* n) override;

private:
    void LoadStateTransitions(AnimGraphNode::StateBaseData& data, Value& transitionsData);
};

/// <summary>
/// The sub-graph for the main Animation Graph. Used for Anim graphs nesting.
/// </summary>
/// <seealso cref="AnimGraphBase" />
class AnimSubGraph : public AnimGraphBase
{
    friend AnimGraph;
    friend AnimGraphBase;
    friend AnimGraphBox;
    friend AnimGraphNode;
    friend AnimGraphParameter;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AnimSubGraph" /> class.
    /// </summary>
    /// <param name="graph">The graph.</param>
    AnimSubGraph(AnimGraph* graph)
        : AnimGraphBase(graph)
    {
    }
};

/// <summary>
/// The Animation Graph is used to evaluate a final pose for the animated model for the current frame.
/// </summary>
class AnimGraph : public AnimGraphBase
{
    friend AnimGraphBase;
    friend AnimGraphBox;
    friend AnimGraphNode;
    friend AnimGraphParameter;
    friend AnimGraphExecutor;

private:
    typedef void (*InitBucketHandler)(AnimGraphInstanceData::Bucket&);

    bool _isFunction, _isRegisteredForScriptingEvents;
    int32 _bucketsCounter;
    Array<InitBucketHandler> _bucketInitializerList;
    Array<Node*> _customNodes;
    Asset* _owner;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AnimGraph"/> class.
    /// </summary>
    /// <param name="owner">The graph owning asset.</param>
    /// <param name="isFunction">True if the graph is Animation Graph Function, otherwise false.</param>
    AnimGraph(Asset* owner, bool isFunction = false)
        : AnimGraphBase(this)
        , _isFunction(isFunction)
        , _isRegisteredForScriptingEvents(false)
        , _owner(owner)
    {
    }

    ~AnimGraph();

public:
    /// <summary>
    /// The Anim Graph data version number. Used to sync the Anim Graph data with the instances state. Handles Anim Graph reloads to ensure data is valid.
    /// </summary>
    uint32 Version = 0;

    /// <summary>
    /// The base model asset used for the animation preview and the skeleton layout source.
    /// </summary>
    /// <remarks>
    /// Use for read-only as it's serialized from the one of the Graph parameters (see ANIM_GRAPH_PARAM_BASE_MODEL_ID);
    /// </remarks>
    AssetReference<SkinnedModel> BaseModel;

public:
    /// <summary>
    /// Determines whether this graph is ready for the animation evaluation.
    /// </summary>
    bool IsReady() const;

private:
    void ClearCustomNode(Node* node);
    bool InitCustomNode(Node* node);

#if USE_EDITOR
    void OnScriptsReloading();
    void OnScriptsReloaded();
#endif
    void OnScriptsLoaded();

public:
    // [Graph]
    bool Load(ReadStream* stream, bool loadMeta) override;
    bool onParamCreated(Parameter* p) override;
};

/// <summary>
/// The Animation Graph evaluation context.
/// </summary>
struct AnimGraphContext
{
    float DeltaTime;
    uint64 CurrentFrameIndex;
    AnimGraphInstanceData* Data;
    AnimGraphImpulse EmptyNodes;
    AnimGraphTransitionData TransitionData;
    bool StackOverFlow;
    Array<VisjectExecutor::Node*, FixedAllocation<ANIM_GRAPH_MAX_CALL_STACK>> CallStack;
    Array<VisjectExecutor::Graph*, FixedAllocation<32>> GraphStack;
    Array<uint32, FixedAllocation<ANIM_GRAPH_MAX_CALL_STACK>> NodePath;
    Dictionary<VisjectExecutor::Node*, VisjectExecutor::Graph*> Functions;
    ChunkedArray<AnimGraphImpulse, 256> PoseCache;
    int32 PoseCacheSize;
    Dictionary<VisjectExecutor::Box*, Variant> ValueCache;

    AnimGraphTraceEvent& AddTraceEvent(const AnimGraphNode* node);
};

/// <summary>
/// The Animation Graph executor runtime for animation pose evaluation.
/// </summary>
class AnimGraphExecutor : public VisjectExecutor
{
    friend AnimGraphNode;
private:
    AnimGraph& _graph;
    RootMotionExtraction _rootMotionMode = RootMotionExtraction::NoExtraction;
    int32 _skeletonNodesCount = 0;

    // Per-thread context to allow async execution
    static ThreadLocal<AnimGraphContext*> Context;

public:
    /// <summary>
    /// Initializes the managed runtime calls.
    /// </summary>
    static void initRuntime();

    /// <summary>
    /// Initializes a new instance of the <see cref="AnimGraphExecutor"/> class.
    /// </summary>
    /// <param name="graph">The graph to execute.</param>
    explicit AnimGraphExecutor(AnimGraph& graph);

public:
    /// <summary>
    /// Updates the graph animation.
    /// </summary>
    /// <param name="data">The instance data.</param>
    /// <param name="dt">The delta time (in seconds).</param>
    void Update(AnimGraphInstanceData& data, float dt);

    void GetInputValue(Box* box, Value& result);

    /// <summary>
    /// Gets the skeleton nodes transformations structure containing identity matrices.
    /// </summary>
    AnimGraphImpulse* GetEmptyNodes();

    // Initialize impulse with cached node transformations
    void InitNodes(AnimGraphImpulse* nodes) const;

    FORCE_INLINE void CopyNodes(AnimGraphImpulse* dstNodes, AnimGraphImpulse* srcNodes) const
    {
        // Copy the node transformations
        Platform::MemoryCopy(dstNodes->Nodes.Get(), srcNodes->Nodes.Get(), sizeof(Transform) * _skeletonNodesCount);

        // Copy the animation playback state
        dstNodes->Position = srcNodes->Position;
        dstNodes->Length = srcNodes->Length;
    }

    FORCE_INLINE void CopyNodes(AnimGraphImpulse* dstNodes, const Value& value) const
    {
        ASSERT(ANIM_GRAPH_IS_VALID_PTR(value));
        CopyNodes(dstNodes, static_cast<AnimGraphImpulse*>(value.AsPointer));
    }

    /// <summary>
    /// Resets all the state bucket used by the given graph including sub-graphs (total). Can be used to reset the animation state of the nested graph (including children).
    /// </summary>
    void ResetBuckets(AnimGraphContext& context, AnimGraphBase* graph);

private:
    Value eatBox(Node* caller, Box* box) override;
    Graph* GetCurrentGraph() const override;

    void ProcessGroupParameters(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* nodeBase, Value& value);
    void ProcessGroupAnimation(Box* boxBase, Node* nodeBase, Value& value);
    void ProcessGroupCustom(Box* boxBase, Node* nodeBase, Value& value);
    void ProcessGroupFunction(Box* boxBase, Node* node, Value& value);

    enum class ProcessAnimationMode
    {
        Override,
        Add,
        BlendAdditive,
    };

    int32 GetRootNodeIndex(Animation* anim);
    void ProcessAnimEvents(AnimGraphNode* node, bool loop, float length, float animPos, float animPrevPos, Animation* anim, float speed);
    void ProcessAnimation(AnimGraphImpulse* nodes, AnimGraphNode* node, bool loop, float length, float pos, float prevPos, Animation* anim, float speed, float weight = 1.0f, ProcessAnimationMode mode = ProcessAnimationMode::Override, BitArray<InlinedAllocation<8>>* usedNodes = nullptr);
    Variant SampleAnimation(AnimGraphNode* node, bool loop, float length, float startTimePos, float prevTimePos, float& newTimePos, Animation* anim, float speed);
    Variant SampleAnimation(AnimGraphNode* node, bool loop, float startTimePos, struct AnimSampleData& sample);
    Variant SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float startTimePos, AnimSampleData& a, AnimSampleData& b, float alpha);
    Variant SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float startTimePos, AnimSampleData& a, AnimSampleData& b, AnimSampleData& c, float alphaA, float alphaB, float alphaC);
    Variant Blend(AnimGraphNode* node, const Value& poseA, const Value& poseB, float alpha, AlphaBlendMode alphaMode);
    Variant SampleState(AnimGraphContext& context, const AnimGraphNode* state);
    void InitStateTransition(AnimGraphContext& context, AnimGraphInstanceData::StateMachineBucket& stateMachineBucket, AnimGraphStateTransition* transition = nullptr);
    AnimGraphStateTransition* UpdateStateTransitions(AnimGraphContext& context, const AnimGraphNode::StateMachineData& stateMachineData, AnimGraphNode* state, AnimGraphNode* ignoreState = nullptr);
    AnimGraphStateTransition* UpdateStateTransitions(AnimGraphContext& context, const AnimGraphNode::StateMachineData& stateMachineData, const AnimGraphNode::StateBaseData& stateData, AnimGraphNode* state, AnimGraphNode* ignoreState = nullptr);
    void UpdateStateTransitions(AnimGraphContext& context, const AnimGraphNode::StateMachineData& stateMachineData, AnimGraphInstanceData::StateMachineBucket& stateMachineBucket, const AnimGraphNode::StateBaseData& stateData);
};
