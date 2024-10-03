// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "BehaviorTreeNodes.h"
#include "Behavior.h"
#include "BehaviorKnowledge.h"
#include "Engine/Core/Random.h"
#include "Engine/Scripting/Scripting.h"
#if USE_CSHARP
#include "Engine/Core/Utilities.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#endif
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Actor.h"
#include "Engine/Navigation/NavMeshRuntime.h"
#include "Engine/Physics/Actors/RigidBody.h"
#include "Engine/Physics/Colliders/CapsuleCollider.h"
#include "Engine/Physics/Colliders/CharacterController.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"

bool IsAssignableFrom(const StringAnsiView& to, const StringAnsiView& from)
{
    // Special case of null
    if (to.IsEmpty())
        return from.IsEmpty();
    if (from.IsEmpty())
        return false;

    // Exact typename math
    if (to == from)
        return true;

    // Scripting Type match
    const ScriptingTypeHandle typeHandleTo = Scripting::FindScriptingType(to);
    const ScriptingTypeHandle typeHandleFrom = Scripting::FindScriptingType(from);
    if (typeHandleTo && typeHandleFrom)
        return typeHandleTo.IsAssignableFrom(typeHandleFrom);

#if USE_CSHARP
    // MClass match
    const auto mclassTo = Scripting::FindClass(to);
    const auto mclassFrom = Scripting::FindClass(from);
    if (mclassTo && mclassFrom)
        return mclassTo == mclassFrom || mclassFrom->IsSubClassOf(mclassTo);
#endif

    return false;
}

BehaviorUpdateResult BehaviorTreeNode::InvokeUpdate(const BehaviorUpdateContext& context)
{
    ASSERT_LOW_LAYER(_executionIndex != -1);
    const BitArray<>& relevantNodes = *(const BitArray<>*)context.RelevantNodes;

    // If node is not yet relevant
    if (relevantNodes.Get(_executionIndex) == false)
    {
        // Check decorators if node can be executed
        for (BehaviorTreeDecorator* decorator : _decorators)
        {
            ASSERT_LOW_LAYER(decorator->_executionIndex != -1);
            if (relevantNodes.Get(decorator->_executionIndex) == false)
                decorator->BecomeRelevant(context);
            if (!decorator->CanUpdate(context))
            {
                return BehaviorUpdateResult::Failed;
            }
        }

        // Make node relevant
        BecomeRelevant(context);
    }

    // Update decorators
    bool decoratorFailed = false;
    for (BehaviorTreeDecorator* decorator : _decorators)
    {
        decoratorFailed |= decorator->Update(context) == BehaviorUpdateResult::Failed;
    }

    // Node-specific update
    BehaviorUpdateResult result;
    if (decoratorFailed)
        result = BehaviorUpdateResult::Failed;
    else
        result = Update(context);
    if ((int32)result < 0 ||  (int32)result > (int32)BehaviorUpdateResult::Failed)
        result = BehaviorUpdateResult::Failed; // Invalid value is a failure

    // Post-process result from decorators
    for (BehaviorTreeDecorator* decorator : _decorators)
    {
        decorator->PostUpdate(context, result);
    }

    // Check if node is not relevant anymore
    if (result != BehaviorUpdateResult::Running)
        BecomeIrrelevant(context);

    return result;
}

void BehaviorTreeNode::BecomeRelevant(const BehaviorUpdateContext& context)
{
    // Initialize state
    BitArray<>& relevantNodes = *(BitArray<>*)context.RelevantNodes;
    ASSERT_LOW_LAYER(relevantNodes.Get(_executionIndex) == false);
    relevantNodes.Set(_executionIndex, true);
    InitState(context);
}

void BehaviorTreeNode::BecomeIrrelevant(const BehaviorUpdateContext& context)
{
    // Release state
    BitArray<>& relevantNodes = *(BitArray<>*)context.RelevantNodes;
    ASSERT_LOW_LAYER(relevantNodes.Get(_executionIndex) == true);
    relevantNodes.Set(_executionIndex, false);
    ReleaseState(context);

    // Release decorators
    for (BehaviorTreeDecorator* decorator : _decorators)
    {
        if (relevantNodes.Get(decorator->_executionIndex) == true)
        {
            decorator->BecomeIrrelevant(context);
        }
    }
}

