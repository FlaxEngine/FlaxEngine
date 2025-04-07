// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/BoundingFrustum.h"
#include "Engine/Level/Actor.h"
#include "Engine/Platform/CriticalSection.h"

class SceneRenderTask;
class SceneRendering;
struct PostProcessSettings;
struct RenderContext;
struct RenderContextBatch;
struct RenderView;

/// <summary>
/// Interface for actors that can override the default rendering settings (eg. PostFxVolume actor).
/// </summary>
class FLAXENGINE_API IPostFxSettingsProvider
{
public:
    /// <summary>
    /// Collects the settings for rendering of the specified task.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    virtual void Collect(RenderContext& renderContext) = 0;

    /// <summary>
    /// Blends the object settings to the given settings using given weight.
    /// </summary>
    /// <param name="other">The other settings to blend to.</param>
    /// <param name="weight">The blending weight (normalized to 0-1 range).</param>
    virtual void Blend(PostProcessSettings& other, float weight) = 0;
};

/// <summary>
/// Interface for objects to plug into Scene Rendering and listen for its evens such as static actors changes which are relevant for drawing cache.
/// </summary>
/// <seealso cref="SceneRendering"/>
class FLAXENGINE_API ISceneRenderingListener
{
    friend SceneRendering;
private:
    Array<SceneRendering*, InlinedAllocation<8>> _scenes;
public:
    ~ISceneRenderingListener();

    // Actor properties that were modified.
    enum UpdateFlags
    {
        Visual = 1,
        Bounds = 2,
        Layer = 4,
        StaticFlags = 8,
        Auto = Visual | Bounds | Layer,
    };

    // Starts listening to the scene rendering events.
    void ListenSceneRendering(SceneRendering* scene);

    // Events called by Scene Rendering
    virtual void OnSceneRenderingAddActor(Actor* a) = 0;
    virtual void OnSceneRenderingUpdateActor(Actor* a, const BoundingSphere& prevBounds, UpdateFlags flags = Auto) = 0;
    virtual void OnSceneRenderingRemoveActor(Actor* a) = 0;
    virtual void OnSceneRenderingClear(SceneRendering* scene) = 0;
};

/// <summary>
/// Scene rendering helper subsystem that boosts the level rendering by providing efficient objects cache and culling implementation.
/// </summary>
class FLAXENGINE_API SceneRendering
{
#if USE_EDITOR
    typedef Function<void(RenderView&)> PhysicsDebugCallback;
    typedef Function<void(RenderView&)> LightsDebugCallback;
    friend class ViewportIconsRendererService;
#endif
public:
    struct DrawActor
    {
        Actor* Actor;
        uint32 LayerMask;
        int8 NoCulling : 1;
        BoundingSphere Bounds;
    };

    /// <summary>
    /// Drawing categories for separate draw stages.
    /// </summary>
    enum DrawCategory
    {
        SceneDraw = 0,
        SceneDrawAsync,
        PreRender,
        PostRender,
        MAX
    };

    Array<DrawActor> Actors[MAX];
    Array<IPostFxSettingsProvider*> PostFxProviders;
    CriticalSection Locker;

private:
#if USE_EDITOR
    Array<PhysicsDebugCallback> PhysicsDebug;
    Array<LightsDebugCallback> LightsDebug;
    Array<Actor*> ViewportIcons;
#endif

    // Listener - some rendering systems cache state of the scene (eg. in RenderBuffers::CustomBuffer), this extensions allows those systems to invalidate cache and handle scene changes
    friend ISceneRenderingListener;
    Array<ISceneRenderingListener*, InlinedAllocation<8>> _listeners;

public:
    /// <summary>
    /// Draws the scene. Performs the optimized actors culling and draw calls submission for the current render pass (defined by the render view).
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="category">The actors category to draw.</param>
    void Draw(RenderContextBatch& renderContextBatch, DrawCategory category = SceneDraw);

    /// <summary>
    /// Collects the post fx volumes for the given rendering view.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void CollectPostFxVolumes(RenderContext& renderContext);

    /// <summary>
    /// Clears this instance data.
    /// </summary>
    void Clear();

public:
    void AddActor(Actor* a, int32& key);
    void UpdateActor(Actor* a, int32& key, ISceneRenderingListener::UpdateFlags flags = ISceneRenderingListener::Auto);
    void RemoveActor(Actor* a, int32& key);

    FORCE_INLINE void AddPostFxProvider(IPostFxSettingsProvider* obj)
    {
        PostFxProviders.Add(obj);
    }

    FORCE_INLINE void RemovePostFxProvider(IPostFxSettingsProvider* obj)
    {
        PostFxProviders.Remove(obj);
    }

#if USE_EDITOR
    template<class T, void(T::*Method)(RenderView&)>
    FORCE_INLINE void AddPhysicsDebug(T* obj)
    {
        PhysicsDebugCallback f;
        f.Bind<T, Method>(obj);
        PhysicsDebug.Add(f);
    }

    template<class T, void(T::*Method)(RenderView&)>
    void RemovePhysicsDebug(T* obj)
    {
        PhysicsDebugCallback f;
        f.Bind<T, Method>(obj);
        PhysicsDebug.Remove(f);
    }

    template<class T, void(T::*Method)(RenderView&)>
    FORCE_INLINE void AddLightsDebug(T* obj)
    {
        LightsDebugCallback f;
        f.Bind<T, Method>(obj);
        LightsDebug.Add(f);
    }

    template<class T, void(T::*Method)(RenderView&)>
    void RemoveLightsDebug(T* obj)
    {
        LightsDebugCallback f;
        f.Bind<T, Method>(obj);
        LightsDebug.Remove(f);
    }

    FORCE_INLINE void AddViewportIcon(Actor* obj)
    {
        ViewportIcons.Add(obj);
    }

    FORCE_INLINE void RemoveViewportIcon(Actor* obj)
    {
        ViewportIcons.Remove(obj);
    }
#endif

private:
    Array<BoundingFrustum> _drawFrustumsData;
    DrawActor* _drawListData;
    int64 _drawListSize;
    volatile int64 _drawListIndex;
    RenderContextBatch* _drawBatch;

    void DrawActorsJob(int32);
};
