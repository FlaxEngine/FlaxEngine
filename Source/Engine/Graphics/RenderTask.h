// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Scripting/ScriptingType.h"
#include "PostProcessBase.h"
#include "RenderView.h"

class GPUDevice;
class GPUContext;
class GPUTexture;
class GPUTextureView;
class GPUSwapChain;
class RenderBuffers;
struct RenderContext;
class Camera;
class Actor;

/// <summary>
/// Allows to perform custom rendering using graphics pipeline.
/// </summary>
API_CLASS() class FLAXENGINE_API RenderTask : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE(RenderTask);

    /// <summary>
    /// List with all registered tasks
    /// </summary>
    static Array<RenderTask*> Tasks;

    /// <summary>
    /// Static locker for render tasks list
    /// </summary>
    static CriticalSection TasksLocker;

    /// <summary>
    /// Amount of tasks rendered during last frame
    /// </summary>
    static int32 TasksDoneLastFrame;

    /// <summary>
    /// Draws all tasks. Called only during rendering by the graphics device.
    /// </summary>
    static void DrawAll();

private:

    RenderTask* _prevTask = nullptr;

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="RenderTask"/> class.
    /// </summary>
    virtual ~RenderTask();

public:

    /// <summary>
    /// Gets or sets a value indicating whether task is enabled.
    /// </summary>
    API_FIELD() bool Enabled = true;

    /// <summary>
    /// The order of the task. Used for tasks rendering order. Lower first, higher later.
    /// </summary>
    API_FIELD() int32 Order = 0;

    /// <summary>
    /// The amount of frames rendered by this task. It is auto incremented on task drawing.
    /// </summary>
    API_FIELD(ReadOnly) int32 FrameCount = 0;

    /// <summary>
    /// The output window swap chain. Optional, used only when rendering to native window backbuffer.
    /// </summary>
    GPUSwapChain* SwapChain = nullptr;

    /// <summary>
    /// The index of the frame when this task was last time rendered.
    /// </summary>
    uint64 LastUsedFrame = 0;

    /// <summary>
    /// Action fired on task rendering.
    /// </summary>
    API_EVENT() Delegate<RenderTask*, GPUContext*> Render;

    /// <summary>
    /// Action fired on task rendering begin.
    /// </summary>
    API_EVENT() Delegate<RenderTask*, GPUContext*> Begin;

    /// <summary>
    /// Action fired on task rendering end.
    /// </summary>
    API_EVENT() Delegate<RenderTask*, GPUContext*> End;

    /// <summary>
    /// Action fired just after frame present.
    /// </summary>
    API_EVENT() Delegate<RenderTask*> Present;

    /// <summary>
    /// Determines whether this task can be rendered.
    /// </summary>
    /// <returns><c>true</c> if this task can be rendered; otherwise, <c>false</c>.</returns>
    API_PROPERTY() virtual bool CanDraw() const;

    /// <summary>
    /// Called by graphics device to draw this task. Can be used to invoke task rendering nested inside another task - use on own risk!
    /// </summary>
    API_FUNCTION() virtual void OnDraw();

    /// <summary>
    /// Called on task rendering begin.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    virtual void OnBegin(GPUContext* context);

    /// <summary>
    /// Called on task rendering.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    virtual void OnRender(GPUContext* context);

    /// <summary>
    /// Called on task rendering end.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    virtual void OnEnd(GPUContext* context);

    /// <summary>
    /// Presents frame to the output.
    /// </summary>
    /// <param name="vsync">True if use vertical synchronization to lock frame rate.</param>
    virtual void OnPresent(bool vsync);

    /// <summary>
    /// Changes the buffers and output size. Does nothing if size won't change. Called by window or user to resize rendering buffers.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <returns>True if cannot resize the buffers.</returns>
    API_FUNCTION() virtual bool Resize(int32 width, int32 height);

public:

    bool operator<(const RenderTask& other) const
    {
        return Order < other.Order;
    }
};