void BehaviorTreeNode::Serialize(SerializeStream& stream, const void* otherObj)
{
    SerializableScriptingObject::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(BehaviorTreeNode);
    SERIALIZE(Name);
}

void BehaviorTreeNode::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    SerializableScriptingObject::Deserialize(stream, modifier);

    Name.Clear(); // Missing Name is assumed as unnamed node
    DESERIALIZE(Name);
}

void BehaviorTreeCompoundNode::Init(BehaviorTree* tree)
{
    for (BehaviorTreeNode* child : Children)
        child->Init(tree);
}

BehaviorUpdateResult BehaviorTreeCompoundNode::Update(const BehaviorUpdateContext& context)
{
    auto result = BehaviorUpdateResult::Success;
    for (int32 i = 0; i < Children.Count() && result == BehaviorUpdateResult::Success; i++)
    {
        BehaviorTreeNode* child = Children[i];
        result = child->InvokeUpdate(context);
    }
    return result;
}

void BehaviorTreeCompoundNode::BecomeIrrelevant(const BehaviorUpdateContext& context)
{
    // Make any nested nodes irrelevant as well
    const BitArray<>& relevantNodes = *(const BitArray<>*)context.RelevantNodes;
    for (BehaviorTreeNode* child : Children)
    {
        if (relevantNodes.Get(child->_executionIndex) == true)
        {
            child->BecomeIrrelevant(context);
        }
    }

    BehaviorTreeNode::BecomeIrrelevant(context);
}

int32 BehaviorTreeSequenceNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeSequenceNode::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    new(state)State();
}

BehaviorUpdateResult BehaviorTreeSequenceNode::Update(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);

    if (state->CurrentChildIndex >= Children.Count())
        return BehaviorUpdateResult::Success;
    if (state->CurrentChildIndex == -1)
        return BehaviorUpdateResult::Failed;

    auto result = Children[state->CurrentChildIndex]->InvokeUpdate(context);
    switch (result)
    {
    case BehaviorUpdateResult::Success:
        state->CurrentChildIndex++; // Move to the next node
        if (state->CurrentChildIndex < Children.Count())
            result = BehaviorUpdateResult::Running; // Keep on running to the next child on the next update
        break;
    case BehaviorUpdateResult::Failed:
        state->CurrentChildIndex = -1; // Mark whole sequence as failed
        break;
    }

    return result;
}

int32 BehaviorTreeSelectorNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeSelectorNode::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    new(state)State();
}

BehaviorUpdateResult BehaviorTreeSelectorNode::Update(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);

    if (state->CurrentChildIndex >= Children.Count())
        return BehaviorUpdateResult::Failed;

    auto result = Children[state->CurrentChildIndex]->InvokeUpdate(context);
    switch (result)
    {
    case BehaviorUpdateResult::Success:
        return BehaviorUpdateResult::Success;
    case BehaviorUpdateResult::Failed:
        state->CurrentChildIndex++; // Move to the next node
        if (state->CurrentChildIndex < Children.Count())
            result = BehaviorUpdateResult::Running; // Keep on running to the next child on the next update
        break;
    }

    return result;
}

int32 BehaviorTreeDelayNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeDelayNode::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    if (!WaitTimeSelector.TryGet(context.Knowledge, state->TimeLeft))
        state->TimeLeft = WaitTime;
    state->TimeLeft = Random::RandRange(Math::Max(state->TimeLeft - RandomDeviation, 0.0f), state->TimeLeft + RandomDeviation);
}

BehaviorUpdateResult BehaviorTreeDelayNode::Update(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    state->TimeLeft -= context.DeltaTime;
    return state->TimeLeft <= 0.0f ? BehaviorUpdateResult::Success : BehaviorUpdateResult::Running;
}

#if USE_EDITOR

