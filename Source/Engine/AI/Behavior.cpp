// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Behavior.h"
#include "BehaviorKnowledge.h"
#include "BehaviorTreeNodes.h"
#include "Engine/Engine/Time.h"

BehaviorKnowledge::~BehaviorKnowledge()
{
    FreeMemory();
}

void BehaviorKnowledge::InitMemory(BehaviorTree* tree)
{
    ASSERT_LOW_LAYER(!Tree && tree);
    Tree = tree;
    Blackboard = Variant::NewValue(tree->Graph.Root->BlackboardType);
    RelevantNodes.Resize(tree->Graph.NodesCount, false);
    RelevantNodes.SetAll(false);
    if (!Memory && tree->Graph.NodesStatesSize)
        Memory = Allocator::Allocate(tree->Graph.NodesStatesSize);
}

void BehaviorKnowledge::FreeMemory()
{
    if (Memory)
    {
        // Release any outstanding nodes state and memory
        ASSERT_LOW_LAYER(Tree);
        for (const auto& node : Tree->Graph.Nodes)
        {
            if (node.Instance && node.Instance->_executionIndex != -1 && RelevantNodes[node.Instance->_executionIndex])
                node.Instance->ReleaseState(Behavior, Memory);
        }
        Allocator::Free(Memory);
        Memory = nullptr;
    }
    RelevantNodes.Clear();
    Blackboard.DeleteValue();
    Tree = nullptr;
}

Behavior::Behavior(const SpawnParams& params)
    : Script(params)
{
    _tickLateUpdate = 1; // TODO: run Behavior via Job System (use Engine::UpdateGraph)
    _knowledge.Behavior = this;
    Tree.Changed.Bind<Behavior, &Behavior::ResetLogic>(this);
}

void Behavior::StartLogic()
{
    // Ensure to have tree loaded on begin play
    CHECK(Tree && !Tree->WaitForLoaded());
    BehaviorTree* tree = Tree.Get();
    CHECK(tree->Graph.Root);

    _result = BehaviorUpdateResult::Running;

    // Init knowledge
    _knowledge.InitMemory(tree);
}

void Behavior::StopLogic()
{
    if (_result != BehaviorUpdateResult::Running)
        return;
    _accumulatedTime = 0.0f;
    _result = BehaviorUpdateResult::Success;
}

void Behavior::ResetLogic()
{
    const bool isActive = _result == BehaviorUpdateResult::Running;
    if (isActive)
        StopLogic();

    // Reset state
    _knowledge.FreeMemory();
    _accumulatedTime = 0.0f;
    _result = BehaviorUpdateResult::Success;

    if (isActive)
        StartLogic();
}

void Behavior::OnEnable()
{
    if (AutoStart)
        StartLogic();
}

void Behavior::OnLateUpdate()
{
    if (_result != BehaviorUpdateResult::Running)
        return;
    const BehaviorTree* tree = Tree.Get();
    if (!tree || !tree->Graph.Root)
    {
        _result = BehaviorUpdateResult::Failed;
        Finished();
        return;
    }

    // Update timer
    _accumulatedTime += Time::Update.DeltaTime.GetTotalSeconds();
    const float updateDeltaTime = 1.0f / Math::Max(tree->Graph.Root->UpdateFPS * UpdateRateScale, ZeroTolerance);
    if (_accumulatedTime < updateDeltaTime)
        return;
    _accumulatedTime -= updateDeltaTime;

    // Update tree
    BehaviorUpdateContext context;
    context.Behavior = this;
    context.Knowledge = &_knowledge;
    context.Memory = _knowledge.Memory;
    context.DeltaTime = updateDeltaTime;
    const BehaviorUpdateResult result = tree->Graph.Root->InvokeUpdate(context);
    if (result != BehaviorUpdateResult::Running)
    {
        _result = result;
        Finished();
    }
}
