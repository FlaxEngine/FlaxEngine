// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "MTypes.h"

/// <summary>
/// Contains information about a single managed class.
/// </summary>
class FLAXENGINE_API MClass
{
private:

#if USE_MONO
    MonoClass* _monoClass;
    void* _attrInfo;
#endif
    const MAssembly* _assembly;

    MString _fullname;

    Array<MMethod*> _methods;
    Array<MField*> _fields;
    Array<MProperty*> _properties;
    Array<MonoObject*> _attributes;
    Array<MEvent*> _events;

    MVisibility _visibility;

    int32 _hasCachedProperties : 1;
    int32 _hasCachedFields : 1;
    int32 _hasCachedMethods : 1;
    int32 _hasCachedAttributes : 1;
    int32 _hasCachedEvents : 1;
    int32 _isStatic : 1;
    int32 _isSealed : 1;
    int32 _isAbstract : 1;
    int32 _isInterface : 1;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MClass"/> class.
    /// </summary>
    /// <param name="parentAssembly">The parent assembly.</param>
    /// <param name="monoClass">The Mono class.</param>
    /// <param name="fullname">The fullname.</param>
    MClass(const MAssembly* parentAssembly, MonoClass* monoClass, const MString& fullname);

    /// <summary>
    /// Finalizes an instance of the <see cref="MClass"/> class.
    /// </summary>
    ~MClass();

public:

    /// <summary>
    /// Gets the parent assembly.
    /// </summary>
    /// <returns>The assembly.</returns>
    const MAssembly* GetAssembly() const
    {
        return _assembly;
    }

    /// <summary>
    /// Gets the full name of the class (namespace and typename).
    /// </summary>
    /// <returns>The fullname.</returns>
    FORCE_INLINE const MString& GetFullName() const
    {
        return _fullname;
    }

    /// <summary>
    /// Gets the class full name as string.
    /// </summary>
    /// <returns>The class full name.</returns>
    String ToString() const;

#if USE_MONO

    /// <summary>
    /// Gets the Mono class handle.
    /// </summary>
    /// <returns>The Mono class.</returns>
    MonoClass* GetNative() const;

#endif

    /// <summary>
    /// Gets class visibility
    /// </summary>
    /// <returns>Returns visibility struct.</returns>
    FORCE_INLINE MVisibility GetVisibility() const
    {
        return _visibility;
    }

    /// <summary>
    /// Gets if class is static
    /// </summary>
    /// <returns>Returns true if class is static, otherwise false.</returns>
    FORCE_INLINE bool IsStatic() const
    {
        return _isStatic != 0;
    }

    /// <summary>
    /// Gets if class is abstract
    /// </summary>
    /// <returns>Returns true if class is static, otherwise false.</returns>
    FORCE_INLINE bool IsAbstract() const
    {
        return _isAbstract != 0;
    }

    /// <summary>
    /// Gets if class is sealed
    /// </summary>
    /// <returns>Returns true if class is static, otherwise false.</returns>
    FORCE_INLINE bool IsSealed() const
    {
        return _isSealed != 0;
    }

    /// <summary>
    /// Gets if class is interface
    /// </summary>
    /// <returns>Returns true if class is static, otherwise false.</returns>
    FORCE_INLINE bool IsInterface() const
    {
        return _isInterface != 0;
    }

    /// <summary>
    /// Gets if class is generic
    /// </summary>
    /// <returns>Returns true if class is generic type, otherwise false.</returns>
    bool IsGeneric() const;

    /// <summary>
    /// Gets the class type.
    /// </summary>
    /// <returns>The type.</returns>
    MType GetType() const;

    /// <summary>
    /// Returns the base class of this class. Null if this class has no base.
    /// </summary>
    /// <returns>The base class.</returns>
    MClass* GetBaseClass() const;

    /// <summary>
    /// Checks if this class is a sub class of the specified class (including any derived types).
    /// </summary>
    /// <param name="klass">The class.</param>
    /// <returns>True if this class is a sub class of the specified class.</returns>
    bool IsSubClassOf(const MClass* klass) const;

    /// <summary>
    /// Checks if this class is a sub class of the specified class (including any derived types).
    /// </summary>
    /// <param name="monoClass">The Mono class.</param>
    /// <returns>True if this class is a sub class of the specified class.</returns>
    bool IsSubClassOf(MonoClass* monoClass) const;

    /// <summary>
    /// Checks is the provided object instance of this class' type.
    /// </summary>
    /// <param name="object">The object to check.</param>
    /// <returns>True if object  is an instance the this class.</returns>
    bool IsInstanceOfType(MonoObject* object) const;

