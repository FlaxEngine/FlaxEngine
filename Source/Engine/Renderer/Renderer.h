// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

class GPUContext;
class GPUTexture;
class GPUTextureView;
struct RenderContext;
class RenderTask;
class SceneRenderTask;
class MaterialBase;
class Actor;

/// <summary>
/// High-level rendering service.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Renderer
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Renderer);
public:
    /// <summary>
    /// Determines whether the scene rendering system is ready (all shaders are loaded and helper resources are ready).
    /// </summary>
    static bool IsReady();

    /// <summary>
    /// Performs rendering for the input task.
    /// </summary>
    /// <param name="task">The scene rendering task.</param>
    API_FUNCTION() static void Render(SceneRenderTask* task);

    /// <summary>
    /// Draws scene objects depth (to the output Z buffer). The output must be depth texture to write hardware depth to it.
    /// </summary>
    /// <param name="context">The GPU commands context to use.</param>
    /// <param name="task">Render task to use it's view description and the render buffers.</param>
    /// <param name="output">The output texture. Must be valid and created.</param>
    /// <param name="customActors">The custom set of actors to render. If empty, the loaded scenes will be rendered.</param>
    API_FUNCTION() static void DrawSceneDepth(GPUContext* context, SceneRenderTask* task, GPUTexture* output, const Array<Actor*, HeapAllocation>& customActors);

    /// <summary>
    /// Draws postFx material to the render target.
    /// </summary>
    /// <param name="context">The GPU commands context to use.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="material">The material to render. It must be a post fx material.</param>
    /// <param name="output">The output texture. Must be valid and created.</param>
    /// <param name="input">The input texture. It's optional.</param>
    API_FUNCTION() static void DrawPostFxMaterial(GPUContext* context, API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, GPUTexture* output, GPUTextureView* input);

    /// <summary>
    /// Invoked drawing of the scene objects (collects draw calls into RenderList for a given RenderContext).
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="customActors">The custom set of actors to render. If empty, the loaded scenes will be rendered.</param>
    API_FUNCTION() static void DrawActors(API_PARAM(Ref) RenderContext& renderContext, API_PARAM(DefaultValue=null) const Array<Actor*, HeapAllocation>& customActors);
};
