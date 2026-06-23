// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObjectInterfaceReferenceUtils.h"

/// <summary>
/// The scene object interface reference.
/// </summary>
/// <typeparam name="T">The type of the scripting interface.</typeparam>
template<typename T>
API_CLASS(InBuild) class ScriptingObjectInterfaceReference : public ScriptingObjectReferenceBase
{
    typedef ScriptingObjectInterfaceReferenceHelper<T> Helper;

public:
    typedef ScriptingObjectInterfaceReference<T> Type;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference"/> class.
    /// </summary>
    ScriptingObjectInterfaceReference()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="obj">The object to link.</param>
    ScriptingObjectInterfaceReference(SceneObject* obj)
        : ScriptingObjectReferenceBase(Helper::IsValidObject(obj) ? obj : nullptr)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="interfaceObj">The interface object to link.</param>
    ScriptingObjectInterfaceReference(T* interfaceObj)
        : ScriptingObjectReferenceBase(Helper::GetSceneObject(interfaceObj))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="other">The other property.</param>
    ScriptingObjectInterfaceReference(const ScriptingObjectInterfaceReference& other)
        : ScriptingObjectReferenceBase(other._object)
    {
    }

    ScriptingObjectInterfaceReference(ScriptingObjectInterfaceReference&& other) noexcept
        : ScriptingObjectReferenceBase(MoveTemp(other))
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ScriptingObjectInterfaceReference"/> class.
    /// </summary>
    ~ScriptingObjectInterfaceReference()
    {
    }

public:
    FORCE_INLINE bool operator==(SceneObject* other) const
    {
        return _object == other;
    }

    FORCE_INLINE bool operator!=(SceneObject* other) const
    {
        return _object != other;
    }

    FORCE_INLINE bool operator==(T* other) const
    {
        return Get() == other;
    }

    FORCE_INLINE bool operator!=(T* other) const
    {
        return Get() != other;
    }

    FORCE_INLINE bool operator==(const ScriptingObjectInterfaceReference& other) const
    {
        return _object == other._object;
    }

    FORCE_INLINE bool operator!=(const ScriptingObjectInterfaceReference& other) const
    {
        return _object != other._object;
    }

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(SceneObject* other)
    {
        OnSet(Helper::IsValidObject(other) ? other : nullptr);
        return *this;
    }

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(T* other)
    {
        OnSet(Helper::GetSceneObject(other));
        return *this;
    }

    ScriptingObjectInterfaceReference& operator=(const ScriptingObjectInterfaceReference& other)
    {
        OnSet(other._object);
        return *this;
    }

    ScriptingObjectInterfaceReference& operator=(ScriptingObjectInterfaceReference&& other) noexcept
    {
        ScriptingObjectReferenceBase::operator=(MoveTemp(other));
        return *this;
    }

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(const Guid& id)
    {
        OnSet(Helper::FindSceneObject(id));
        return *this;
    }

    /// <summary>
    /// Implicit conversion to the interface.
    /// </summary>
    FORCE_INLINE operator T*() const
    {
        return Get();
    }

    /// <summary>
    /// Implicit conversion to boolean value.
    /// </summary>
    FORCE_INLINE operator bool() const
    {
        return _object != nullptr;
    }

    /// <summary>
    /// Interface accessor.
    /// </summary>
    FORCE_INLINE T* operator->() const
    {
        return Get();
    }

    /// <summary>
    /// Gets the interface pointer.
    /// </summary>
    FORCE_INLINE T* Get() const
    {
        return ScriptingObject::ToInterface<T>(_object);
    }

    /// <summary>
    /// Gets the referenced object.
    /// </summary>
    FORCE_INLINE SceneObject* GetObject() const
    {
        return static_cast<SceneObject*>(_object);
    }

    /// <summary>
    /// Copies the object ID into the raw storage.
    /// </summary>
    FORCE_INLINE void CopyID(uint32 id[4]) const
    {
        memset(id, 0, sizeof(uint32) * 4);
        if (_object)
        {
            const Guid value = GetID();
            memcpy(id, &value, sizeof(uint32) * 4);
        }
    }

    /// <summary>
    /// Gets the object as a given type (static cast).
    /// </summary>
    template<typename U>
    FORCE_INLINE U* As() const
    {
        return static_cast<U*>(_object);
    }

};

template<typename T>
uint32 GetHash(const ScriptingObjectInterfaceReference<T>& key)
{
    return GetHash(key.GetID());
}
