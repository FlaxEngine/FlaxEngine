// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "MTypes.h"

/// <summary>
/// Encapsulates information about a single Mono managed property belonging to some managed class.
/// This object also allows you to set or retrieve values to or from specific instances containing the property.
/// </summary>
class FLAXENGINE_API MProperty
{
    friend MClass;

protected:

#if USE_MONO
    MonoProperty* _monoProperty;
#endif

    MMethod* _getMethod;
    MMethod* _setMethod;
    MClass* _parentClass;

    MString _name;

    Array<MonoObject*> _attributes;

    int32 _hasCachedAttributes : 1;
    int32 _hasSetMethod : 1;
    int32 _hasGetMethod : 1;

public:

#if USE_MONO
    explicit MProperty(MonoProperty* monoProperty, const char* name, MClass* parentClass);
#endif

    /// <summary>
    /// Finalizes an instance of the <see cref="MProperty"/> class.
    /// </summary>
    ~MProperty();

public:

    /// <summary>
    /// Gets the property name.
    /// </summary>
    /// <returns>The property name.</returns>
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
    /// Gets property type class.
    /// </summary>
    /// <returns>The property type.</returns>
    MType GetType();

    /// <summary>
    /// Gets property get method.
    /// </summary>
    /// <returns>The getter method.</returns>
    MMethod* GetGetMethod();

    /// <summary>
    /// Gets property set method.
    /// </summary>
    /// <returns>The setter method.</returns>
    MMethod* GetSetMethod();

    /// <summary>
    /// Gets property visibility in the class.
    /// </summary>
    /// <returns>The property visibility.</returns>
    MVisibility GetVisibility();

    /// <summary>
    /// Returns true if property is static.
    /// </summary>
    /// <returns>True if is static, otherwise false.</returns>
    bool IsStatic();

public:

    /// <summary>
    /// Retrieves value currently set in the property on the specified object instance. If property is static object instance can be null.
    /// </summary>
    /// <remarks>
    /// Value will be a pointer to raw data type for value types (for example int, float), and a MonoObject* for reference types.
    /// </remarks>
    /// <param name="instance">The object of given type to get value from.</param>
    /// <param name="exception">An optional pointer to the exception value to store exception object reference.</param>
    /// <returns>The returned boxed value object.</returns>
    MonoObject* GetValue(MonoObject* instance, MonoObject** exception);

    /// <summary>
    /// Sets a value for the property on the specified object instance. If property is static object instance can be null.
    /// </summary>
    /// <remarks>
    /// Value should be a pointer to raw data type for value types (for example int, float), and a MonoObject* for reference types.
    /// </remarks>
    /// <param name="instance">Object of given type to set value to.</param>
    /// <param name="exception">An optional pointer to the exception value to store exception object reference.</param>
    /// <param name="value">The value to set of undefined type.</param>
    void SetValue(MonoObject* instance, void* value, MonoObject** exception);

public:

    /// <summary>
    /// Checks if property has an attribute of the specified type.
    /// </summary>
    /// <param name="monoClass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(MClass* monoClass) const;

    /// <summary>
    /// Checks if property has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the property doesn't have such an attribute.
    /// </summary>
    /// <param name="monoClass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MonoObject* GetAttribute(MClass* monoClass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given property. Returns null if the property doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MonoObject*>& GetAttributes();
};
