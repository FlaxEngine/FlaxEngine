// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "MTypes.h"

/// <summary>
/// Contains information about a single managed class.
/// </summary>
class FLAXENGINE_API MClass
{
    friend MCore;
private:
#if USE_MONO
    MonoClass* _monoClass;
    mutable void* _attrInfo = nullptr;
#elif USE_NETCORE
    void* _handle;
    StringAnsi _name;
    StringAnsi _namespace;
    uint32 _types = 0;
    mutable uint32 _size = 0;
#endif
    const MAssembly* _assembly;

    StringAnsi _fullname;

    mutable Array<MMethod*> _methods;
    mutable Array<MField*> _fields;
    mutable Array<MProperty*> _properties;
    mutable Array<MObject*> _attributes;
    mutable Array<MEvent*> _events;
    mutable Array<MClass*> _interfaces;

    MVisibility _visibility;

    mutable int32 _hasCachedProperties : 1;
    mutable int32 _hasCachedFields : 1;
    mutable int32 _hasCachedMethods : 1;
    mutable int32 _hasCachedAttributes : 1;
    mutable int32 _hasCachedEvents : 1;
    mutable int32 _hasCachedInterfaces : 1;
    int32 _isStatic : 1;
    int32 _isSealed : 1;
    int32 _isAbstract : 1;
    int32 _isInterface : 1;
    int32 _isValueType : 1;
    int32 _isEnum : 1;

public:
#if USE_MONO
    MClass(const MAssembly* parentAssembly, MonoClass* monoClass, const StringAnsi& fullname);
#elif USE_NETCORE
    MClass(const MAssembly* parentAssembly, void* handle, const char* name, const char* fullname, const char* namespace_, MTypeAttributes typeAttributes);
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
    FORCE_INLINE const StringAnsi& GetFullName() const
    {
        return _fullname;
    }

    /// <summary>
    /// Gets the name of the class.
    /// </summary>
    StringAnsiView GetName() const;

    /// <summary>
    /// Gets the namespace of the class.
    /// </summary>
    StringAnsiView GetNamespace() const;

#if USE_MONO
    /// <summary>
    /// Gets the Mono class handle.
    /// </summary>
    FORCE_INLINE MonoClass* GetNative() const
    {
        return _monoClass;
    }
#elif USE_NETCORE
    FORCE_INLINE void* GetNative() const
    {
        return _handle;
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
    /// Gets if class is value type (eg. enum or structure) but not reference type (eg. class, string, array, interface)
    /// </summary>
    FORCE_INLINE bool IsValueType() const
    {
        return _isValueType != 0;
    }

    /// <summary>
    /// Gets if class is enumeration
    /// </summary>
    FORCE_INLINE bool IsEnum() const
    {
        return _isEnum != 0;
    }

    /// <summary>
    /// Gets if class is generic
    /// </summary>
    bool IsGeneric() const
    {
        return _fullname.FindLast('`') != -1;
    }

    /// <summary>
    /// Gets the class type.
    /// </summary>
    MType* GetType() const;

    /// <summary>
    /// Returns the base class of this class. Null if this class has no base.
    /// </summary>
    MClass* GetBaseClass() const;

    /// <summary>
    /// Checks if this class is a sub class of the specified class (including any derived types).
    /// </summary>
    /// <param name="klass">The class.</param>
    /// <param name="checkInterfaces">True if check interfaces, otherwise just base class.</param>
    /// <returns>True if this class is a sub class of the specified class.</returns>
    bool IsSubClassOf(const MClass* klass, bool checkInterfaces = false) const;

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

    /// <summary>
    /// Returns the class of the array type elements.
    /// </summary>
    MClass* GetElementClass() const;

public:
    /// <summary>
    /// Returns an object referencing a method with the specified name and number of parameters. Optionally checks the base classes.
    /// </summary>
    /// <param name="name">The method name.</param>
    /// <param name="numParams">The method parameters count.</param>
    /// <param name="checkBaseClasses">True if check base classes when searching for the given method.</param>
    /// <returns>The method or null if failed to find it.</returns>
    MMethod* FindMethod(const char* name, int32 numParams, bool checkBaseClasses = true) const
    {
        MMethod* method = GetMethod(name, numParams);
        if (!method && checkBaseClasses)
        {
            MClass* base = GetBaseClass();
            if (base)
                method = base->FindMethod(name, numParams, true);
        }
        return method;
    }

    /// <summary>
    /// Returns an object referencing a method with the specified name and number of parameters.
    /// </summary>
    /// <remarks>If the type contains more than one method of the given name and parameters count the returned value can be non-deterministic (one of the matching methods).</remarks>
    /// <param name="name">The method name.</param>
    /// <param name="numParams">The method parameters count.</param>
    /// <returns>The method or null if failed to get it.</returns>
    MMethod* GetMethod(const char* name, int32 numParams = 0) const;

    /// <summary>
    /// Returns all methods belonging to this class.
    /// </summary>
    /// <remarks>Be aware this will not include the methods of any base classes.</remarks>
    /// <returns>The list of methods.</returns>
    const Array<MMethod*>& GetMethods() const;

    /// <summary>
    /// Returns an object referencing a field with the specified name.
    /// </summary>
    /// <remarks>Does not query base class fields. Returns null if field cannot be found.</remarks>
    /// <param name="name">The field name.</param>
    /// <returns>The field or null if failed.</returns>
    MField* GetField(const char* name) const;

    /// <summary>
    /// Returns all fields belonging to this class.
    /// </summary>
    /// <remarks>Be aware this will not include the fields of any base classes.</remarks>
    /// <returns>The list of fields.</returns>
    const Array<MField*>& GetFields() const;

    /// <summary>
    /// Returns an object referencing a event with the specified name.
    /// </summary>
    /// <param name="name">The event name.</param>
    /// <returns>The event object.</returns>
    MEvent* GetEvent(const char* name) const;

    /// <summary>
    /// Returns all events belonging to this class.
    /// </summary>
    /// <returns>The list of events.</returns>
    const Array<MEvent*>& GetEvents() const;

    /// <summary>
    /// Returns an object referencing a property with the specified name.
    /// </summary>
    /// <remarks>Does not query base class properties. Returns null if property cannot be found.</remarks>
    /// <param name="name">The property name.</param>
    /// <returns>The property.</returns>
    MProperty* GetProperty(const char* name) const;

    /// <summary>
    /// Returns all properties belonging to this class.
    /// </summary>
    /// <remarks>Be aware this will not include the properties of any base classes.</remarks>
    /// <returns>The list of properties.</returns>
    const Array<MProperty*>& GetProperties() const;

    /// <summary>
    /// Returns all interfaces implemented by this class (excluding interfaces from base classes).
    /// </summary>
    /// <remarks>Be aware this will not include the interfaces of any base classes.</remarks>
    /// <returns>The list of interfaces.</returns>
    const Array<MClass*>& GetInterfaces() const;

public:
    /// <summary>
    /// Creates a new instance of this class and constructs it.
    /// </summary>
    /// <returns>The created managed object.</returns>
    MObject* CreateInstance() const;

public:
    /// <summary>
    /// Checks if class has an attribute of the specified type.
    /// </summary>
    /// <param name="klass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(const MClass* klass) const;

    /// <summary>
    /// Checks if class has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the class doesn't have such an attribute.
    /// </summary>
    /// <param name="klass">The attribute class to take.</param>
    /// <returns>The attribute object.</returns>
    MObject* GetAttribute(const MClass* klass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given class. Returns null if the class doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MObject*>& GetAttributes() const;
};
