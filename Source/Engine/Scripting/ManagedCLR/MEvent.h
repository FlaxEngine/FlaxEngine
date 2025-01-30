// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "MMethod.h"

/// <summary>
/// Encapsulates information about a single Mono (managed) event belonging to some managed class. This object also allows you to invoke this event or register other methods to it.
/// </summary>
class FLAXENGINE_API MEvent
{
    friend MClass;
    friend MCore;

protected:
#if USE_MONO
    MonoEvent* _monoEvent;
#elif USE_NETCORE
    void* _handle;
#endif

    mutable MMethod* _addMethod;
    mutable MMethod* _removeMethod;
    MClass* _parentClass;

    StringAnsi _name;

    mutable int32 _hasCachedAttributes : 1;
    mutable int32 _hasAddMonoMethod : 1;
    mutable int32 _hasRemoveMonoMethod : 1;

    mutable Array<MObject*> _attributes;

public:
#if USE_MONO
    explicit MEvent(MonoEvent* monoEvent, const char* name, MClass* parentClass);
#elif USE_NETCORE
    MEvent(MClass* parentClass, void* handle, const char* name);
#endif

public:
    /// <summary>
    /// Gets the event name.
    /// </summary>
    FORCE_INLINE const StringAnsi& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Returns the parent class that this method is contained with.
    /// </summary>
    FORCE_INLINE MClass* GetParentClass() const
    {
        return _parentClass;
    }

    /// <summary>
    /// Gets the event type class.
    /// </summary>
    MType* GetType() const;

    /// <summary>
    /// Gets the event add method.
    /// </summary>
    MMethod* GetAddMethod() const;

    /// <summary>
    /// Gets the event remove method.
    /// </summary>
    MMethod* GetRemoveMethod() const;

    /// <summary>
    /// Gets event visibility in the class.
    /// </summary>
    FORCE_INLINE MVisibility GetVisibility() const
    {
        return GetAddMethod()->GetVisibility();
    }

    /// <summary>
    /// Returns true if event is static.
    /// </summary>
    FORCE_INLINE bool IsStatic() const
    {
        return GetAddMethod()->IsStatic();
    }

#if USE_MONO
    /// <summary>
    /// Gets the Mono event handle.
    /// </summary>
    FORCE_INLINE MonoEvent* GetNative() const
    {
        return _monoEvent;
    }
#endif

public:
    /// <summary>
    /// Checks if event has an attribute of the specified type.
    /// </summary>
    /// <param name="klass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(const MClass* klass) const;

    /// <summary>
    /// Checks if event has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the event doesn't have such an attribute.
    /// </summary>
    /// <param name="klass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MObject* GetAttribute(const MClass* klass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given event. Returns null if the event doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MObject*>& GetAttributes() const;
};
