// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "MTypes.h"

/// <summary>
/// Encapsulates information about a single Mono (managed) fields belonging to some managed class. This object also allows you to access the field data of an object instance.
/// </summary>
class FLAXENGINE_API MField
{
    friend MClass;
    friend MCore;

protected:
#if USE_MONO
    MonoClassField* _monoField;
    MonoType* _monoType;
#elif USE_NETCORE
    void* _handle;
    void* _type;
    int32 _fieldOffset;
#endif

    MClass* _parentClass;
    StringAnsi _name;

    MVisibility _visibility;

    mutable int32 _hasCachedAttributes : 1;
    int32 _isStatic : 1;

    mutable Array<MObject*> _attributes;

public:
#if USE_MONO
    explicit MField(MonoClassField* monoField, const char* name, MClass* parentClass);
#elif USE_NETCORE
    MField(MClass* parentClass, void* handle, const char* name, void* type, int fieldOffset, MFieldAttributes attributes);
#endif

public:
    /// <summary>
    /// Gets field name.
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
    /// Gets field type class.
    /// </summary>
    MType* GetType() const;

    /// <summary>
    /// Gets the field offset (in bytes) from the start of the parent object.
    /// </summary>
    int32 GetOffset() const;

    /// <summary>
    /// Gets field visibility in the class.
    /// </summary>
    FORCE_INLINE MVisibility GetVisibility() const
    {
        return _visibility;
    }

    /// <summary>
    /// Returns true if field is static.
    /// </summary>
    FORCE_INLINE bool IsStatic() const
    {
        return _isStatic != 0;
    }

#if USE_MONO
    /// <summary>
    /// Gets mono field handle.
    /// </summary>
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
    /// Value will be a pointer to raw data type for value types (for example int, float), and a MObject* for reference types.
    /// </remarks>
    /// <param name="instance">The object of given type to get value from.</param>
    /// <param name="result">The return value of undefined type.</param>
    void GetValue(MObject* instance, void* result) const;

    /// <summary>
    /// Retrieves value currently set in the field on the specified object instance. If field is static object instance can be null.
    /// </summary>
    /// <remarks>
    /// Value will be a pointer.
    /// </remarks>
    /// <param name="instance">The object of given type to get value from.</param>
    /// <param name="result">The return value of undefined type.</param>
    void GetValueReference(MObject* instance, void* result) const;

    /// <summary>
    /// Retrieves value currently set in the field on the specified object instance. If field is static object instance can be null. If returned value is a value type it will be boxed.
    /// </summary>
    /// <param name="instance">The object of given type to get value from.</param>
    /// <returns>The boxed value object.</returns>
    MObject* GetValueBoxed(MObject* instance) const;

    /// <summary>
    /// Sets a value for the field on the specified object instance. If field is static object instance can be null.
    /// </summary>
    /// <remarks>
    /// Value should be a pointer to raw data type for value types (for example int, float), and a MObject* for reference types.
    /// </remarks>
    /// <param name="instance">The object of given type to set value to.</param>
    /// <param name="value">Th value of undefined type.</param>
    void SetValue(MObject* instance, void* value) const;

public:
    /// <summary>
    /// Checks if field has an attribute of the specified type.
    /// </summary>
    /// <param name="klass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(const MClass* klass) const;

    /// <summary>
    /// Checks if field has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the field doesn't have such an attribute.
    /// </summary>
    /// <param name="klass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MObject* GetAttribute(const MClass* klass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given field. Returns null if the field doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MObject*>& GetAttributes() const;
};
