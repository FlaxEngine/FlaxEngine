// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BehaviorKnowledge.h"
#include "BehaviorTreeNode.h"
#include "BehaviorKnowledgeSelector.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Level/Tags.h"
#include "Engine/Navigation/NavMeshRuntime.h"

class Actor;

/// <summary>
/// Base class for compound Behavior Tree nodes that composite child nodes.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BehaviorTreeCompoundNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeCompoundNode, BehaviorTreeNode);

    /// <summary>
    /// List with all child nodes.
    /// </summary>
    API_FIELD(Readonly) Array<BehaviorTreeNode*, InlinedAllocation<8>> Children;

public:
    // [BehaviorTreeNode]
    void Init(BehaviorTree* tree) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;

protected:
    // [BehaviorTreeNode]
    void BecomeIrrelevant(const BehaviorUpdateContext& context) override;
};

/// <summary>
/// Sequence node updates all its children (from left to right) as long as they return success. If any child fails, the sequence is failed.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorTreeSequenceNode : public BehaviorTreeCompoundNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSequenceNode, BehaviorTreeCompoundNode);

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;

private:
    struct State
    {
        int32 CurrentChildIndex = 0;
    };
};

/// <summary>
/// Selector node updates all its children (from left to right) until one of them succeeds. If all children fail, the selector fails.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorTreeSelectorNode : public BehaviorTreeCompoundNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSelectorNode, BehaviorTreeCompoundNode);

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;

private:
    struct State
    {
        int32 CurrentChildIndex = 0;
    };
};

/// <summary>
/// Root node of the behavior tree. Contains logic properties and definitions for the runtime.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeRootNode : public BehaviorTreeSequenceNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeRootNode, BehaviorTreeSequenceNode);
    API_AUTO_SERIALIZATION();

    // Full typename of the blackboard data type (structure or class). Spawned for each instance of the behavior.
    API_FIELD(Attributes="EditorOrder(0), TypeReference(\"\", \"IsValidBlackboardType\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.TypeNameEditor\")")
    StringAnsi BlackboardType;

    // List of full typenames of the behavior goals (structure or class).
    API_FIELD(Attributes="EditorOrder(10), Collection(OverrideEditorTypeName=\"FlaxEditor.CustomEditors.Editors.TypeNameEditor\")")
    Array<StringAnsi> GoalTypes;

    // The target amount of the behavior logic updates per second.
    API_FIELD(Attributes="EditorOrder(100)")
    float UpdateFPS = 10.0f;
};

/// <summary>
/// Delay node that waits a specific amount of time while executed.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeDelayNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeDelayNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // Time in seconds to wait when node gets activated. Unused if WaitTimeSelector is used.
    API_FIELD(Attributes="EditorOrder(10), Limit(0)")
    float WaitTime = 3.0f;

    // Wait time randomization range to deviate original value.
    API_FIELD(Attributes="EditorOrder(20), Limit(0)")
    float RandomDeviation = 0.0f;

    // Wait time from behavior's knowledge (blackboard, goal or sensor). If set, overrides WaitTime but still uses RandomDeviation.
    API_FIELD(Attributes="EditorOrder(30)")
    BehaviorKnowledgeSelector<float> WaitTimeSelector;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;
#if USE_EDITOR
    String GetDebugInfo(const BehaviorUpdateContext& context) const override;
#endif

private:
    struct State
    {
        float TimeLeft;
    };
};

/// <summary>
/// Sub-tree node runs a nested Behavior Tree within this tree.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeSubTreeNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSubTreeNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // Nested behavior tree to execute within this node.
    API_FIELD(Attributes="EditorOrder(10), Limit(0)")
    AssetReference<BehaviorTree> Tree;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    void ReleaseState(const BehaviorUpdateContext& context) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;

private:
    struct State
    {
        Array<byte> Memory;
        BitArray<> RelevantNodes;
    };
};

/// <summary>
/// Forces behavior execution end with a specific result (eg. force fail).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeForceFinishNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeForceFinishNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // Execution result.
    API_FIELD() BehaviorUpdateResult Result = BehaviorUpdateResult::Success;

public:
    // [BehaviorTreeNode]
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;
};