String BehaviorTreeDelayNode::GetDebugInfo(const BehaviorUpdateContext& context) const
{
    if (context.Memory)
    {
        const auto state = GetState<State>(context.Memory);
        return String::Format(TEXT("Time Left: {}s"), Utilities::RoundTo2DecimalPlaces(state->TimeLeft));
    }

    String delay;
    if (WaitTimeSelector.Path.HasChars())
        delay = String(WaitTimeSelector.Path);
    else
        delay = StringUtils::ToString(WaitTime);
    if (RandomDeviation > 0.0f)
        delay += String::Format(TEXT("+/-{}"), RandomDeviation);
    return String::Format(TEXT("Delay: {}s"), delay);
}

#endif

int32 BehaviorTreeSubTreeNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeSubTreeNode::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    new(state)State();
    const BehaviorTree* tree = Tree.Get();
    if (!tree || tree->WaitForLoaded())
        return;
    state->Memory.Resize(tree->Graph.NodesStatesSize);
    state->RelevantNodes.Resize(tree->Graph.NodesCount, false);
    state->RelevantNodes.SetAll(false);
}

void BehaviorTreeSubTreeNode::ReleaseState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    const BehaviorTree* tree = Tree.Get();
    if (tree && tree->IsLoaded())
    {
        // Override memory with custom one for the subtree
        BehaviorUpdateContext subContext = context;
        subContext.Memory = state->Memory.Get();
        subContext.RelevantNodes = &state->RelevantNodes;

        for (const auto& node : tree->Graph.Nodes)
        {
            if (node.Instance && node.Instance->_executionIndex != -1 && state->RelevantNodes.HasItems() && state->RelevantNodes[node.Instance->_executionIndex])
                node.Instance->ReleaseState(subContext);
        }
    }
    state->~State();
}

BehaviorUpdateResult BehaviorTreeSubTreeNode::Update(const BehaviorUpdateContext& context)
{
    const BehaviorTree* tree = Tree.Get();
    if (!tree || !tree->Graph.Root)
        return BehaviorUpdateResult::Failed;
    const StringAnsiView treeBlackboardType = tree->Graph.Root->BlackboardType;
    if (treeBlackboardType.HasChars())
    {
        // Validate if nested tree blackboard data matches (the same type or base type)
        const VariantType& blackboardType = context.Knowledge->Blackboard.Type;
        if (IsAssignableFrom(treeBlackboardType, StringAnsiView(blackboardType.GetTypeName())))
        {
            LOG(Error, "Cannot use nested '{}' with Blackboard of type '{}' inside '{}' with Blackboard of type '{}'",
                tree->ToString(), String(treeBlackboardType),
                context.Knowledge->Tree->ToString(), blackboardType.ToString());
            return BehaviorUpdateResult::Failed;
        }
    }

    // Override memory with custom one for the subtree
    auto state = GetState<State>(context.Memory);
    BehaviorUpdateContext subContext = context;
    subContext.Memory = state->Memory.Get();
    subContext.RelevantNodes = &state->RelevantNodes;

    // Run nested tree
    return tree->Graph.Root->InvokeUpdate(subContext);
}

BehaviorUpdateResult BehaviorTreeForceFinishNode::Update(const BehaviorUpdateContext& context)
{
    context.Behavior->StopLogic(Result);
    return Result;
}

bool BehaviorTreeMoveToNode::Move(Actor* agent, const Vector3& move) const
{
    agent->AddMovement(move);
    return false;
}

NavMeshRuntime* BehaviorTreeMoveToNode::GetNavMesh(Actor* agent) const
{
    return NavMeshRuntime::Get();
}

