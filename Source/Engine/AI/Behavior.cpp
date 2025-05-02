// Copyright (c) Wojciech Figat. All rights reserved.

#include "Behavior.h"
#include "BehaviorKnowledge.h"
#include "BehaviorTreeNodes.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/TaskGraph.h"

class BehaviorSystem : public TaskGraphSystem
{
public:
    Array<Behavior*> Behaviors;

    void Job(int32 index);
    void Execute(TaskGraph* graph) override;
};

class BehaviorService : public EngineService
{
public:
    Array<Behavior*> UpdateList;

    BehaviorService()
        : EngineService(TEXT("Behaviors"), 0)
    {
    }

    bool Init() override;
    void Dispose() override;
};

BehaviorService BehaviorServiceInstance;
TaskGraphSystem* Behavior::System = nullptr;

void BehaviorSystem::Job(int32 index)
{
    PROFILE_CPU_NAMED("Behavior.Job");
    Behaviors[index]->UpdateAsync();
}

void BehaviorSystem::Execute(TaskGraph* graph)
{
    // Copy list of behaviors to update (in case one of them gets disabled during async jobs)
    if (BehaviorServiceInstance.UpdateList.Count() == 0)
        return;
    Behaviors.Clear();
    Behaviors.Add(BehaviorServiceInstance.UpdateList);

    // Schedule work to update all behaviors in async
    Function<void(int32)> job;
    job.Bind<BehaviorSystem, &BehaviorSystem::Job>(this);
    graph->DispatchJob(job, Behaviors.Count());
}

bool BehaviorService::Init()
{
    Behavior::System = New<BehaviorSystem>();
    Engine::UpdateGraph->AddSystem(Behavior::System);
    return false;
}

void BehaviorService::Dispose()
{
    BehaviorServiceInstance.UpdateList.Resize(0);
    SAFE_DELETE(Behavior::System);
}

Behavior::Behavior(const SpawnParams& params)
    : Script(params)
{
    _knowledge.Behavior = this;
    Tree.Changed.Bind<Behavior, &Behavior::ResetLogic>(this);
}

void Behavior::UpdateAsync()
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

void Behavior::StartLogic()
{
    if (_result == BehaviorUpdateResult::Running)
        return;
    PROFILE_CPU();

    // Ensure to have tree loaded on play
    CHECK(Tree && !Tree->WaitForLoaded());
    BehaviorTree* tree = Tree.Get();
    CHECK(tree->Graph.Root);

    // Setup state
    _result = BehaviorUpdateResult::Running;
    _accumulatedTime = 0.0f;
    _totalTime = 0;

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
    _knowledge.FreeMemory();
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
    BehaviorServiceInstance.UpdateList.Add(this);
    if (AutoStart)
        StartLogic();
}

void Behavior::OnDisable()
{
    BehaviorServiceInstance.UpdateList.Remove(this);
}

#if USE_EDITOR

bool Behavior::GetNodeDebugRelevancy(const BehaviorTreeNode* node, const Behavior* behavior)
{
    return node &&
            behavior &&
            node->_executionIndex >= 0 &&
            node->_executionIndex < behavior->_knowledge.RelevantNodes.Count() &&
            behavior->_knowledge.RelevantNodes.Get(node->_executionIndex);
}

String Behavior::GetNodeDebugInfo(const BehaviorTreeNode* node, Behavior* behavior)
{
    if (!node)
        return String::Empty;
    BehaviorUpdateContext context;
    Platform::MemoryClear(&context, sizeof(context));
    if (GetNodeDebugRelevancy(node, behavior))
    {
        // Pass behavior and knowledge data only for relevant nodes to properly access it
        context.Behavior = behavior;
        context.Knowledge = &behavior->_knowledge;
        context.Memory = behavior->_knowledge.Memory;
        context.RelevantNodes = &behavior->_knowledge.RelevantNodes;
        context.Time = behavior->_totalTime;
    }
    return node->GetDebugInfo(context);
}

#endif
