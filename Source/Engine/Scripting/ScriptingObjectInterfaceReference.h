// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ScriptingObjectReference.h"

/// <summary>
/// The scripting object reference with interface.
/// </summary>
/// <typeparam name="T">The type of the scripting interface.</typeparam>
template<typename T>
API_CLASS(Template, MarshalAs=ScriptingObject*) class ScriptingObjectInterfaceReference : public ScriptingObjectReferenceBase
{
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
    ScriptingObjectInterfaceReference(ScriptingObject* obj)
        : ScriptingObjectReferenceBase(IsValid(obj) ? obj : nullptr)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference"/> class.
    /// </summary>
    /// <param name="interfaceObj">The interface object to link.</param>
    ScriptingObjectInterfaceReference(T* interfaceObj)
        : ScriptingObjectReferenceBase(ScriptingObject::FromInterface<T>(interfaceObj))
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
    FORCE_INLINE bool operator==(ScriptingObject* other) const
    {
        return _object == other;
    }

    FORCE_INLINE bool operator!=(ScriptingObject* other) const
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

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(ScriptingObject* other)
    {
        OnSet(IsValid(other) ? other : nullptr);
        return *this;
    }

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(T* other)
    {
        OnSet(ScriptingObject::FromInterface<T>(other));
        return *this;
    }

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(const ScriptingObjectInterfaceReference& other)
    {
        OnSet(other._object);
        return *this;
    }

    FORCE_INLINE ScriptingObjectInterfaceReference& operator=(ScriptingObjectInterfaceReference&& other) noexcept
    {
        ScriptingObjectReferenceBase::operator=(MoveTemp(other));
        return *this;
    }

    ScriptingObjectInterfaceReference& operator=(const Guid& id)
    {
        ScriptingObject* obj = FindObject(id, ScriptingObject::GetStaticClass());
        OnSet(IsValid(obj) ? obj : nullptr);
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
    /// Implicit conversion to the object.
    /// </summary>
    FORCE_INLINE operator ScriptingObject*() const
    {
        return _object;
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
    FORCE_INLINE ScriptingObject* GetObject() const
    {
        return _object;
    }

    /// <summary>
    /// Gets managed instance object.
    /// </summary>
    FORCE_INLINE MObject* GetManagedInstance() const
    {
        return _object ? _object->GetOrCreateManagedInstance() : nullptr;
    }

private:
    FORCE_INLINE static bool IsValid(const ScriptingObject* obj)
    {
        return !obj || obj->GetType().GetInterface(T::TypeInitializer);
    }
};

template<typename T>
uint32 GetHash(const ScriptingObjectInterfaceReference<T>& key)
{
    return GetHash(key.GetID());
}
