// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "EditorScene.h"

EditorScene::EditorScene(const SpawnParams& params)
    : Scene(params)
{
    // Mock editor preview scene to be in gameplay
    InitializeHierarchy();
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
    for (auto& e : Ticking.LateFixedUpdate.Ticks)
        e.Call();
}
