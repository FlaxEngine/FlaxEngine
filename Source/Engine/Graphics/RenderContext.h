// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Renderer/RendererAllocation.h"
#include "RenderView.h"

class RenderBuffers;
class RenderList;
class SceneRenderTask;

/// <summary>
/// The high-level renderer context. Used to collect the draw calls for the scene rendering. Can be used to perform a custom rendering.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API RenderContext
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(RenderContext);

    /// <summary>
    /// The render buffers that contain drawing state (eg. LOD transitions) and scene buffers (eg. GBuffer, DDGI, Shadow Maps).
    /// </summary>
    API_FIELD() RenderBuffers* Buffers = nullptr;

    /// <summary>
    /// The render list that collects draw calls.
    /// </summary>
    API_FIELD() RenderList* List = nullptr;

    /// <summary>
    /// The scene rendering task that is a source of renderable objects (optional).
    /// </summary>
    API_FIELD() SceneRenderTask* Task = nullptr;

    /// <summary>
    /// The proxy render view used to synchronize objects level of detail during rendering (eg. during shadow maps rendering passes). It's optional.
    /// </summary>
    API_FIELD() RenderView* LodProxyView = nullptr;

    /// <summary>
    /// The render view.
    /// </summary>
    API_FIELD() RenderView View;

    /// <summary>
    /// The GPU access locking critical section to protect data access when performing multi-threaded rendering.
    /// </summary>
    static CriticalSection GPULocker;

    RenderContext() = default;
    RenderContext(SceneRenderTask* task) noexcept;
};

/// <summary>
/// The high-level renderer context batch that encapsulates multiple rendering requests within a single task (eg. optimize main view scene rendering and shadow projections at once).
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API RenderContextBatch
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(RenderContextBatch);

    /// <summary>
    /// The render buffers.
    /// </summary>
    API_FIELD() RenderBuffers* Buffers = nullptr;

    /// <summary>
    /// The scene rendering task that is a source of renderable objects (optional).
    /// </summary>
    API_FIELD() SceneRenderTask* Task = nullptr;

    /// <summary>
    /// The all render views collection for the current rendering (main view, shadow projections, etc.).
    /// </summary>
    API_FIELD() Array<RenderContext, RendererAllocation> Contexts;

    /// <summary>
    /// The Job System labels to wait on, after draw calls collecting.
    /// </summary>
    API_FIELD() Array<int64, InlinedAllocation<8>> WaitLabels;

    /// <summary>
    /// Enables using async tasks via Job System when performing drawing.
    /// </summary>
    API_FIELD() bool EnableAsync = true;

    RenderContextBatch() = default;
    RenderContextBatch(SceneRenderTask* task);
    RenderContextBatch(const RenderContext& context);

    FORCE_INLINE RenderContext& GetMainContext()
    {
        return Contexts.Get()[0];
    }

    FORCE_INLINE const RenderContext& GetMainContext() const
    {
        return Contexts.Get()[0];
    }

    /// <summary>
    /// Waits for all scheduled async jobs to complete and clears WaitLabels.
    /// </summary>
    void FlushWaitLabels();
};
