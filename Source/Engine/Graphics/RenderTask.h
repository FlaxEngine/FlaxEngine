// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Renderer/RendererAllocation.h"
#include "RenderView.h"

class GPUDevice;
class GPUContext;
class GPUTexture;
class GPUTextureView;
class GPUSwapChain;
class RenderBuffers;
class PostProcessEffect;
struct RenderContext;
class Camera;
class Actor;
class Scene;

/// <summary>
/// Allows to perform custom rendering using graphics pipeline.
/// </summary>
API_CLASS() class FLAXENGINE_API RenderTask : public ScriptingObject
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
    API_FIELD(ReadOnly) uint64 LastUsedFrame = 0;

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
    /// Called by graphics device to draw this task.
    /// </summary>
    API_FUNCTION() virtual void OnDraw();

    /// <summary>
    /// Called by graphics device to idle task that has not been selected for drawing this frame (CanDraw returned false). Can be used to recycle cached memory if task is idle for many frames in a row.
    /// </summary>
    virtual void OnIdle();

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
    /// The scenes from the custom collection.
    /// </summary>
    CustomScenes = 4,

    /// <summary>
    /// The actors from the loaded scenes and custom collection.
    /// </summary>
    ScenesAndCustomActors = Scenes | CustomActors,
};

DECLARE_ENUM_OPERATORS(ActorsSources);

/// <summary>
/// The Post Process effect rendering location within the rendering pipeline.
/// </summary>
API_ENUM() enum class RenderingUpscaleLocation
{
    /// <summary>
    /// The up-scaling happens directly to the output buffer (backbuffer) after post processing and anti-aliasing.
    /// </summary>
    AfterAntiAliasingPass = 0,
    
    /// <summary>
    /// The up-scaling happens before the post processing after scene rendering (after geometry, lighting, volumetrics, transparency and SSR/SSAO).
    /// </summary>
    BeforePostProcessingPass = 1,
};

/// <summary>
/// Render task which draws scene actors into the output buffer.
/// </summary>
/// <seealso cref="FlaxEngine.RenderTask" />
API_CLASS() class FLAXENGINE_API SceneRenderTask : public RenderTask
{
    DECLARE_SCRIPTING_TYPE(SceneRenderTask);
protected:
    class SceneRendering* _customActorsScene = nullptr;

public:
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
    /// True if the task is used for custom scene rendering and default scene drawing into output should be skipped. Enable it if you use Render event and draw scene manually.
    /// </summary>
    API_FIELD() bool IsCustomRendering = false;

    /// <summary>
    /// Marks the next rendered frame as camera cut. Used to clear the temporal effects history and prevent visual artifacts blended from the previous frames.
    /// </summary>
    API_FUNCTION() void CameraCut();

    /// <summary>
    /// The output texture (can be null if using rendering to window swap chain). Can be used to redirect the default scene rendering output to a texture.
    /// </summary>
    API_FIELD() GPUTexture* Output = nullptr;

    /// <summary>
    /// The scene rendering buffers. Created and managed by the task.
    /// </summary>
    API_FIELD(ReadOnly) RenderBuffers* Buffers = nullptr;

    /// <summary>
    /// The scene rendering camera. Can be used to override the rendering view properties based on the current camera setup.
    /// </summary>
    API_FIELD() ScriptingObjectReference<Camera> Camera;

    /// <summary>
    /// The render view description.
    /// </summary>
    API_FIELD() RenderView View;

    /// <summary>
    /// The actors source to use (configures what objects to render).
    /// </summary>
    API_FIELD() ActorsSources ActorsSource = ActorsSources::Scenes;

    /// <summary>
    /// The scale of the rendering resolution relative to the output dimensions. If lower than 1 the scene and postprocessing will be rendered at a lower resolution and upscaled to the output backbuffer.
    /// </summary>
    API_FIELD() float RenderingPercentage = 1.0f;

    /// <summary>
    /// The image resolution upscale location within rendering pipeline. Unused if RenderingPercentage is 1.
    /// </summary>
    API_FIELD() RenderingUpscaleLocation UpscaleLocation = RenderingUpscaleLocation::AfterAntiAliasingPass;

public:
    /// <summary>
    /// The custom set of actors to render. Used when ActorsSources::CustomActors flag is active.
    /// </summary>
    API_FIELD() Array<Actor*> CustomActors;

    /// <summary>
    /// The custom set of scenes to render. Used when ActorsSources::CustomScenes flag is active.
    /// </summary>
    API_FIELD() Array<Scene*> CustomScenes;

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

public:
    /// <summary>
    /// The custom set of postfx to render.
    /// </summary>
    Array<PostProcessEffect*> CustomPostFx;

    /// <summary>
    /// Adds the custom postfx to the rendering.
    /// </summary>
    /// <param name="fx">The postfx script.</param>
    API_FUNCTION() void AddCustomPostFx(PostProcessEffect* fx);

    /// <summary>
    /// Removes the custom postfx from the rendering.
    /// </summary>
    /// <param name="fx">The postfx script.</param>
    API_FUNCTION() void RemoveCustomPostFx(PostProcessEffect* fx);

    /// <summary>
    /// True if allow using global custom PostFx when rendering this task.
    /// </summary>
    API_FIELD() bool AllowGlobalCustomPostFx = true;

    /// <summary>
    /// The custom set of global postfx to render for all <see cref="SceneRenderTask"/> (applied to tasks that have <see cref="AllowGlobalCustomPostFx"/> turned on).
    /// </summary>
    static Array<PostProcessEffect*> GlobalCustomPostFx;

    /// <summary>
    /// Adds the custom global postfx to the rendering.
    /// </summary>
    /// <param name="fx">The postfx script.</param>
    API_FUNCTION() static void AddGlobalCustomPostFx(PostProcessEffect* fx);

    /// <summary>
    /// Removes the custom global postfx from the rendering.
    /// </summary>
    /// <param name="fx">The postfx script.</param>
    API_FUNCTION() static void RemoveGlobalCustomPostFx(PostProcessEffect* fx);

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
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="category">The actors category to draw (see SceneRendering::DrawCategory).</param>
    virtual void OnCollectDrawCalls(RenderContextBatch& renderContextBatch, byte category = 0);

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

    /// <summary>
    /// The action called before any rendering to override/customize setup RenderSetup inside RenderList. Can be used to enable eg. Motion Vectors rendering.
    /// </summary>
    Delegate<RenderContext&> SetupRender;

public:
    /// <summary>
    /// Gets the rendering render task viewport (before upsampling).
    /// </summary>
    API_PROPERTY() Viewport GetViewport() const;

    /// <summary>
    /// Gets the rendering output viewport (after upsampling).
    /// </summary>
    API_PROPERTY() Viewport GetOutputViewport() const;

    /// <summary>
    /// Gets the rendering output view.
    /// </summary>
    API_PROPERTY() GPUTextureView* GetOutputView() const;

public:
    // [RenderTask]
    bool Resize(int32 width, int32 height) override;
    bool CanDraw() const override;
    void OnIdle() override;
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
API_STRUCT(NoDefault) struct RenderContext
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
API_STRUCT(NoDefault) struct RenderContextBatch
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
    API_FIELD() Array<uint64, InlinedAllocation<8>> WaitLabels;

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
};
