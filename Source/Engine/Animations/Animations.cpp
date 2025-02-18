// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Animations.h"
#include "AnimEvent.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Actors/AnimatedModel.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/TaskGraph.h"

class AnimationsService : public EngineService
{
public:
    Array<AnimatedModel*> UpdateList;

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
    bool Active;

    void Job(int32 index);
    void Execute(TaskGraph* graph) override;
    void PostExecute(TaskGraph* graph) override;
};

namespace
{
    FORCE_INLINE bool CanUpdateModel(const AnimatedModel* animatedModel)
    {
        auto skinnedModel = animatedModel->SkinnedModel.Get();
        auto animGraph = animatedModel->AnimationGraph.Get();
        return animGraph && animGraph->IsLoaded()
                && skinnedModel && skinnedModel->IsLoaded()
#if USE_EDITOR
                // It may happen in editor so just add safe check to prevent any crashes
                && animGraph->Graph.Parameters.Count() == animatedModel->GraphInstance.Parameters.Count()
#endif
                && animGraph->Graph.IsReady();
    }
}

AnimationsService AnimationManagerInstance;
TaskGraphSystem* Animations::System = nullptr;
ConcurrentSystemLocker Animations::SystemLocker;
#if USE_EDITOR
Delegate<Animations::DebugFlowInfo> Animations::DebugFlow;
#endif

AnimEvent::AnimEvent(const SpawnParams& params)
    : SerializableScriptingObject(params)
{
}

AnimContinuousEvent::AnimContinuousEvent(const SpawnParams& params)
    : AnimEvent(params)
{
}

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
    auto animatedModel = AnimationManagerInstance.UpdateList[index];
    if (CanUpdateModel(animatedModel))
    {
        auto graph = animatedModel->AnimationGraph.Get();
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
    if (AnimationManagerInstance.UpdateList.Count() == 0)
        return;
    Active = true;

    // Ensure no animation assets can be reloaded/modified during async update
    Animations::SystemLocker.Begin(false);

    // Setup data for async update
    const auto& tickData = Time::Update;
    DeltaTime = tickData.DeltaTime.GetTotalSeconds();
    UnscaledDeltaTime = tickData.UnscaledDeltaTime.GetTotalSeconds();
    Time = tickData.Time.GetTotalSeconds();
    UnscaledTime = tickData.UnscaledTime.GetTotalSeconds();

#if USE_EDITOR
    // If debug flow is registered, then warm it up (eg. static cached method inside DebugFlow_ManagedWrapper) so it doesn't crash on highly multi-threaded code
    if (Animations::DebugFlow.IsBinded())
        Animations::DebugFlow(Animations::DebugFlowInfo());
#endif

    // Schedule work to update all animated models in async
    Function<void(int32)> job;
    job.Bind<AnimationsSystem, &AnimationsSystem::Job>(this);
    graph->DispatchJob(job, AnimationManagerInstance.UpdateList.Count());
}

void AnimationsSystem::PostExecute(TaskGraph* graph)
{
    if (!Active)
        return;
    PROFILE_CPU_NAMED("Animations.PostExecute");

    // Update gameplay
    for (int32 index = 0; index < AnimationManagerInstance.UpdateList.Count(); index++)
    {
        auto animatedModel = AnimationManagerInstance.UpdateList[index];
        if (CanUpdateModel(animatedModel))
        {
            animatedModel->GraphInstance.InvokeAnimEvents();
            animatedModel->OnAnimationUpdated_Sync();
        }
    }

    // Cleanup
    AnimationManagerInstance.UpdateList.Clear();
    Animations::SystemLocker.End(false);
    Active = false;
}

void Animations::AddToUpdate(AnimatedModel* obj)
{
    AnimationManagerInstance.UpdateList.Add(obj);
}

void Animations::RemoveFromUpdate(AnimatedModel* obj)
{
    AnimationManagerInstance.UpdateList.Remove(obj);
}
