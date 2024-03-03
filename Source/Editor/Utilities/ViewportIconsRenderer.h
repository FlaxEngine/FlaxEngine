// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

struct RenderContext;
class Texture;
class SceneRenderTask;
class Actor;

/// <summary>
/// Editor viewports icons rendering service.
/// </summary>
API_CLASS(Static, Namespace="FlaxEditor") class FLAXENGINE_API ViewportIconsRenderer
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(ViewportIconsRenderer);

public:
    /// <summary>
    /// Draws the icons for the actors in the given scene (or actor tree).
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="actor">The actor (use scene for faster rendering).</param>
    API_FUNCTION() static void DrawIcons(API_PARAM(Ref) RenderContext& renderContext, Actor* actor);

    /// <summary>
    /// Adds icon to the custom actor.
    /// </summary>
    /// <param name="type">The actor type.</param>
    /// <param name="iconTexture">The icon texture to draw.</param>
    API_FUNCTION() static void AddCustomIcon(const ScriptingTypeHandle& type, Texture* iconTexture);

    /// <summary>
    /// Adds actor to the viewport icon rendering.
    /// </summary>
    /// <param name="actor">The actor to register for icon drawing.</param>
    API_FUNCTION() static void AddActor(Actor* actor);

    /// <summary>
    /// Adds actor to the viewport icon rendering.
    /// </summary>
    /// <param name="actor">The actor to register for icon drawing.</param>
    /// <param name="iconTexture">The icon texture to draw.</param>
    API_FUNCTION() static void AddActorWithTexture(Actor* actor, Texture* iconTexture);

    /// <summary>
    /// Removes actor from the viewport icon rendering.
    /// </summary>
    /// <param name="actor">The actor to unregister for icon drawing.</param>
    API_FUNCTION() static void RemoveActor(Actor* actor);
};
