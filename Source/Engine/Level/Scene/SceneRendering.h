// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/Types.h"

class SceneRenderTask;
struct PostProcessSettings;
struct RenderContext;
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
/// Scene rendering helper subsystem that boosts the level rendering by providing efficient objects cache and culling implementation.
/// </summary>
class FLAXENGINE_API SceneRendering
{
    friend Scene;
#if USE_EDITOR
    typedef Function<void(RenderView&)> PhysicsDebugCallback;
    friend class ViewportIconsRendererService;
#endif
    struct DrawEntry
    {
        Actor* Actor;
        uint32 LayerMask;
        BoundingSphere Bounds;
    };

    struct DrawEntries
    {
        Array<DrawEntry> List;

        int32 Add(Actor* obj);
        void Update(Actor* obj, int32 key);
        void Remove(Actor* obj, int32 key);
        void Clear();
        void CullAndDraw(RenderContext& renderContext);
        void CullAndDrawOffline(RenderContext& renderContext);
    };

private:

    Scene* Scene;
    DrawEntries Geometry;
    DrawEntries Common;
    Array<Actor*> CommonNoCulling;
    Array<IPostFxSettingsProvider*> PostFxProviders;
#if USE_EDITOR
    Array<PhysicsDebugCallback> PhysicsDebug;
    Array<Actor*> ViewportIcons;
#endif

    explicit SceneRendering(::Scene* scene);

public:

    /// <summary>
    /// Draws the scene. Performs the optimized actors culling and draw calls submission for the current render pass (defined by the render view).
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Draw(RenderContext& renderContext);

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

    FORCE_INLINE int32 AddGeometry(Actor* obj)
    {
        return Geometry.Add(obj);
    }

    FORCE_INLINE void UpdateGeometry(Actor* obj, int32 key)
    {
        Geometry.Update(obj, key);
    }

    FORCE_INLINE void RemoveGeometry(Actor* obj, int32& key)
    {
        Geometry.Remove(obj, key);
        key = -1;
    }

    FORCE_INLINE int32 AddCommon(Actor* obj)
    {
        return Common.Add(obj);
    }

    FORCE_INLINE void UpdateCommon(Actor* obj, int32 key)
    {
        Common.Update(obj, key);
    }

    FORCE_INLINE void RemoveCommon(Actor* obj, int32& key)
    {
        Common.Remove(obj, key);
        key = -1;
    }

    FORCE_INLINE void AddCommonNoCulling(Actor* obj)
    {
        CommonNoCulling.Add(obj);
    }

    FORCE_INLINE void RemoveCommonNoCulling(Actor* obj)
    {
        CommonNoCulling.Remove(obj);
    }

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

    FORCE_INLINE void AddViewportIcon(Actor* obj)
    {
        ViewportIcons.Add(obj);
    }

    FORCE_INLINE void RemoveViewportIcon(Actor* obj)
    {
        ViewportIcons.Remove(obj);
    }

#endif
};