/// <summary>
/// Defines actors to draw sources.
/// </summary>
API_ENUM(Attributes="Flags") enum class ActorsSources
{
    /// <summary>
    /// The actors won't be rendered.
    /// </summary>
    None = 0,

    /// <summary>
    /// The actors from the loaded scenes.
    /// </summary>
    Scenes = 1,

    /// <summary>
    /// The actors from the custom collection.
    /// </summary>
    CustomActors = 2,

    /// <summary>
    /// The actors from the loaded scenes and custom collection.
    /// </summary>
    ScenesAndCustomActors = Scenes | CustomActors,
};

DECLARE_ENUM_OPERATORS(ActorsSources);

/// <summary>
/// Wrapper for PostProcessBase that allows to render to customized managed postFx.
/// TODO: add support for managed inheritance of custom native types with proper spawn handling (like for ManagedScript)
/// </summary>
/// <seealso cref="PostProcessBase" />
class ManagedPostProcessEffect : public PostProcessBase
{
public:

    /// <summary>
    /// The script to use. Inherits from C# class 'PostProcessEffect'.
    /// </summary>
    Script* Target = nullptr;

public:

    /// <summary>
    /// Fetches the information about the PostFx location from the managed object.
    /// </summary>
    void FetchInfo();

public:

    // [PostProcessBase]
    bool IsLoaded() const override;
    void Render(RenderContext& renderContext, GPUTexture* input, GPUTexture* output) override;
};

/// <summary>
/// Render task which draws scene actors into the output buffer.
/// </summary>
/// <seealso cref="FlaxEngine.RenderTask" />
API_CLASS() class FLAXENGINE_API SceneRenderTask : public RenderTask
{
DECLARE_SCRIPTING_TYPE(SceneRenderTask);

    /// <summary>
    /// Finalizes an instance of the <see cref="SceneRenderTask"/> class.
    /// </summary>
    ~SceneRenderTask();

public:

    /// <summary>
    /// True if the current frame is after the camera cut. Used to clear the temporal effects history and prevent visual artifacts blended from the previous frames.
    /// </summary>
    bool IsCameraCut = true;

    /// <summary>
    /// Marks the next rendered frame as camera cut. Used to clear the temporal effects history and prevent visual artifacts blended from the previous frames.
    /// </summary>
    API_FUNCTION() void CameraCut();

    /// <summary>
    /// The output texture (can be null if using rendering to window swap chain). Can be sued to redirect the default scene rendering output to a texture.
    /// </summary>
    API_FIELD() GPUTexture* Output = nullptr;

    /// <summary>
    /// The scene rendering buffers. Created and managed by the task.
    /// </summary>
    API_FIELD(ReadOnly) RenderBuffers* Buffers = nullptr;

    /// <summary>
    /// The scene rendering camera. Can be used to override the rendering view properties based on the current camera setup.
    /// </summary>
    API_FIELD() Camera* Camera = nullptr;

    /// <summary>
    /// The render view description.
    /// </summary>
    API_FIELD() RenderView View;

    /// <summary>
    /// The actors source to use (configures what objects to render).
    /// </summary>
    API_FIELD() ActorsSources ActorsSource = ActorsSources::Scenes;

    /// <summary>
    /// The custom set of actors to render.
    /// </summary>
    Array<Actor*> CustomActors;

    /// <summary>
    /// Adds the custom actor to the rendering.
    /// </summary>
    /// <param name="actor">The actor.</param>
    API_FUNCTION() void AddCustomActor(Actor* actor);

    /// <summary>
    /// Removes the custom actor from the rendering.
    /// </summary>
    /// <param name="actor">The actor.</param>
    API_FUNCTION() void RemoveCustomActor(Actor* actor);

    /// <summary>
    /// Removes all the custom actors from the rendering.
    /// </summary>
    API_FUNCTION() void ClearCustomActors();

    /// <summary>
    /// The custom post fx to render (managed).
    /// </summary>
    Array<ManagedPostProcessEffect> CustomPostFx;

public:

    /// <summary>
    /// The action called on view rendering to collect draw calls. It allows to extend rendering pipeline and draw custom geometry non-existing in the scene or custom actors set.
    /// </summary>
    API_EVENT() Delegate<RenderContext&> CollectDrawCalls;

    /// <summary>
    /// Calls collecting postFx volumes for rendering.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    virtual void CollectPostFxVolumes(RenderContext& renderContext);

    /// <summary>
    /// Calls drawing scene objects.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    virtual void OnCollectDrawCalls(RenderContext& renderContext);

    /// <summary>
    /// The action called after scene rendering. Can be used to perform custom pre-rendering or to modify the render view.
    /// </summary>
    API_EVENT() Delegate<GPUContext*, RenderContext&> PreRender;

    /// <summary>
    /// Called before scene rendering. Can be used to perform custom pre-rendering or to modify the render view.
    /// </summary>
    /// <param name="context">The GPU commands context.</param>
    /// <param name="renderContext">The rendering context.</param>
    virtual void OnPreRender(GPUContext* context, RenderContext& renderContext);

    /// <summary>
    /// The action called after scene rendering. Can be used to render additional visual elements to the output.
    /// </summary>
    API_EVENT() Delegate<GPUContext*, RenderContext&> PostRender;

    /// <summary>
    /// Called after scene rendering. Can be used to render additional visual elements to the output.
    /// </summary>
    /// <param name="context">The GPU commands context.</param>
    /// <param name="renderContext">The rendering context.</param>
    virtual void OnPostRender(GPUContext* context, RenderContext& renderContext);

public:

    /// <summary>
    /// Gets the rendering render task viewport.
    /// </summary>
    API_PROPERTY() Viewport GetViewport() const;

    /// <summary>
    /// Gets the rendering output view.
    /// </summary>
    API_PROPERTY() GPUTextureView* GetOutputView() const;

public:

    // [RenderTask]
    bool Resize(int32 width, int32 height) override;
    bool CanDraw() const override;
    void OnBegin(GPUContext* context) override;
    void OnRender(GPUContext* context) override;
    void OnEnd(GPUContext* context) override;
};

