// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

struct RenderContext;
class SceneRenderTask;
class Scene;

/// <summary>
/// Editor viewports icons rendering service.
/// </summary>
API_CLASS(Static, Namespace="FlaxEditor") class ViewportIconsRenderer
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ViewportIconsRenderer);
public:

    /// <summary>
    /// Draws the icons for the actors in the given scene.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="scene">The scene.</param>
    API_FUNCTION() static void DrawIcons(API_PARAM(Ref) RenderContext& renderContext, Scene* scene);
};
