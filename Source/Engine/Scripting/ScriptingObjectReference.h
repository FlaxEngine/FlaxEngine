// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

// Don't include Scripting.h but just FindObject method
extern FLAXENGINE_API ScriptingObject* FindObject(const Guid& id, MClass* type);

/// <summary>
/// The scripting object reference.
/// </summary>
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
        if (_object)
            _object->Deleted.Unbind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
    }

public:

    /// <summary>
    /// Gets the object ID.
    /// </summary>
    /// <returns>The object ID or Guid::Empty if nothing assigned.</returns>
    FORCE_INLINE Guid GetID() const
    {
        return _object ? _object->GetID() : Guid::Empty;
    }

    /// <summary>
    /// Gets managed instance object (or null if no object linked).
    /// </summary>
    /// <returns>The managed object instance.</returns>
    FORCE_INLINE MonoObject* GetManagedInstance() const
    {
        return _object ? _object->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Determines whether object is assigned and managed instance of the object is alive.
    /// </summary>
    /// <returns>True if managed object has been created and exists, otherwise false.</returns>
    FORCE_INLINE bool HasManagedInstance() const
    {
        return _object && _object->HasManagedInstance();
    }

    /// <summary>
    /// Gets the managed instance object or creates it if missing or null if not assigned.
    /// </summary>
    /// <returns>The Mono managed object.</returns>
    FORCE_INLINE MonoObject* GetOrCreateManagedInstance() const
    {
        return _object ? _object->GetOrCreateManagedInstance() : nullptr;
    }

protected:

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="object">The object.</param>
    void OnSet(ScriptingObject* object)
    {
        auto e = _object;
        if (e != object)
        {
            if (e)
                e->Deleted.Unbind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
            _object = e = object;
            if (e)
                e->Deleted.Bind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
            Changed();
        }
    }

    void OnDeleted(ScriptingObject* obj)
    {
        ASSERT(_object == obj);
        _object->Deleted.Unbind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
        _object = nullptr;
        Changed();
    }
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
        : ScriptingObjectReferenceBase(other.Get())
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ScriptingObjectReference"/> class.
    /// </summary>
    ~ScriptingObjectReference()
    {
    }

public:

    /// <summary>
    /// Compares the property value with the given object.
    /// </summary>
    /// <param name="other">The other.</param>
    /// <returns>True if property object equals the given value.</returns>
    FORCE_INLINE bool operator==(T* other)
    {
        return _object == other;
    }

    /// <summary>
    /// Compares the property value with the other property value.
    /// </summary>
    /// <param name="other">The other property.</param>
    /// <returns>True if properties are equal.</returns>
    FORCE_INLINE bool operator==(const ScriptingObjectReference& other)
    {
        return _object == other._object;
    }

    /// <summary>
    /// Compares the property value with the given object.
    /// </summary>
    /// <param name="other">The other.</param>
    /// <returns>True if property object not equals the given value.</returns>
    FORCE_INLINE bool operator!=(T* other)
    {
        return _object != other;
    }

    /// <summary>
    /// Compares the property value with the other property value.
    /// </summary>
    /// <param name="other">The other property.</param>
    /// <returns>True if properties are not equal.</returns>
    FORCE_INLINE bool operator!=(const ScriptingObjectReference& other)
    {
        return _object != other._object;
    }

    /// <summary>
    /// Sets the property to the given property value.
    /// </summary>
    /// <param name="other">The other property.</param>
    /// <returns>The reference to this property.</returns>
    ScriptingObjectReference& operator=(const ScriptingObjectReference& other)
    {
        if (this != &other)
            OnSet(other.Get());
        return *this;
    }

    /// <summary>
    /// Sets the property to the given value.
    /// </summary>
    /// <param name="other">The object.</param>
    /// <returns>The reference to this property.</returns>
    FORCE_INLINE ScriptingObjectReference& operator=(const T& other)
    {
        OnSet(&other);
        return *this;
    }

    /// <summary>
    /// Sets the property to the given value.
    /// </summary>
    /// <param name="other">The object.</param>
    /// <returns>The reference to this property.</returns>
    FORCE_INLINE ScriptingObjectReference& operator=(T* other)
    {
        OnSet(other);
        return *this;
    }

    /// <summary>
    /// Sets the property to the object of the given ID.
    /// </summary>
    /// <param name="id">The object ID.</param>
    /// <returns>The reference to this property.</returns>
    FORCE_INLINE ScriptingObjectReference& operator=(const Guid& id)
    {
        Set(id);
        return *this;
    }

    /// <summary>
    /// Implicit conversion to the object.
    /// </summary>
    /// <returns>The object reference.</returns>
    FORCE_INLINE operator T*() const
    {
        return (T*)_object;
    }

    /// <summary>
    /// Implicit conversion to boolean value.
    /// </summary>
    /// <returns>True if object has been assigned, otherwise false</returns>
    FORCE_INLINE operator bool() const
    {
        return _object != nullptr;
    }

    /// <summary>
    /// Object accessor.
    /// </summary>
    /// <returns>The object reference.</returns>
    FORCE_INLINE T* operator->() const
    {
        return (T*)_object;
    }

    /// <summary>
    /// Gets the object pointer.
    /// </summary>
    /// <returns>The object reference.</returns>
    FORCE_INLINE T* Get() const
    {
        return (T*)_object;
    }

    /// <summary>
    /// Gets the object as a given type (static cast).
    /// </summary>
    /// <returns>Asset</returns>
    template<typename U>
    FORCE_INLINE U* As() const
    {
        return static_cast<U*>(_object);
    }

public:

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="id">The object ID. Uses Scripting to find the registered object of the given ID.</param>
    FORCE_INLINE void Set(const Guid& id)
    {
        Set(static_cast<T*>(FindObject(id, T::GetStaticClass())));
    }

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="object">The object.</param>
    FORCE_INLINE void Set(T* object)
    {
        OnSet(object);
    }
};
