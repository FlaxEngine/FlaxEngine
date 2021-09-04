// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Animations.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Actors/AnimatedModel.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/TaskGraph.h"

class AnimationsService : public EngineService
{
public:

    AnimationsService()
        : EngineService(TEXT("Animations"), -10)
    {
    }

    bool Init() override;
    void Dispose() override;
};

class AnimationsSystem : public TaskGraphSystem
{
public:
    float DeltaTime, UnscaledDeltaTime, Time, UnscaledTime;
    void Job(int32 index);
    void Execute(TaskGraph* graph) override;
    void PostExecute(TaskGraph* graph) override;
};

AnimationsService AnimationManagerInstance;
Array<AnimatedModel*> UpdateList;
TaskGraphSystem* Animations::System = nullptr;
#if USE_EDITOR
Delegate<Asset*, ScriptingObject*, uint32, uint32> Animations::DebugFlow;
#endif

bool AnimationsService::Init()
{
    Animations::System = New<AnimationsSystem>();
    Engine::UpdateGraph->AddSystem(Animations::System);
    return false;
}

void AnimationsService::Dispose()
{
    UpdateList.Resize(0);
    SAFE_DELETE(Animations::System);
}

void AnimationsSystem::Job(int32 index)
{
    PROFILE_CPU_NAMED("Animations.Job");
    auto animatedModel = UpdateList[index];
    auto skinnedModel = animatedModel->SkinnedModel.Get();
    auto graph = animatedModel->AnimationGraph.Get();
    if (graph && graph->IsLoaded() && graph->Graph.CanUseWithSkeleton(skinnedModel)
#if USE_EDITOR
        && graph->Graph.Parameters.Count() == animatedModel->GraphInstance.Parameters.Count() // It may happen in editor so just add safe check to prevent any crashes
#endif
    )
    {
#if COMPILE_WITH_PROFILER && TRACY_ENABLE
        const StringView graphName(graph->GetPath());
        ZoneName(*graphName, graphName.Length());
#endif

        // Prepare skinning data
        animatedModel->SetupSkinningData();

        // Animation delta time can be based on a time since last update or the current delta
        float dt = animatedModel->UseTimeScale ? DeltaTime : UnscaledDeltaTime;
        float t = animatedModel->UseTimeScale ? Time : UnscaledTime;
        const float lastUpdateTime = animatedModel->GraphInstance.LastUpdateTime;
        if (lastUpdateTime > 0 && t > lastUpdateTime)
        {
            dt = t - lastUpdateTime;
        }
        dt *= animatedModel->UpdateSpeed;
        animatedModel->GraphInstance.LastUpdateTime = t;

        // Evaluate animated nodes pose
        graph->GraphExecutor.Update(animatedModel->GraphInstance, dt);

        // Update gameplay
        animatedModel->OnAnimationUpdated_Async();
    }
}

void AnimationsSystem::Execute(TaskGraph* graph)
{
    if (UpdateList.Count() == 0)
        return;

    // Setup data for async update
    const auto& tickData = Time::Update;
    DeltaTime = tickData.DeltaTime.GetTotalSeconds();
    UnscaledDeltaTime = tickData.UnscaledDeltaTime.GetTotalSeconds();
    Time = tickData.Time.GetTotalSeconds();
    UnscaledTime = tickData.UnscaledTime.GetTotalSeconds();

#if USE_EDITOR
    // If debug flow is registered, then warm it up (eg. static cached method inside DebugFlow_ManagedWrapper) so it doesn't crash on highly multi-threaded code
    if (Animations::DebugFlow.IsBinded())
        Animations::DebugFlow(nullptr, nullptr, 0, 0);
#endif

    // Schedule work to update all animated models in async
    Function<void(int32)> job;
    job.Bind<AnimationsSystem, &AnimationsSystem::Job>(this);
    graph->DispatchJob(job, UpdateList.Count());
}

void AnimationsSystem::PostExecute(TaskGraph* graph)
{
    PROFILE_CPU_NAMED("Animations.PostExecute");

    // Update gameplay
    for (int32 index = 0; index < UpdateList.Count(); index++)
    {
        auto animatedModel = UpdateList[index];
        auto skinnedModel = animatedModel->SkinnedModel.Get();
        auto animGraph = animatedModel->AnimationGraph.Get();
        if (animGraph && animGraph->IsLoaded() && animGraph->Graph.CanUseWithSkeleton(skinnedModel)
#if USE_EDITOR
            && animGraph->Graph.Parameters.Count() == animatedModel->GraphInstance.Parameters.Count() // It may happen in editor so just add safe check to prevent any crashes
#endif
        )
        {
            animatedModel->OnAnimationUpdated_Sync();
        }
    }

    // Cleanup
    UpdateList.Clear();
}

void Animations::AddToUpdate(AnimatedModel* obj)
{
    UpdateList.Add(obj);
}

void Animations::RemoveFromUpdate(AnimatedModel* obj)
{
    UpdateList.Remove(obj);
}
