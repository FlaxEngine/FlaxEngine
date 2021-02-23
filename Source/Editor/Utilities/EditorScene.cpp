// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "EditorScene.h"

#include "Engine/Debug/DebugDraw.h"

EditorScene::EditorScene(const SpawnParams& params)
    : Scene(params)
{
    // Mock editor preview scene to be in gameplay
    EditorScene::PostSpawn();
    SceneBeginData beginData;
    EditorScene::BeginPlay(&beginData);
    beginData.OnDone();
}

void EditorScene::Update()
{
    for (auto& e : Ticking.Update.Ticks)
        e.Call();
    for (auto& e : Ticking.LateUpdate.Ticks)
        e.Call();
    for (auto& e : Ticking.FixedUpdate.Ticks)
        e.Call();
}
