// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

// Don't include Scripting.h but just FindObject method
extern FLAXENGINE_API ScriptingObject* FindObject(const Guid& id, MClass* type);

/// <summary>
/// The scripting object soft reference. Objects gets referenced on use (ID reference is resolving it).
/// </summary>
class FLAXENGINE_API SoftObjectReferenceBase
{
public:

    typedef Delegate<> EventType;

protected:

    ScriptingObject* _object = nullptr;
    Guid _id = Guid::Empty;

public:

    /// <summary>
    /// Action fired when reference gets changed.
    /// </summary>
    EventType Changed;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectReferenceBase"/> class.
    /// </summary>
    SoftObjectReferenceBase()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectReferenceBase"/> class.
    /// </summary>
    /// <param name="obj">The object to link.</param>
    SoftObjectReferenceBase(ScriptingObject* obj)
    {
        OnSet(obj);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SoftObjectReference"/> class.
    /// </summary>
    ~SoftObjectReferenceBase()
    {
        if (_object)
            _object->Deleted.Unbind<SoftObjectReferenceBase, &SoftObjectReferenceBase::OnDeleted>(this);
    }

public:

    /// <summary>
    /// Gets the object ID.
    /// </summary>
    /// <returns>The object ID or Guid::Empty if nothing assigned.</returns>
    Guid GetID() const
    {
        return _object ? _object->GetID() : _id;
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
                e->Deleted.Unbind<SoftObjectReferenceBase, &SoftObjectReferenceBase::OnDeleted>(this);
            _object = e = object;
            _id = e ? e->GetID() : Guid::Empty;
            if (e)
                e->Deleted.Bind<SoftObjectReferenceBase, &SoftObjectReferenceBase::OnDeleted>(this);
            Changed();
        }
    }

    void OnDeleted(ScriptingObject* obj)
    {
        ASSERT(_object == obj);
        _object->Deleted.Unbind<SoftObjectReferenceBase, &SoftObjectReferenceBase::OnDeleted>(this);
        _object = nullptr;
        Changed();
    }
};

/// <summary>
/// The scripting object soft reference. Objects gets referenced on use (ID reference is resolving it).
/// </summary>
template<typename T>
API_CLASS(InBuild) class SoftObjectReference : public SoftObjectReferenceBase
{
public:

    typedef SoftObjectReference<T> Type;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectReference"/> class.
    /// </summary>
    SoftObjectReference()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectReference"/> class.
    /// </summary>
    /// <param name="obj">The object to link.</param>
    SoftObjectReference(T* obj)
        : SoftObjectReferenceBase(obj)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftObjectReference"/> class.
    /// </summary>
    /// <param name="other">The other property.</param>
    SoftObjectReference(const SoftObjectReference& other)
        : SoftObjectReferenceBase(other.Get())
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SoftObjectReference"/> class.
    /// </summary>
    ~SoftObjectReference()
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
        return Get() == other;
    }

    /// <summary>
    /// Compares the property value with the other property value.
    /// </summary>
    /// <param name="other">The other property.</param>
    /// <returns>True if properties are equal.</returns>
    FORCE_INLINE bool operator==(const SoftObjectReference& other)
    {
        return _id == other._id;
    }

    /// <summary>
    /// Compares the property value with the given object.
    /// </summary>
    /// <param name="other">The other.</param>
    /// <returns>True if property object not equals the given value.</returns>
    FORCE_INLINE bool operator!=(T* other)
    {
        return Get() != other;
    }

    /// <summary>
    /// Compares the property value with the other property value.
    /// </summary>
    /// <param name="other">The other property.</param>
    /// <returns>True if properties are not equal.</returns>
    FORCE_INLINE bool operator!=(const SoftObjectReference& other)
    {
        return _id != other._id;
    }

    /// <summary>
    /// Sets the property to the given property value.
    /// </summary>
    /// <param name="other">The other property.</param>
    /// <returns>The reference to this property.</returns>
    SoftObjectReference& operator=(const SoftObjectReference& other)
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
    FORCE_INLINE SoftObjectReference& operator=(const T& other)
    {
        OnSet(&other);
        return *this;
    }

    /// <summary>
    /// Sets the property to the given value.
    /// </summary>
    /// <param name="other">The object.</param>
    /// <returns>The reference to this property.</returns>
    FORCE_INLINE SoftObjectReference& operator=(T* other)
    {
        OnSet(other);
        return *this;
    }

    /// <summary>
    /// Sets the property to the object of the given ID.
    /// </summary>
    /// <param name="id">The object ID.</param>
    /// <returns>The reference to this property.</returns>
    FORCE_INLINE SoftObjectReference& operator=(const Guid& id)
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
        return (T*)Get();
    }

    /// <summary>
    /// Implicit conversion to boolean value.
    /// </summary>
    /// <returns>True if object has been assigned, otherwise false</returns>
    FORCE_INLINE operator bool() const
    {
        return _object != nullptr || _id.IsValid();
    }

    /// <summary>
    /// Object accessor.
    /// </summary>
    /// <returns>The object reference.</returns>
    FORCE_INLINE T* operator->() const
    {
        return (T*)Get();
    }

    /// <summary>
    /// Gets the object pointer.
    /// </summary>
    /// <returns>The object reference.</returns>
    T* Get() const
    {
        if (!_object)
            const_cast<SoftObjectReference*>(this)->OnSet(FindObject(_id, T::GetStaticClass()));
        return (T*)_object;
    }

    /// <summary>
    /// Gets the object as a given type (static cast).
    /// </summary>
    /// <returns>Asset</returns>
    template<typename U>
    FORCE_INLINE U* As() const
    {
        return static_cast<U*>(Get());
    }

public:

    /// <summary>
    /// Gets managed instance object (or null if no object linked).
    /// </summary>
    /// <returns>The managed object instance.</returns>
    MonoObject* GetManagedInstance() const
    {
        auto object = Get();
        return object ? object->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Determines whether object is assigned and managed instance of the object is alive.
    /// </summary>
    /// <returns>True if managed object has been created and exists, otherwise false.</returns>
    bool HasManagedInstance() const
    {
        auto object = Get();
        return object && object->HasManagedInstance();
    }

    /// <summary>
    /// Gets the managed instance object or creates it if missing or null if not assigned.
    /// </summary>
    /// <returns>The Mono managed object.</returns>
    MonoObject* GetOrCreateManagedInstance() const
    {
        auto object = Get();
        return object ? object->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Sets the object.
    /// </summary>
    /// <param name="id">The object ID. Uses Scripting to find the registered object of the given ID.</param>
    void Set(const Guid& id)
    {
        _id = id;
        _object = nullptr;
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