/// <summary>
/// The main game rendering task used by the engine.
/// </summary>
/// <remarks>
/// For Main Render Task its <see cref="SceneRenderTask.Output"/> may be null because game can be rendered directly to the native window backbuffer.
/// This allows to increase game rendering performance (reduced memory usage and data transfer).
/// User should use post effects pipeline to modify the final frame.
/// </remarks>
/// <seealso cref="FlaxEngine.SceneRenderTask" />
API_CLASS() class FLAXENGINE_API MainRenderTask : public SceneRenderTask
{
DECLARE_SCRIPTING_TYPE(MainRenderTask);

    /// <summary>
    /// Finalizes an instance of the <see cref="MainRenderTask"/> class.
    /// </summary>
    ~MainRenderTask();

public:

    /// <summary>
    /// Gets the main game rendering task. Use it to plug custom rendering logic for your game.
    /// </summary>
    API_FIELD(ReadOnly) static MainRenderTask* Instance;

public:

    // [SceneRenderTask]
    void OnBegin(GPUContext* context) override;
};

/// <summary>
/// The high-level renderer context. Used to collect the draw calls for the scene rendering. Can be used to perform a custom rendering.
/// </summary>
API_STRUCT() struct RenderContext
{
DECLARE_SCRIPTING_TYPE_MINIMAL(RenderContext);

    /// <summary>
    /// The render buffers.
    /// </summary>
    API_FIELD() RenderBuffers* Buffers = nullptr;

    /// <summary>
    /// The render list.
    /// </summary>
    API_FIELD() RenderList* List = nullptr;

    /// <summary>
    /// The render view.
    /// </summary>
    API_FIELD() RenderView View;

    /// <summary>
    /// The proxy render view used to synchronize objects level of detail during rendering (eg. during shadow maps rendering passes). It's optional.
    /// </summary>
    API_FIELD() RenderView* LodProxyView = nullptr;

    /// <summary>
    /// The scene rendering task that is a source of renderable objects (optional).
    /// </summary>
    API_FIELD() SceneRenderTask* Task = nullptr;

    RenderContext()
    {
    }

    RenderContext(SceneRenderTask* task);
};
