// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Behavior.h"
#include "BehaviorKnowledge.h"
#include "BehaviorTreeNodes.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"

Behavior::Behavior(const SpawnParams& params)
    : Script(params)
{
    _tickLateUpdate = 1; // TODO: run Behavior via Job System (use Engine::UpdateGraph)
    _knowledge.Behavior = this;
    Tree.Changed.Bind<Behavior, &Behavior::ResetLogic>(this);
}

void Behavior::StartLogic()
{
    PROFILE_CPU();

    // Ensure to have tree loaded on begin play
    CHECK(Tree && !Tree->WaitForLoaded());
    BehaviorTree* tree = Tree.Get();
    CHECK(tree->Graph.Root);

    _result = BehaviorUpdateResult::Running;

    // Init knowledge
    _knowledge.InitMemory(tree);
}

void Behavior::StopLogic(BehaviorUpdateResult result)
{
    if (_result != BehaviorUpdateResult::Running || result == BehaviorUpdateResult::Running)
        return;
    PROFILE_CPU();
    _accumulatedTime = 0.0f;
    _totalTime = 0;
    _result = result;
}

void Behavior::ResetLogic()
{
    PROFILE_CPU();
    const bool isActive = _result == BehaviorUpdateResult::Running;
    if (isActive)
        StopLogic();

    // Reset state
    _knowledge.FreeMemory();
    _accumulatedTime = 0.0f;
    _totalTime = 0;
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
    PROFILE_CPU();
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
    _totalTime += updateDeltaTime;

    // Update tree
    BehaviorUpdateContext context;
    context.Behavior = this;
    context.Knowledge = &_knowledge;
    context.Memory = _knowledge.Memory;
    context.RelevantNodes = &_knowledge.RelevantNodes;
    context.DeltaTime = updateDeltaTime;
    context.Time = _totalTime;
    const BehaviorUpdateResult result = tree->Graph.Root->InvokeUpdate(context);
    if (result != BehaviorUpdateResult::Running)
        _result = result;
    if (_result != BehaviorUpdateResult::Running)
    {
        Finished();
    }
}
