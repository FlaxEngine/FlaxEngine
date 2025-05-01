// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

// Enables locking scenes during scene query execution can provide some safety when using scene queries from other threads but may provide stalls on a main thread
#define SCENE_QUERIES_WITH_LOCK 1

#include "Level.h"
#include "Scene/Scene.h"
#if SCENE_QUERIES_WITH_LOCK
#include "Engine/Threading/Threading.h"
#endif

/// <summary>
/// Helper class to perform scene actions and queries
/// </summary>
class FLAXENGINE_API SceneQuery
{
public:
    /// <summary>
    /// Try to find actor hit by the given ray.
    /// </summary>
    /// <param name="ray">Ray to test</param>
    /// <returns>Hit actor or nothing</returns>
    static Actor* RaycastScene(const Ray& ray);

public:
    /// <summary>
    /// Gets all scene objects from the actor into linear list. Appends them (without the given actor).
    /// </summary>
    /// <param name="root">The root actor.</param>
    /// <param name="objects">The objects output.</param>
    static void GetAllSceneObjects(Actor* root, Array<SceneObject*>& objects);

    /// <summary>
    /// Gets all scene objects (for serialization) from the actor into linear list. Appends them (including the given actor).
    /// </summary>
    /// <param name="root">The root actor.</param>
    /// <param name="objects">The objects output.</param>
    static void GetAllSerializableSceneObjects(Actor* root, Array<SceneObject*>& objects);

    /// <summary>
    /// Gets all actors from the actor into linear list. Appends them (without the given actor).
    /// </summary>
    /// <param name="root">The root actor.</param>
    /// <param name="actors">The actors output.</param>
    static void GetAllActors(Actor* root, Array<Actor*>& actors);

    /// <summary>
    /// Gets all actors from the loaded scenes into linear list (without scene actors).
    /// </summary>
    /// <param name="actors">The actors output.</param>
    static void GetAllActors(Array<Actor*>& actors);

public:
    /// <summary>
    /// Execute custom action on actors tree.
    /// </summary>
    /// <param name="action">Actor to call on every actor in the tree. Returns true if keep calling deeper.</param>
    /// <param name="args">Custom arguments for the function</param>
    template<typename... Params>
    static void TreeExecute(Function<bool(Actor*, Params...)>& action, Params... args)
    {
#if SCENE_QUERIES_WITH_LOCK
        ScopeLock lock(Level::ScenesLock);
#endif
        for (int32 i = 0; i < Level::Scenes.Count(); i++)
            Level::Scenes.Get()[i]->TreeExecute<Params...>(action, args...);
    }
};