/// <summary>
/// Moves an actor to the specific target location. Uses pathfinding on navmesh.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorTreeMoveToNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeMoveToNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // The agent actor to move. If not set, uses Behavior's parent actor.
    API_FIELD(Attributes="EditorOrder(10)")
    BehaviorKnowledgeSelector<Actor*> Agent;

    // The agent movement speed. Default value is 100 units/second.
    API_FIELD(Attributes="EditorOrder(15)")
    BehaviorKnowledgeSelector<float> MovementSpeed;

    // The target movement object.
    API_FIELD(Attributes="EditorOrder(30)")
    BehaviorKnowledgeSelector<Actor*> Target;

    // The target movement location.
    API_FIELD(Attributes="EditorOrder(35)")
    BehaviorKnowledgeSelector<Vector3> TargetLocation;

    // Threshold value between Agent and Target goal location for destination reach test.
    API_FIELD(Attributes="EditorOrder(100), Limit(0)")
    float AcceptableRadius = 5.0f;

    // Threshold value for the Target actor location offset that will trigger re-pathing to find a new path.
    API_FIELD(Attributes="EditorOrder(110), Limit(0)")
    float TargetGoalUpdateTolerance = 4.0f;

    // If checked, the movement will use navigation system pathfinding, otherwise direct motion to the target location will happen.
    API_FIELD(Attributes="EditorOrder(120)")
    bool UsePathfinding = true;

    // If checked, the movement will start even if there is no direct path to the target (only partial).
    API_FIELD(Attributes="EditorOrder(130)")
    bool UsePartialPath = true;

    // If checked, the target goal location will be updated while Target actor moves.
    API_FIELD(Attributes="EditorOrder(140)")
    bool UseTargetGoalUpdate = true;

public:
    // Applies the movement to the agent. Returns true if cannot move.
    API_FUNCTION() virtual bool Move(Actor* agent, const Vector3& move) const;

    // Returns the navmesh to use for the path-finding. Can query nav agent properties from the agent actor to select navmesh.
    API_FUNCTION() virtual NavMeshRuntime* GetNavMesh(Actor* agent) const;

    // Returns the agent dimensions used for path following (eg. goal reachability test).
    API_FUNCTION() virtual void GetAgentSize(Actor* agent, API_PARAM(Out) float& outRadius, API_PARAM(Out) float& outHeight) const;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    void ReleaseState(const BehaviorUpdateContext& context) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;
#if USE_EDITOR
    String GetDebugInfo(const BehaviorUpdateContext& context) const override;
#endif

protected:
    struct State
    {
        bool HasPath = false; // True if has Path computed
        bool HasTick = false; // True if OnUpdate is binded
        Vector3 GoalLocation;
        BehaviorTreeMoveToNode* Node;
        BehaviorKnowledge* Knowledge;
        ScriptingObjectReference<Actor> Agent;
        Array<Vector3> Path;
        Vector3 AgentOffset; // Offset between agent position and path position (aka feet offset)
        float NavAgentRadius; // Size of the agent used to compute navmesh
        Float3 UpVector; // Path Up vector (from navmesh orientation)
        int32 TargetPathIndex; // Index of the next path point to go to
        BehaviorUpdateResult Result; // Current result of the OnUpdate

        void OnUpdate();
    };
};

/// <summary>
/// Inverts node's result - fails if node succeeded or succeeds if node failed.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeInvertDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeInvertDecorator, BehaviorTreeDecorator);

public:
    // [BehaviorTreeNode]
    void PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result) override;
};

/// <summary>
/// Forces node to success - even if it failed.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeForceSuccessDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeForceSuccessDecorator, BehaviorTreeDecorator);

public:
    // [BehaviorTreeNode]
    void PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result) override;
};

/// <summary>
/// Forces node to fail - even if it succeeded.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeForceFailedDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeForceFailedDecorator, BehaviorTreeDecorator);

public:
    // [BehaviorTreeNode]
    void PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result) override;
};

/// <summary>
/// Loops node execution multiple times as long as it doesn't fail. Returns the last iteration result.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeLoopDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeLoopDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // Is the loop infinite (until failed)?
    API_FIELD(Attributes="EditorOrder(10)")
    bool InfiniteLoop = false;

    // Amount of times to execute the node. Unused if LoopCountSelector is used or if InfiniteLoop is used.
    API_FIELD(Attributes="EditorOrder(20), Limit(0), VisibleIf(nameof(InfiniteLoop), true)")
    int32 LoopCount = 3;

    // Amount of times to execute the node from behavior's knowledge (blackboard, goal or sensor). If set, overrides LoopCount. Unused if InfiniteLoop is used.
    API_FIELD(Attributes="EditorOrder(30), VisibleIf(nameof(InfiniteLoop), true)")
    BehaviorKnowledgeSelector<int32> LoopCountSelector;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    void PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result) override;

    struct State
    {
        int32 Loops;
    };
};

/// <summary>
/// Limits maximum duration of the node execution time (in seconds). Node will fail if it runs out of time.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeTimeLimitDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeTimeLimitDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // Maximum node execution time (in seconds). Unused if MaxDurationSelector is used.
    API_FIELD(Attributes="EditorOrder(10), Limit(0)")
    float MaxDuration = 3.0;

    // Duration time randomization range to deviate original value.
    API_FIELD(Attributes="EditorOrder(20), Limit(0)")
    float RandomDeviation = 0.0f;

    // Maximum node execution time (in seconds) from behavior's knowledge (blackboard, goal or sensor). If set, overrides MaxDuration but still uses RandomDeviation.
    API_FIELD(Attributes="EditorOrder(20)")
    BehaviorKnowledgeSelector<float> MaxDurationSelector;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    BehaviorUpdateResult Update(const BehaviorUpdateContext& context) override;

    struct State
    {
        float TimeLeft;
    };
};