void BehaviorTreeMoveToNode::GetAgentSize(Actor* agent, float& outRadius, float& outHeight) const
{
    if (const auto* characterController = Cast<CharacterController>(agent))
    {
        // Character Controller is an capsule
        outRadius = characterController->GetRadius();
        outHeight = characterController->GetHeight() + 2 * outRadius;
        return;
    }
    if (const auto* rigidBody = Cast<RigidBody>(agent))
    {
        // Rigid Body with a single Capsule collider (directed Up)
        Array<Collider*, InlinedAllocation<16>> colliders;
        rigidBody->GetColliders(colliders);
        const auto* capsuleCollider = colliders.Count() == 1 ? (CapsuleCollider*)colliders[0] : nullptr;
        if (capsuleCollider && (capsuleCollider->GetLocalOrientation() == Quaternion::Euler(0, 0, 90) || capsuleCollider->GetLocalOrientation() == Quaternion::Euler(0, 0, -90)))
        {
            outRadius = capsuleCollider->GetRadius();
            outHeight = capsuleCollider->GetHeight() + 2 * outRadius;
            return;
        }
    }

    // Estimate actor bounds to extract capsule information
    const BoundingBox box = agent->GetBox();
    const BoundingSphere sphere = agent->GetSphere();
    outRadius = (float)sphere.Radius;
    outHeight = (float)box.GetSize().Y;
}

int32 BehaviorTreeMoveToNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeMoveToNode::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    new(state)State();
    state->Node = this;
    state->Knowledge = context.Knowledge;

    // Get agent to move
    if (Agent.Path.HasChars())
        state->Agent = Agent.Get(context.Knowledge);
    else
        state->Agent = context.Behavior->GetActor();
}

void BehaviorTreeMoveToNode::ReleaseState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    if (state->HasTick)
        Engine::Update.Unbind<State, &State::OnUpdate>(state);
    state->~State();
}

BehaviorUpdateResult BehaviorTreeMoveToNode::Update(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    if (state->Agent == nullptr)
        return BehaviorUpdateResult::Failed;
    bool repath = !state->HasPath;

    Vector3 goalLocation = state->GoalLocation;
    if (repath || UseTargetGoalUpdate)
    {
        // Get current goal location
        const Actor* target = Target.Get(context.Knowledge);
        if (target)
            goalLocation = target->GetPosition();
        else
            goalLocation = TargetLocation.Get(context.Knowledge);
        repath |= Vector3::Distance(goalLocation, state->GoalLocation) > TargetGoalUpdateTolerance;
    }

    if (repath)
    {
        // Clear path
        state->HasPath = false;
        state->Path.Clear();
        state->AgentOffset = Vector3::Zero;
        state->UpVector = Float3::Up;
        state->NavAgentRadius = 0;

        // Find a new path
        const Vector3 agentLocation = state->Agent->GetPosition();
        if (UsePathfinding)
        {
            const NavMeshRuntime* navMesh = GetNavMesh(state->Agent);
            if (!navMesh)
                return BehaviorUpdateResult::Failed;
            NavMeshPathFlags pathFlags;
            if (!navMesh->FindPath(agentLocation, goalLocation, state->Path, pathFlags))
                return BehaviorUpdateResult::Failed;
            if (!UsePartialPath && EnumHasAnyFlags(pathFlags, NavMeshPathFlags::PartialPath))
                return BehaviorUpdateResult::Failed;
            state->UpVector = Float3::Transform(Float3::Up, navMesh->Properties.Rotation);
            state->NavAgentRadius = navMesh->Properties.Agent.Radius;

            // Place start and end on navmesh
            navMesh->FindClosestPoint(state->Path.First(), state->Path.First());
            navMesh->FindClosestPoint(state->Path.Last(), state->Path.Last());

            // Calculate offset between path and the agent (aka feet offset)
            state->AgentOffset = state->Path.First() - agentLocation;
        }
        else
        {
            // Dummy movement
            state->Path.Resize(2);
            state->Path.Get()[0] = agentLocation;
            state->Path.Get()[1] = goalLocation;
        }

        // Start path following
        state->HasPath = true;
        state->TargetPathIndex = 1;
        state->Result = BehaviorUpdateResult::Running;
        state->GoalLocation = goalLocation;

        // TODO: add path debugging in Editor (eg. via BT window)

        // Register for ticking the path following logic at game update rate (BT usually use lower FPS due to performance)
        if (!state->HasTick)
        {
            state->HasTick = true;
            Engine::Update.Bind<State, &State::OnUpdate>(state);
        }
    }

    return state->Result;
}

#if USE_EDITOR

