// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/SoftObjectReference.h"
#include "Engine/Scripting/ScriptingObjectInterfaceReferenceUtils.h"

/// <summary>
/// The scene object soft interface reference. Objects gets referenced on use (ID reference is resolving it).
/// </summary>
/// <typeparam name="T">The type of the scripting interface.</typeparam>
template<typename T>
API_CLASS(InBuild) class SoftObjectInterfaceReference : public SoftObjectReferenceBase
{
    typedef ScriptingObjectInterfaceReferenceHelper<T> Helper;

public:
    typedef SoftObjectInterfaceReference<T> Type;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectInterfaceReference"/> class.
    /// </summary>
    SoftObjectInterfaceReference()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="obj">The object to link.</param>
    SoftObjectInterfaceReference(SceneObject* obj)
    {
        OnSet(Helper::IsValidObject(obj) ? obj : nullptr);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="interfaceObj">The interface object to link.</param>
    SoftObjectInterfaceReference(T* interfaceObj)
    {
        OnSet(Helper::GetSceneObject(interfaceObj));
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="other">The other property.</param>
    SoftObjectInterfaceReference(const SoftObjectInterfaceReference& other)
    {
        OnSet(other.GetID());
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="other">The other property.</param>
    SoftObjectInterfaceReference(SoftObjectInterfaceReference&& other)
    {
        OnSet(other.GetID());
        other.OnSet(nullptr);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SoftObjectInterfaceReference"/> class.
    /// </summary>
    ~SoftObjectInterfaceReference()
    {
    }

public:
    FORCE_INLINE bool operator==(SceneObject* other)
    {
        return GetObject() == other;
    }

    FORCE_INLINE bool operator!=(SceneObject* other)
    {
        return GetObject() != other;
    }

    FORCE_INLINE bool operator==(T* other)
    {
        return Get() == other;
    }

    FORCE_INLINE bool operator!=(T* other)
    {
        return Get() != other;
    }

    FORCE_INLINE bool operator==(const SoftObjectInterfaceReference& other)
    {
        return GetID() == other.GetID();
    }

    FORCE_INLINE bool operator!=(const SoftObjectInterfaceReference& other)
    {
        return GetID() != other.GetID();
    }

    SoftObjectInterfaceReference& operator=(const SoftObjectInterfaceReference& other)
    {
        if (this != &other)
            OnSet(other.GetID());
        return *this;
    }

    SoftObjectInterfaceReference& operator=(SoftObjectInterfaceReference&& other)
    {
        if (this != &other)
        {
            OnSet(other.GetID());
            other.OnSet(nullptr);
        }
        return *this;
    }

    FORCE_INLINE SoftObjectInterfaceReference& operator=(SceneObject* other)
    {
        OnSet(Helper::IsValidObject(other) ? other : nullptr);
        return *this;
    }

    FORCE_INLINE SoftObjectInterfaceReference& operator=(T* other)
    {
        OnSet(Helper::GetSceneObject(other));
        return *this;
    }

    FORCE_INLINE SoftObjectInterfaceReference& operator=(const Guid& id)
    {
        OnSet(id);
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
        return Get() != nullptr;
    }

    /// <summary>
    /// Interface accessor.
    /// </summary>
    FORCE_INLINE T* operator->() const
    {
        return Get();
    }

    /// <summary>
    /// Gets the object as a given type (static cast).
    /// </summary>
    template<typename U>
    FORCE_INLINE U* As() const
    {
        return static_cast<U*>(GetObject());
    }

public:
    /// <summary>
    /// Gets the interface pointer.
    /// </summary>
    FORCE_INLINE T* Get() const
    {
        return ScriptingObject::ToInterface<T>(GetObject());
    }

    /// <summary>
    /// Gets the referenced object.
    /// </summary>
    SceneObject* GetObject() const
    {
        if (!_object)
            const_cast<SoftObjectInterfaceReference*>(this)->OnResolve(SceneObject::GetStaticClass());
        return Helper::IsValidObject(static_cast<SceneObject*>(_object)) ? static_cast<SceneObject*>(_object) : nullptr;
    }

    /// <summary>
    /// Gets managed instance object (or null if no object linked).
    /// </summary>
    MObject* GetManagedInstance() const
    {
        auto object = GetObject();
        return object ? object->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Determines whether object is assigned and managed instance of the object is alive.
    /// </summary>
    bool HasManagedInstance() const
    {
        auto object = GetObject();
        return object && object->HasManagedInstance();
    }

    /// <summary>
    /// Gets the managed instance object or creates it if missing or null if not assigned.
    /// </summary>
    MObject* GetOrCreateManagedInstance() const
    {
        auto object = GetObject();
        return object ? object->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Copies the object ID into the raw storage.
    /// </summary>
    FORCE_INLINE void CopyID(uint32 id[4]) const
    {
        const Guid value = GetID();
        memcpy(id, &value, sizeof(uint32) * 4);
    }

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="id">The object ID. Uses Scripting to find the registered object of the given ID.</param>
    FORCE_INLINE void Set(const Guid& id)
    {
        OnSet(id);
    }

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="object">The object.</param>
    FORCE_INLINE void Set(SceneObject* object)
    {
        OnSet(Helper::IsValidObject(object) ? object : nullptr);
    }

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="interfaceObj">The interface object.</param>
    FORCE_INLINE void Set(T* interfaceObj)
    {
        OnSet(Helper::GetSceneObject(interfaceObj));
    }
};

template<typename T>
uint32 GetHash(const SoftObjectInterfaceReference<T>& key)
{
    return GetHash(key.GetID());
}
