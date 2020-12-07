// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "MMethod.h"

/// <summary>
/// Encapsulates information about a single Mono (managed) event belonging to some managed class. This object also allows you to invoke this event or register other methods to it.
/// </summary>
class FLAXENGINE_API MEvent
{
    friend MClass;

protected:

#if USE_MONO
    MonoEvent* _monoEvent;
#endif

    MMethod* _addMethod;
    MMethod* _removeMethod;
    MClass* _parentClass;

    MString _name;

    Array<MonoObject*> _attributes;

    int32 _hasCachedAttributes : 1;
    int32 _hasAddMonoMethod : 1;
    int32 _hasRemoveMonoMethod : 1;

public:

#if USE_MONO
    explicit MEvent(MonoEvent* monoEvent, const char* name, MClass* parentClass);
#endif

public:

    /// <summary>
    /// Gets the event name.
    /// </summary>
    /// <returns>The name.</returns>
    FORCE_INLINE const MString& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Returns the parent class that this method is contained with.
    /// </summary>
    /// <returns>The parent class.</returns>
    FORCE_INLINE MClass* GetParentClass() const
    {
        return _parentClass;
    }

    /// <summary>
    /// Gets the event type class.
    /// </summary>
    /// <returns>The type class.</returns>
    MType GetType();

    /// <summary>
    /// Gets the event add method.
    /// </summary>
    /// <returns>The method object.</returns>
    MMethod* GetAddMethod();

    /// <summary>
    /// Gets the event remove method.
    /// </summary>
    /// <returns>The method object.</returns>
    MMethod* GetRemoveMethod();

    /// <summary>
    /// Gets event visibility in the class.
    /// </summary>
    /// <returns>The event visibility.</returns>
    FORCE_INLINE MVisibility GetVisibility()
    {
        return GetAddMethod()->GetVisibility();
    }

    /// <summary>
    /// Returns true if event is static.
    /// </summary>
    /// <returns>True if is static, otherwise false.</returns>
    FORCE_INLINE bool IsStatic()
    {
        return GetAddMethod()->IsStatic();
    }

#if USE_MONO

    /// <summary>
    /// Gets the Mono event handle.
    /// </summary>
    /// <returns>The Mono event.</returns>
    FORCE_INLINE MonoEvent* GetNative() const
    {
        return _monoEvent;
    }

#endif

public:

    /// <summary>
    /// Checks if event has an attribute of the specified type.
    /// </summary>
    /// <param name="monoClass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(MClass* monoClass) const;

    /// <summary>
    /// Checks if event has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the event doesn't have such an attribute.
    /// </summary>
    /// <param name="monoClass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MonoObject* GetAttribute(MClass* monoClass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given event. Returns null if the event doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MonoObject*>& GetAttributes();
};
