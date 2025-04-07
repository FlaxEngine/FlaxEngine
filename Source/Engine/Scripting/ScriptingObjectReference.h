// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

// Don't include Scripting.h but just FindObject method
extern FLAXENGINE_API ScriptingObject* FindObject(const Guid& id, MClass* type);

/// <summary>
/// The scripting object reference.
/// </summary>
/// <typeparam name="T">The type of the scripting object.</typeparam>
class FLAXENGINE_API ScriptingObjectReferenceBase
{
public:
    typedef Delegate<> EventType;

protected:
    ScriptingObject* _object = nullptr;

public:
    /// <summary>
    /// Action fired when reference gets changed.
    /// </summary>
    EventType Changed;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectReferenceBase"/> class.
    /// </summary>
    ScriptingObjectReferenceBase()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectReferenceBase"/> class.
    /// </summary>
    /// <param name="obj">The object to link.</param>
    ScriptingObjectReferenceBase(ScriptingObject* obj)
    {
        OnSet(obj);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ScriptingObjectReference"/> class.
    /// </summary>
    ~ScriptingObjectReferenceBase()
    {
        ScriptingObject* obj = _object;
        if (obj)
        {
            _object = nullptr;
            obj->Deleted.Unbind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
        }
    }

public:
    /// <summary>
    /// Gets the object ID.
    /// </summary>
    FORCE_INLINE Guid GetID() const
    {
        return _object ? _object->GetID() : Guid::Empty;
    }

    /// <summary>
    /// Gets managed instance object (or null if no object linked).
    /// </summary>
    FORCE_INLINE MObject* GetManagedInstance() const
    {
        return _object ? _object->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Determines whether object is assigned and managed instance of the object is alive.
    /// </summary>
    FORCE_INLINE bool HasManagedInstance() const
    {
        return _object && _object->HasManagedInstance();
    }

    /// <summary>
    /// Gets the managed instance object or creates it if missing or null if not assigned.
    /// </summary>
    FORCE_INLINE MObject* GetOrCreateManagedInstance() const
    {
        return _object ? _object->GetOrCreateManagedInstance() : nullptr;
    }

protected:
    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="object">The object.</param>
    void OnSet(ScriptingObject* object);

    void OnDeleted(ScriptingObject* obj);
};

/// <summary>
/// The scripting object reference.
/// </summary>
template<typename T>
API_CLASS(InBuild) class ScriptingObjectReference : public ScriptingObjectReferenceBase
{
public:
    typedef ScriptingObjectReference<T> Type;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectReference"/> class.
    /// </summary>
    ScriptingObjectReference()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectReference"/> class.
    /// </summary>
    /// <param name="obj">The object to link.</param>
    ScriptingObjectReference(T* obj)
        : ScriptingObjectReferenceBase(obj)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectReference"/> class.
    /// </summary>
    /// <param name="other">The other property.</param>
    ScriptingObjectReference(const ScriptingObjectReference& other)
        : ScriptingObjectReferenceBase(other._object)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ScriptingObjectReference"/> class.
    /// </summary>
    ~ScriptingObjectReference()
    {
    }

public:
    FORCE_INLINE bool operator==(T* other) const
    {
        return _object == other;
    }

    FORCE_INLINE bool operator!=(T* other) const
    {
        return _object != other;
    }

    FORCE_INLINE bool operator==(const ScriptingObjectReference& other) const
    {
        return _object == other._object;
    }

    FORCE_INLINE bool operator!=(const ScriptingObjectReference& other) const
    {
        return _object != other._object;
    }

    FORCE_INLINE ScriptingObjectReference& operator=(T* other)
    {
        OnSet(other);
        return *this;
    }

    ScriptingObjectReference& operator=(const ScriptingObjectReference& other)
    {
        OnSet(other._object);
        return *this;
    }

    FORCE_INLINE ScriptingObjectReference& operator=(const Guid& id)
    {
        OnSet(static_cast<ScriptingObject*>(FindObject(id, T::GetStaticClass())));
        return *this;
    }

    /// <summary>
    /// Implicit conversion to the object.
    /// </summary>
    FORCE_INLINE operator T*() const
    {
        return (T*)_object;
    }

    /// <summary>
    /// Implicit conversion to boolean value.
    /// </summary>
    FORCE_INLINE operator bool() const
    {
        return _object != nullptr;
    }

    /// <summary>
    /// Object accessor.
    /// </summary>
    FORCE_INLINE T* operator->() const
    {
        return (T*)_object;
    }

    /// <summary>
    /// Gets the object pointer.
    /// </summary>
    FORCE_INLINE T* Get() const
    {
        return (T*)_object;
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
uint32 GetHash(const ScriptingObjectReference<T>& key)
{
    return GetHash(key.GetID());
}
