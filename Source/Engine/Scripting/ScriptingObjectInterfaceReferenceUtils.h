// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Level/SceneObject.h"

/// <summary>
/// Utility methods for scene object interface references.
/// </summary>
/// <typeparam name="T">The type of the scripting interface.</typeparam>
template<typename T>
struct ScriptingObjectInterfaceReferenceHelper
{
    FORCE_INLINE static bool IsValidObject(const SceneObject* obj)
    {
        return !obj || obj->GetType().GetInterface(T::TypeInitializer) != nullptr;
    }

    FORCE_INLINE static SceneObject* GetSceneObject(T* interfaceObj)
    {
        return ScriptingObject::Cast<SceneObject>(ScriptingObject::FromInterface<T>(interfaceObj));
    }

    FORCE_INLINE static SceneObject* FindSceneObject(const Guid& id)
    {
        SceneObject* obj = static_cast<SceneObject*>(FindObject(id, SceneObject::GetStaticClass()));
        return IsValidObject(obj) ? obj : nullptr;
    }
};
