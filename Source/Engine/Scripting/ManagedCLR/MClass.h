// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
    mutable void* _attrInfo = nullptr;
#endif
    const MAssembly* _assembly;

    MString _fullname;

    Array<MMethod*> _methods;
    Array<MField*> _fields;
    Array<MProperty*> _properties;
    Array<MObject*> _attributes;
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

#if USE_MONO
    /// <summary>
    /// Initializes a new instance of the <see cref="MClass"/> class.
    /// </summary>
    /// <param name="parentAssembly">The parent assembly.</param>
    /// <param name="monoClass">The Mono class.</param>
    /// <param name="fullname">The fullname.</param>
    MClass(const MAssembly* parentAssembly, MonoClass* monoClass, const MString& fullname);
#endif

    /// <summary>
    /// Finalizes an instance of the <see cref="MClass"/> class.
    /// </summary>
    ~MClass();

public:

    /// <summary>
    /// Gets the parent assembly.
    /// </summary>
    const MAssembly* GetAssembly() const
    {
        return _assembly;
    }

    /// <summary>
    /// Gets the full name of the class (namespace and typename).
    /// </summary>
    FORCE_INLINE const MString& GetFullName() const
    {
        return _fullname;
    }

#if USE_MONO
    /// <summary>
    /// Gets the Mono class handle.
    /// </summary>
    FORCE_INLINE MonoClass* GetNative() const
    {
        return _monoClass;
    }
#endif

    /// <summary>
    /// Gets class visibility
    /// </summary>
    FORCE_INLINE MVisibility GetVisibility() const
    {
        return _visibility;
    }

    /// <summary>
    /// Gets if class is static
    /// </summary>
    FORCE_INLINE bool IsStatic() const
    {
        return _isStatic != 0;
    }

    /// <summary>
    /// Gets if class is abstract
    /// </summary>
    FORCE_INLINE bool IsAbstract() const
    {
        return _isAbstract != 0;
    }

    /// <summary>
    /// Gets if class is sealed
    /// </summary>
    FORCE_INLINE bool IsSealed() const
    {
        return _isSealed != 0;
    }

    /// <summary>
    /// Gets if class is interface
    /// </summary>
    FORCE_INLINE bool IsInterface() const
    {
        return _isInterface != 0;
    }

    /// <summary>
    /// Gets if class is generic
    /// </summary>
    bool IsGeneric() const;

    /// <summary>
    /// Gets the class type.
    /// </summary>
    MType GetType() const;

    /// <summary>
    /// Returns the base class of this class. Null if this class has no base.
    /// </summary>
    MClass* GetBaseClass() const;

    /// <summary>
    /// Checks if this class is a sub class of the specified class (including any derived types).
    /// </summary>
    /// <param name="klass">The class.</param>
    /// <returns>True if this class is a sub class of the specified class.</returns>
    bool IsSubClassOf(const MClass* klass) const;

#if USE_MONO
    /// <summary>
    /// Checks if this class is a sub class of the specified class (including any derived types).
    /// </summary>
    /// <param name="monoClass">The Mono class.</param>
    /// <returns>True if this class is a sub class of the specified class.</returns>
    bool IsSubClassOf(const MonoClass* monoClass) const;
#endif

    /// <summary>
    /// Checks if this class implements the specified interface (including any base types).
    /// </summary>
    /// <param name="klass">The interface class.</param>
    /// <returns>True if this class implements the specified interface.</returns>
    bool HasInterface(const MClass* klass) const;

    /// <summary>
    /// Checks is the provided object instance of this class' type.
    /// </summary>
    /// <param name="object">The object to check.</param>
    /// <returns>True if object  is an instance the this class.</returns>
    bool IsInstanceOfType(MObject* object) const;

    /// <summary>
    /// Returns the size of an instance of this class, in bytes.
    /// </summary>
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
    MObject* CreateInstance() const;

    /// <summary>
    /// Creates a new instance of this class and then constructs it using the constructor with the specified number of parameters.
    /// </summary>
    /// <param name="params">The array containing pointers to constructor parameters. Array length must be equal to number of parameters.</param>
    /// <param name="numParams">The number of parameters the constructor accepts.</param>
    /// <returns>The created managed object.</returns>
    MObject* CreateInstance(void** params, uint32 numParams);

public:

    /// <summary>
    /// Checks if class has an attribute of the specified type.
    /// </summary>
    /// <param name="monoClass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(const MClass* monoClass) const;

    /// <summary>
    /// Checks if class has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the class doesn't have such an attribute.
    /// </summary>
    /// <param name="monoClass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MObject* GetAttribute(const MClass* monoClass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given class. Returns null if the class doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MObject*>& GetAttributes();
};