String BehaviorTreeMoveToNode::GetDebugInfo(const BehaviorUpdateContext& context) const
{
    if (context.Memory)
    {
        const auto state = GetState<State>(context.Memory);
        if (state->Agent)
        {
            const String agent = state->Agent->GetNamePath();
            String goal;
            const Actor* target = Target.Get(context.Knowledge);
            if (target)
                goal = target->GetNamePath();
            else
                goal = state->GoalLocation.ToString();
            const Vector3 agentLocation = state->Agent->GetPosition();
            const Vector3 agentLocationOnPath = agentLocation + state->AgentOffset;
            Real distanceLeft = state->Path.Count() > state->TargetPathIndex ? Vector3::Distance(state->Path[state->TargetPathIndex], agentLocationOnPath) : 0;
            for (int32 i = state->TargetPathIndex; i < state->Path.Count(); i++)
                distanceLeft += Vector3::Distance(state->Path[i - 1], state->Path[i]);
            return String::Format(TEXT("Agent: '{}'\nGoal: '{}'\nDistance: {}"), agent, goal, (int32)distanceLeft);
        }
    }
    return String::Empty;
}

#endif

void BehaviorTreeMoveToNode::State::OnUpdate()
{
    if (Result != BehaviorUpdateResult::Running)
        return;
    PROFILE_CPU();

    // Get agent properties
    const Vector3 agentLocation = Agent->GetPosition();
    float movementSpeed;
    if (!Node->MovementSpeed.TryGet(Knowledge, movementSpeed))
        movementSpeed = 100;
    float agentRadius = 30.0f, agentHeight = 100.0f;
    Node->GetAgentSize(Agent, agentRadius, agentHeight);

    // Test if agent reached the next path segment
    Vector3 pathSegmentEnd = Path[TargetPathIndex];
    const Vector3 agentLocationOnPath = agentLocation + AgentOffset;
    const bool isLastSegment = TargetPathIndex + 1 == Path.Count();
    float testRadius;
    if (isLastSegment)
        testRadius = agentRadius + Node->AcceptableRadius;
    else
        testRadius = agentRadius * 0.05f + Math::Max(agentRadius - NavAgentRadius, 0.0f); // 5% threshold of agent radius and diff between navmesh vs agent radius as threshold for path segments reaching
    const float acceptableHeightPercentage = 1.05f;
    const float testHeight = agentHeight * acceptableHeightPercentage;
    const Vector3 toGoal = agentLocationOnPath - pathSegmentEnd;
    const Real toGoalHeightDiff = (toGoal * UpVector).SumValues();
    if (toGoal.Length() <= testRadius && toGoalHeightDiff <= testHeight)
    {
        TargetPathIndex++;
        if (TargetPathIndex == Path.Count())
        {
            // Goal reached!
            Result = BehaviorUpdateResult::Success;
            return;
        }
        pathSegmentEnd = Path[TargetPathIndex];
    }

    // Move agent
    const float maxMove = movementSpeed * Time::Update.DeltaTime.GetTotalSeconds();
    if (maxMove <= ZeroTolerance)
        return;
    const Vector3 move = Vector3::MoveTowards(agentLocationOnPath, pathSegmentEnd, maxMove) - agentLocationOnPath;
    if (Node->Move(Agent, move))
    {
        // Move failed!
        Result = BehaviorUpdateResult::Failed;
    }
}

void BehaviorTreeInvertDecorator::PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result)
{
    if (result == BehaviorUpdateResult::Success)
        result = BehaviorUpdateResult::Failed;
    else if (result == BehaviorUpdateResult::Failed)
        result = BehaviorUpdateResult::Success;
}

void BehaviorTreeForceSuccessDecorator::PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result)
{
    if (result != BehaviorUpdateResult::Running)
        result = BehaviorUpdateResult::Success;
}

void BehaviorTreeForceFailedDecorator::PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result)
{
    if (result != BehaviorUpdateResult::Running)
        result = BehaviorUpdateResult::Failed;
}

int32 BehaviorTreeLoopDecorator::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeLoopDecorator::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    if (!LoopCountSelector.TryGet(context.Knowledge, state->Loops))
        state->Loops = LoopCount;
}

