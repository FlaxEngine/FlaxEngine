// Copyright (c) Wojciech Figat. All rights reserved.

#include "EditorScene.h"

EditorScene::EditorScene(const SpawnParams& params)
    : Scene(params)
{
    // Mock editor preview scene to be in gameplay
    InitializeHierarchy();
    SceneBeginData beginData;
    EditorScene::BeginPlay(&beginData);
    beginData.OnDone();

    // Mark as internal to prevent collection in ManagedEditor::WipeOutLeftoverSceneObjects
    Tags.Add(Tags::Get(TEXT("__EditorInternal")));
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