/// <summary>
/// Adds cooldown in between node executions. Blocks node execution for a given duration after last run.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeCooldownDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeCooldownDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // Minimum cooldown time (in seconds). Unused if MinDurationSelector is used.
    API_FIELD(Attributes="EditorOrder(10), Limit(0)")
    float MinDuration = 3.0;

    // Duration time randomization range to deviate original value.
    API_FIELD(Attributes="EditorOrder(20), Limit(0)")
    float RandomDeviation = 0.0f;

    // Minimum cooldown time (in seconds) from behavior's knowledge (blackboard, goal or sensor). If set, overrides MinDuration but still uses RandomDeviation.
    API_FIELD(Attributes="EditorOrder(20)")
    BehaviorKnowledgeSelector<float> MinDurationSelector;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(const BehaviorUpdateContext& context) override;
    void ReleaseState(const BehaviorUpdateContext& context) override;
    bool CanUpdate(const BehaviorUpdateContext& context) override;
    void PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result) override;

    struct State
    {
        float EndTime;
    };
};

/// <summary>
/// Checks certain knowledge value to conditionally enter the node.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeKnowledgeConditionalDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeKnowledgeConditionalDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // The first value from behavior's knowledge (blackboard, goal or sensor) to use for comparision.
    API_FIELD(Attributes="EditorOrder(0)")
    BehaviorKnowledgeSelectorAny ValueA;

    // The second value to use for comparision (constant).
    API_FIELD(Attributes="EditorOrder(10)")
    float ValueB = 0.0f;

    // Values comparision mode.
    API_FIELD(Attributes="EditorOrder(20)")
    BehaviorValueComparison Comparison = BehaviorValueComparison::Equal;

public:
    // [BehaviorTreeNode]
    bool CanUpdate(const BehaviorUpdateContext& context) override;
};

/// <summary>
/// Checks certain knowledge value to conditionally enter the node.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeKnowledgeValuesConditionalDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeKnowledgeValuesConditionalDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // The first value from behavior's knowledge (blackboard, goal or sensor) to use for comparision.
    API_FIELD(Attributes="EditorOrder(0)")
    BehaviorKnowledgeSelectorAny ValueA;

    // The second value from behavior's knowledge (blackboard, goal or sensor) to use for comparision.
    API_FIELD(Attributes="EditorOrder(10)")
    BehaviorKnowledgeSelectorAny ValueB;

    // Values comparision mode.
    API_FIELD(Attributes="EditorOrder(20)")
    BehaviorValueComparison Comparison = BehaviorValueComparison::Equal;

public:
    // [BehaviorTreeNode]
    bool CanUpdate(const BehaviorUpdateContext& context) override;
};

/// <summary>
/// Checks certain knowledge value to conditionally enter the node if the value is set (eg. not-null object reference or boolean value).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeKnowledgeBooleanDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeKnowledgeBooleanDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // The value from behavior's knowledge (blackboard, goal or sensor) to check if it's set (eg. not-null object reference or boolean value).
    API_FIELD(Attributes="EditorOrder(0)")
    BehaviorKnowledgeSelectorAny Value;

    // If checked, the condition will be inverted.
    API_FIELD(Attributes="EditorOrder(10)")
    bool Invert = false;

public:
    // [BehaviorTreeNode]
    bool CanUpdate(const BehaviorUpdateContext& context) override;
};

/// <summary>
/// Checks if certain actor (from knowledge) has a given tag assigned.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeHasTagDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeHasTagDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // The actor value from behavior's knowledge (blackboard, goal or sensor) to check against tag ownership.
    API_FIELD(Attributes="EditorOrder(0)")
    BehaviorKnowledgeSelector<Actor*> Actor;

    // The tag to check.
    API_FIELD(Attributes="EditorOrder(10)")
    Tag Tag;

    // If checked, inverts condition - checks if actor doesn't have a tag.
    API_FIELD(Attributes="EditorOrder(20)")
    bool Invert = false;

public:
    // [BehaviorTreeNode]
    bool CanUpdate(const BehaviorUpdateContext& context) override;
};

/// <summary>
/// Checks if certain goal has been added to Behavior knowledge.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeHasGoalDecorator : public BehaviorTreeDecorator
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeHasGoalDecorator, BehaviorTreeDecorator);
    API_AUTO_SERIALIZATION();

    // The goal type to check.
    API_FIELD(Attributes="EditorOrder(0), BehaviorKnowledgeSelector(IsGoalSelector = true)")
    BehaviorKnowledgeSelectorAny Goal;

public:
    // [BehaviorTreeNode]
    bool CanUpdate(const BehaviorUpdateContext& context) override;
};