    /// <summary>
    /// Returns the size of an instance of this class, in bytes.
    /// </summary>
    /// <returns>The instance size (in bytes).</returns>
    uint32 GetInstanceSize() const;

public:

    /// <summary>
    /// Returns an object referencing a method with the specified name and number of parameters. Optionally checks the base classes.
    /// </summary>
    /// <param name="name">The method name.</param>
    /// <param name="numParams">The method parameters count.</param>
    /// <param name="checkBaseClasses">True if check base classes when searching for the given method.</param>
    /// <returns>The method or null if failed to find it.</returns>
    MMethod* FindMethod(const char* name, int32 numParams, bool checkBaseClasses);

    /// <summary>
    /// Returns an object referencing a method with the specified name and number of parameters.
    /// </summary>
    /// <remarks>If the type contains more than one method of the given name and parameters count the returned value can be non-deterministic (one of the matching methods).</remarks>
    /// <param name="name">The method name.</param>
    /// <param name="numParams">The method parameters count.</param>
    /// <returns>The method or null if failed to get it.</returns>
    MMethod* GetMethod(const char* name, int32 numParams = 0);

    /// <summary>
    /// Returns all methods belonging to this class.
    /// </summary>
    /// <remarks>
    /// Be aware this will not include the methods of any base classes.
    /// </remarks>
    /// <returns>The list of methods.</returns>
    const Array<MMethod*>& GetMethods();

    /// <summary>
    /// Returns an object referencing a field with the specified name.
    /// </summary>
    /// <remarks>
    /// Does not query base class fields.
    /// Returns null if field cannot be found.	
    /// </remarks>
    /// <param name="name">The field name.</param>
    /// <returns>The field or null if failed.</returns>
    MField* GetField(const char* name);

    /// <summary>
    /// Returns all fields belonging to this class.
    /// </summary>
    /// <remarks>
    /// Be aware this will not include the fields of any base classes.
    /// </remarks>
    /// <returns>The list of fields.</returns>
    const Array<MField*>& GetFields();

    /// <summary>
    /// Returns an object referencing a event with the specified name.
    /// </summary>
    /// <param name="name">The event name.</param>
    /// <returns>The event object.</returns>
    MEvent* GetEvent(const char* name);

    /// <summary>
    /// Adds event with the specified name to the cache.
    /// </summary>
    /// <param name="name">The event name.</param>
    /// <param name="event">The event Mono object.</param>
    /// <returns>The event.</returns>
    MEvent* AddEvent(const char* name, MonoEvent* event);

    /// <summary>
    /// Returns all events belonging to this class.
    /// </summary>
    /// <returns>The list of events.</returns>
    const Array<MEvent*>& GetEvents();

    /// <summary>
    /// Returns an object referencing a property with the specified name.
    /// </summary>
    /// <remarks>
    /// Does not query base class properties.
    /// Returns null if property cannot be found.
    /// </remarks>
    /// <param name="name">The property name.</param>
    /// <returns>The property.</returns>
    MProperty* GetProperty(const char* name);

    /// <summary>
    /// Returns all properties belonging to this class.
    /// </summary>
    /// <remarks>
    /// Be aware this will not include the properties of any base classes.
    /// </remarks>
    /// <returns>The list of properties.</returns>
    const Array<MProperty*>& GetProperties();

public:

    /// <summary>
    /// Creates a new instance of this class and constructs it.
    /// </summary>
    /// <returns>The created managed object.</returns>
    MonoObject* CreateInstance() const;

    /// <summary>
    /// Creates a new instance of this class and then constructs it using the constructor with the specified number of parameters.
    /// </summary>
    /// <param name="params">The array containing pointers to constructor parameters. Array length must be equal to number of parameters.</param>
    /// <param name="numParams">The number of parameters the constructor accepts.</param>
    /// <returns>The created managed object.</returns>
    MonoObject* CreateInstance(void** params, uint32 numParams);

public:

    /// <summary>
    /// Checks if class has an attribute of the specified type.
    /// </summary>
    /// <param name="monoClass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(MClass* monoClass);

    /// <summary>
    /// Checks if class has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute();

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the class doesn't have such an attribute.
    /// </summary>
    /// <param name="monoClass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MonoObject* GetAttribute(MClass* monoClass);

    /// <summary>
    /// Returns an instance of all attributes connected with given class. Returns null if the class doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MonoObject*>& GetAttributes();
};
