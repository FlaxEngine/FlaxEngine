// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "MTypes.h"

/// <summary>
/// Encapsulates information about a single Mono (managed) fields belonging to some managed class. This object also allows you to access the field data of an object instance.
/// </summary>
class MField
{
    friend MClass;

protected:

#if USE_MONO
    MonoClassField* _monoField;
    MonoType* _monoType;
#endif

    MClass* _parentClass;

    MString _name;
    MVisibility _visibility;

    Array<MonoObject*> _attributes;

    int32 _hasCachedAttributes : 1;
    int32 _isStatic : 1;

public:

#if USE_MONO
    explicit MField(MonoClassField* monoField, const char* name, MClass* parentClass);
#endif

public:

    /// <summary>
    /// Gets field name.
    /// </summary>
    /// <returns>The field name.</returns>
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
    /// Gets field type class.
    /// </summary>
    /// <returns>The field type.</returns>
    MType GetType() const;

    /// <summary>
    /// Gets the field offset (in bytes) from the start of the parent object.
    /// </summary>
    /// <returns>The field offset in bytes.</returns>
    int32 GetOffset() const;

    /// <summary>
    /// Gets field visibility in the class.
    /// </summary>
    /// <returns>The field visibility.</returns>
    FORCE_INLINE MVisibility GetVisibility() const
    {
        return _visibility;
    }

    /// <summary>
    /// Returns true if field is static.
    /// </summary>
    /// <returns>True if is static, otherwise false.</returns>
    FORCE_INLINE bool IsStatic() const
    {
        return _isStatic != 0;
    }

#if USE_MONO

    /// <summary>
    /// Gets mono field handle.
    /// </summary>
    /// <returns>The Mono field object.</returns>
    FORCE_INLINE MonoClassField* GetNative() const
    {
        return _monoField;
    }

#endif

public:

    /// <summary>
    /// Retrieves value currently set in the field on the specified object instance. If field is static object instance can be null.
    /// </summary>
    /// <remarks>
    /// Value will be a pointer to raw data type for value types (for example int, float), and a MonoObject* for reference types.
    /// </remarks>
    /// <param name="instance">The object of given type to get value from.</param>
    /// <param name="result">The return value of undefined type.</param>
    void GetValue(MonoObject* instance, void* result) const;

    /// <summary>
    /// Retrieves value currently set in the field on the specified object instance. If field is static object instance can be null. If returned value is a value type it will be boxed.
    /// </summary>
    /// <param name="instance">The object of given type to get value from.</param>
    /// <returns>The boxed value object.</returns>
    MonoObject* GetValueBoxed(MonoObject* instance) const;

    /// <summary>
    /// Sets a value for the field on the specified object instance. If field is static object instance can be null.
    /// </summary>
    /// <remarks>
    /// Value should be a pointer to raw data type for value types (for example int, float), and a MonoObject* for reference types.
    /// </remarks>
    /// <param name="instance">The object of given type to set value to.</param>
    /// <param name="value">Th value of undefined type.</param>
    void SetValue(MonoObject* instance, void* value) const;

public:

    /// <summary>
    /// Checks if field has an attribute of the specified type.
    /// </summary>
    /// <param name="monoClass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(MClass* monoClass) const;

    /// <summary>
    /// Checks if field has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the field doesn't have such an attribute.
    /// </summary>
    /// <param name="monoClass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MonoObject* GetAttribute(MClass* monoClass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given field. Returns null if the field doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MonoObject*>& GetAttributes();
};
