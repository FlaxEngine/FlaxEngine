// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "SceneQuery.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Profiler/Profiler.h"

Actor* SceneQuery::RaycastScene(const Ray& ray)
{
    PROFILE_CPU();
#if SCENE_QUERIES_WITH_LOCK
    ScopeLock lock(Level::ScenesLock);
#endif
    Actor* target;
    Actor* minTarget = nullptr;
    Real distance;
    Vector3 normal;
    Real minDistance = MAX_Real;
    for (int32 i = 0; i < Level::Scenes.Count(); i++)
    {
        target = Level::Scenes[i]->Intersects(ray, distance, normal);
        if (target)
        {
            if (minDistance > distance)
            {
                minDistance = distance;
                minTarget = target;
            }
        }
    }
    return minTarget;
}

bool GetAllSceneObjectsQuery(Actor* actor, Array<SceneObject*>& objects)
{
    objects.Add(actor);
    objects.Add(reinterpret_cast<SceneObject* const*>(actor->Scripts.Get()), actor->Scripts.Count());
    return true;
}

void SceneQuery::GetAllSceneObjects(Actor* root, Array<SceneObject*>& objects)
{
    ASSERT(root);
    PROFILE_CPU();
    Function<bool(Actor*, Array<SceneObject*>&)> func(GetAllSceneObjectsQuery);
    root->TreeExecuteChildren<Array<SceneObject*>&>(func, objects);
}

bool GetAllSerializableSceneObjectsQuery(Actor* actor, Array<SceneObject*>& objects)
{
    if (EnumHasAnyFlags(actor->HideFlags, HideFlags::DontSave))
        return false;
    objects.Add(actor);
    objects.Add(reinterpret_cast<SceneObject* const*>(actor->Scripts.Get()), actor->Scripts.Count());
    return true;
}

void SceneQuery::GetAllSerializableSceneObjects(Actor* root, Array<SceneObject*>& objects)
{
    ASSERT(root);
    PROFILE_CPU();
    Function<bool(Actor*, Array<SceneObject*>&)> func(GetAllSerializableSceneObjectsQuery);
    root->TreeExecute<Array<SceneObject*>&>(func, objects);
}

bool GetAllActorsQuery(Actor* actor, Array<Actor*>& actors)
{
    actors.Add(actor);
    return true;
}

void SceneQuery::GetAllActors(Actor* root, Array<Actor*>& actors)
{
    PROFILE_CPU();
    ASSERT(root);
    Function<bool(Actor*, Array<Actor*>&)> func(GetAllActorsQuery);
    root->TreeExecuteChildren<Array<Actor*>&>(func, actors);
}

void SceneQuery::GetAllActors(Array<Actor*>& actors)
{
    PROFILE_CPU();
#if SCENE_QUERIES_WITH_LOCK
    ScopeLock lock(Level::ScenesLock);
#endif
    for (int32 i = 0; i < Level::Scenes.Count(); i++)
        GetAllActors(Level::Scenes[i], actors);
}
