// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Scene/Scene.h"

/// <summary>
/// Scene for editor previews with support of object drawing and updating in separation of global scenes collection. It mocks the gameplay to preview scene objects.
/// </summary>
API_CLASS() class EditorScene final : public Scene
{
DECLARE_SCENE_OBJECT(EditorScene);
public:

    /// <summary>
    /// Updates the gameplay.
    /// </summary>
    API_FUNCTION() void Update();
};