void BehaviorTreeLoopDecorator::PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result)
{
    // Continue looping only if node succeeds
    if (result == BehaviorUpdateResult::Success)
    {
        auto state = GetState<State>(context.Memory);
        if (!InfiniteLoop)
            state->Loops--;

        if (state->Loops > 0 || InfiniteLoop)
        {
            // Keep running in a loop but reset node's state (preserve self state)
            result = BehaviorUpdateResult::Running;
            BitArray<>& relevantNodes = *(BitArray<>*)context.RelevantNodes;
            relevantNodes.Set(_executionIndex, false);
            _parent->BecomeIrrelevant(context);
            relevantNodes.Set(_executionIndex, true);
        }
    }
}

int32 BehaviorTreeTimeLimitDecorator::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeTimeLimitDecorator::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    if (!MaxDurationSelector.TryGet(context.Knowledge, state->TimeLeft))
        state->TimeLeft = MaxDuration;
    state->TimeLeft = Random::RandRange(Math::Max(state->TimeLeft - RandomDeviation, 0.0f), state->TimeLeft + RandomDeviation);
}

BehaviorUpdateResult BehaviorTreeTimeLimitDecorator::Update(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    state->TimeLeft -= context.DeltaTime;
    return state->TimeLeft <= 0.0f ? BehaviorUpdateResult::Failed : BehaviorUpdateResult::Success;
}

int32 BehaviorTreeCooldownDecorator::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeCooldownDecorator::InitState(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    state->EndTime = 0; // Allow to entry on start
}

void BehaviorTreeCooldownDecorator::ReleaseState(const BehaviorUpdateContext& context)
{
    // Preserve the decorator's state to keep cooldown
    BitArray<>& relevantNodes = *(BitArray<>*)context.RelevantNodes;
    relevantNodes.Set(_executionIndex, true);
}

bool BehaviorTreeCooldownDecorator::CanUpdate(const BehaviorUpdateContext& context)
{
    auto state = GetState<State>(context.Memory);
    return state->EndTime <= context.Time;
}

void BehaviorTreeCooldownDecorator::PostUpdate(const BehaviorUpdateContext& context, BehaviorUpdateResult& result)
{
    if (result != BehaviorUpdateResult::Running)
    {
        // Initialize cooldown
        auto state = GetState<State>(context.Memory);
        if (!MinDurationSelector.TryGet(context.Knowledge, state->EndTime))
            state->EndTime = MinDuration;
        state->EndTime = Random::RandRange(Math::Max(state->EndTime - RandomDeviation, 0.0f), state->EndTime + RandomDeviation);
        state->EndTime += context.Time;
    }
}

bool BehaviorTreeKnowledgeConditionalDecorator::CanUpdate(const BehaviorUpdateContext& context)
{
    return BehaviorKnowledge::CompareValues((float)ValueA.Get(context.Knowledge), ValueB, Comparison);
}

bool BehaviorTreeKnowledgeValuesConditionalDecorator::CanUpdate(const BehaviorUpdateContext& context)
{
    return BehaviorKnowledge::CompareValues((float)ValueA.Get(context.Knowledge), (float)ValueB.Get(context.Knowledge), Comparison);
}

bool BehaviorTreeKnowledgeBooleanDecorator::CanUpdate(const BehaviorUpdateContext& context)
{
    Variant value = Value.Get(context.Knowledge);
    bool result = (bool)value;
    result ^= Invert;
    return result;
}

bool BehaviorTreeHasTagDecorator::CanUpdate(const BehaviorUpdateContext& context)
{
    bool result = false;
    ::Actor* actor;
    if (Actor.TryGet(context.Knowledge, actor) && actor)
        result = actor->HasTag(Tag);
    result ^= Invert;
    return result;
}

bool BehaviorTreeHasGoalDecorator::CanUpdate(const BehaviorUpdateContext& context)
{
    Variant value; // TODO: use HasGoal in Knowledge to optimize this (goal struct is copied by selector accessor)
    return Goal.TryGet(context.Knowledge, value);
}
