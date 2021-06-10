// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Animations.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Actors/AnimatedModel.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"

Array<AnimatedModel*> UpdateList;

class AnimationsService : public EngineService
{
public:

    AnimationsService()
        : EngineService(TEXT("Animations"), -10)
    {
    }

    void Update() override;
    void Dispose() override;
};

AnimationsService AnimationManagerInstance;
Delegate<Asset*, ScriptingObject*, uint32, uint32> Animations::DebugFlow;

void AnimationsService::Update()
{
    PROFILE_CPU_NAMED("Animations");

    // TODO: implement the thread jobs pipeline to run set of tasks at once (use it for multi-threaded rendering and animations evaluation)

    const auto& tickData = Time::Update;
    const float deltaTime = tickData.DeltaTime.GetTotalSeconds();
    const float unscaledDeltaTime = tickData.UnscaledDeltaTime.GetTotalSeconds();
    const float time = tickData.Time.GetTotalSeconds();
    const float unscaledTime = tickData.UnscaledTime.GetTotalSeconds();

    for (int32 i = 0; i < UpdateList.Count(); i++)
    {
        auto animatedModel = UpdateList[i];
        if (animatedModel->SkinnedModel == nullptr || !animatedModel->SkinnedModel->IsLoaded())
            continue;

        // Prepare skinning data
        animatedModel->SetupSkinningData();

        // Update the animation graph and the skinning
        auto graph = animatedModel->AnimationGraph.Get();
        if (graph && graph->IsLoaded() && graph->Graph.CanUseWithSkeleton(animatedModel->SkinnedModel)
#if USE_EDITOR
            && graph->Graph.Parameters.Count() == animatedModel->GraphInstance.Parameters.Count() // It may happen in editor so just add safe check to prevent any crashes
#endif
        )
        {
#if USE_EDITOR
            // Lock in editor only (more reloads during asset live editing)
            ScopeLock lock(animatedModel->AnimationGraph->Locker);
#endif

            // Animation delta time can be based on a time since last update or the current delta
            float dt = animatedModel->UseTimeScale ? deltaTime : unscaledDeltaTime;
            float t = animatedModel->UseTimeScale ? time : unscaledTime;
            const float lastUpdateTime = animatedModel->GraphInstance.LastUpdateTime;
            if (lastUpdateTime > 0 && t > lastUpdateTime)
            {
                dt = t - lastUpdateTime;
            }
            animatedModel->GraphInstance.LastUpdateTime = t;

            // Evaluate animated nodes pose
            graph->GraphExecutor.Update(animatedModel->GraphInstance, dt);

            // Update gameplay
            animatedModel->OnAnimationUpdated();
        }
    }
    UpdateList.Clear();
}

void AnimationsService::Dispose()
{
    UpdateList.Resize(0);
}

void Animations::AddToUpdate(AnimatedModel* obj)
{
    UpdateList.Add(obj);
}

void Animations::RemoveFromUpdate(AnimatedModel* obj)
{
    UpdateList.Remove(obj);
}
