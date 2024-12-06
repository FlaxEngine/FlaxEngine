// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.Content;
using FlaxEngine.Utilities;

namespace FlaxEditor.Scripting
{
    /// <summary>
    /// Interface for custom scripting languages type object with metadata.
    /// </summary>
    public interface IScriptType
    {
        /// <summary>
        /// Gets a type name (eg. name of the class or enum without leading namespace).
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Gets a full namespace of the type.
        /// </summary>
        string Namespace { get; }

        /// <summary>
        /// Gets a full name of the type including leading namespace and any nested types names. Uniquely identifies the type and can be used to find it via <see cref="TypeUtils.GetType(string)"/>.
        /// </summary>
        string TypeName { get; }

        /// <summary>
        /// Gets a value indicating whether the type is declared public.
        /// </summary>
        bool IsPublic { get; }

        /// <summary>
        /// Gets a value indicating whether the type is abstract and must be overridden.
        /// </summary>
        bool IsAbstract { get; }

        /// <summary>
        /// Gets a value indicating whether the type is declared sealed and cannot be overridden.
        /// </summary>
        bool IsSealed { get; }

        /// <summary>
        /// Gets a value indicating whether the type represents an enumeration.
        /// </summary>
        bool IsEnum { get; }

        /// <summary>
        /// Gets a value indicating whether the type is a class or a delegate; that is, not a value type or interface.
        /// </summary>
        bool IsClass { get; }

        /// <summary>
        /// Gets a value indicating whether the type is an interface.
        /// </summary>
        bool IsInterface { get; }

        /// <summary>
        /// Gets a value indicating whether the type is an array.
        /// </summary>
        bool IsArray { get; }

        /// <summary>
        /// Gets a value indicating whether the type is a value type (basic type, enumeration or a structure).
        /// </summary>
        bool IsValueType { get; }

        /// <summary>
        /// Gets a value indicating whether the type is generic type and is used as a template.
        /// </summary>
        bool IsGenericType { get; }

        /// <summary>
        /// Gets a value indicating whether the type is a reference and value is represented by the valid pointer to the data.
        /// </summary>
        bool IsReference { get; }

        /// <summary>
        /// Gets a value indicating whether the type is a pointer and value is represented by the pointer to the data.
        /// </summary>
        bool IsPointer { get; }

        /// <summary>
        /// Gets a value indicating whether the type is static.
        /// </summary>
        bool IsStatic { get; }

        /// <summary>
        /// Gets a value indicating whether can create default instance of this type via <see cref="CreateInstance"/> method.
        /// </summary>
        bool CanCreateInstance { get; }

        /// <summary>
        /// Gets the type from which the current type directly inherits.
        /// </summary>
        ScriptType BaseType { get; }

        /// <summary>
        /// Gets the editor content item that corresponds to this script type. Can be null.
        /// </summary>
        ContentItem ContentItem { get; }

        /// <summary>
        /// Creates the instance of the object of this type (or throws exception in case of error).
        /// </summary>
        /// <returns>The created instance of the object.</returns>
        object CreateInstance();

        /// <summary>
        /// Determines whether the current type implements the specified interface type. Checks this type, its base classes and implemented interfaces base interfaces too.
        /// </summary>
        /// <param name="c">The type of the interface to check.</param>
        /// <returns>True if this type implements the given interface, otherwise false.</returns>
        bool HasInterface(ScriptType c);

        /// <summary>
        /// Determines whether the specified attribute was defined for this type.
        /// </summary>
        /// <param name="attributeType">The attribute type.</param>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns><c>true</c> if the specified type has attribute; otherwise, <c>false</c>.</returns>
        bool HasAttribute(Type attributeType, bool inherit);

        /// <summary>
        /// When overridden in a derived class, returns an array of all custom attributes applied to this member.
        /// </summary>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns>An array that contains all the custom attributes applied to this member, or an array with zero elements if no attributes are defined.</returns>
        object[] GetAttributes(bool inherit);

        /// <summary>
        /// Searches for the specified members of the specified member type, using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the members to get.</param>
        /// <param name="type">The value to search for.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero, to return an empty array.</param>
        /// <returns>An array of member objects representing the public members with the specified name, if found; otherwise, an empty array.</returns>
        ScriptMemberInfo[] GetMembers(string name, MemberTypes type, BindingFlags bindingAttr);

        /// <summary>
        /// When overridden in a derived class, searches for the members defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty array.</param>
        /// <returns>An array of member objects representing all members defined for the current type that match the specified binding constraints.-or- An empty array of type member, if no members are defined for the current type, or if none of the defined members match the binding constraints.</returns>
        ScriptMemberInfo[] GetMembers(BindingFlags bindingAttr);

        /// <summary>
        /// When overridden in a derived class, searches for the fields defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty array.</param>
        /// <returns>An array of member objects representing all fields defined for the current type that match the specified binding constraints.-or- An empty array of type member, if no fields are defined for the current type, or if none of the defined fields match the binding constraints.</returns>
        ScriptMemberInfo[] GetFields(BindingFlags bindingAttr);

        /// <summary>
        /// When overridden in a derived class, searches for the properties defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty array.</param>
        /// <returns>An array of member objects representing all properties defined for the current type that match the specified binding constraints.-or- An empty array of type member, if no properties are defined for the current type, or if none of the defined properties match the binding constraints.</returns>
        ScriptMemberInfo[] GetProperties(BindingFlags bindingAttr);

        /// <summary>
        /// When overridden in a derived class, searches for the methods defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty array.</param>
        /// <returns>An array of member objects representing all methods defined for the current type that match the specified binding constraints.-or- An empty array of type member, if no methods are defined for the current type, or if none of the defined methods match the binding constraints.</returns>
        ScriptMemberInfo[] GetMethods(BindingFlags bindingAttr);

        /// <summary>
        /// Registers delegate to be invoked upon script type disposal (except hot-reload in Editor via <see cref="ScriptsBuilder.ScriptsReload"/>). For example, can happen when user deleted Visual Script asset.
        /// </summary>
        /// <param name="disposing">Event to call when script type gets disposed (eg. removed asset).</param>
        void TrackLifetime(Action<ScriptType> disposing);
    }

    /// <summary>
    /// Interface for custom scripting languages type object with metadata.
    /// </summary>
    public interface IScriptMemberInfo
    {
        /// <summary>
        /// Gets a member name (eg. name of the field or method without leading class name nor namespace prefixes).
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Gets a metadata token for sorting so it may not be the actual token.
        /// </summary>
        int MetadataToken { get; }

        /// <summary>
        /// Gets a value indicating whether the type is declared public.
        /// </summary>
        bool IsPublic { get; }

        /// <summary>
        /// Gets a value indicating whether the type is declared in static scope.
        /// </summary>
        bool IsStatic { get; }

        /// <summary>
        /// Gets a value indicating whether this method is declared as virtual (can be overriden).
        /// </summary>
        bool IsVirtual { get; }

        /// <summary>
        /// Gets a value indicating whether this method is declared as abstract (has to be overriden).
        /// </summary>
        bool IsAbstract { get; }

        /// <summary>
        /// Gets a value indicating whether this method is declared as generic (needs to be inflated with type arguments).
        /// </summary>
        bool IsGeneric { get; }

        /// <summary>
        /// Gets a value indicating whether this member is a field.
        /// </summary>
        bool IsField { get; }

        /// <summary>
        /// Gets a value indicating whether this member is a property.
        /// </summary>
        bool IsProperty { get; }

        /// <summary>
        /// Gets a value indicating whether this member is a method.
        /// </summary>
        bool IsMethod { get; }

        /// <summary>
        /// Gets a value indicating whether this member is an event.
        /// </summary>
        bool IsEvent { get; }

        /// <summary>
        /// Gets a value indicating whether this member value can be gathered (via getter method or directly from the field).
        /// </summary>
        bool HasGet { get; }

        /// <summary>
        /// Gets a value indicating whether this member value can be set (via setter method or directly to the field).
        /// </summary>
        bool HasSet { get; }

        /// <summary>
        /// Gets the method parameters count (valid for methods only).
        /// </summary>
        int ParametersCount { get; }

        /// <summary>
        /// Gets the type that declares this member.
        /// </summary>
        ScriptType DeclaringType { get; }

        /// <summary>
        /// Gets the type of the value (field type, property type or method return value).
        /// </summary>
        ScriptType ValueType { get; }

        /// <summary>
        /// Determines whether the specified attribute was defined for this member.
        /// </summary>
        /// <param name="attributeType">The attribute type.</param>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns><c>true</c> if the specified member has attribute; otherwise, <c>false</c>.</returns>
        bool HasAttribute(Type attributeType, bool inherit);

        /// <summary>
        /// When overridden in a derived class, returns an array of all custom attributes applied to this member.
        /// </summary>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns>An array that contains all the custom attributes applied to this member, or an array with zero elements if no attributes are defined.</returns>
        object[] GetAttributes(bool inherit);

        /// <summary>
        /// Gets the method parameters metadata (or event delegate signature parameters).
        /// </summary>
        ScriptMemberInfo.Parameter[] GetParameters();

        /// <summary>
        /// Returns the member value of a specified object.
        /// </summary>
        /// <param name="obj">The object whose member value will be returned.</param>
        /// <returns>The member value of the specified object.</returns>
        object GetValue(object obj);

        /// <summary>
        /// Sets the member value of a specified object.
        /// </summary>
        /// <param name="obj">The object whose member value will be modified.</param>
        /// <param name="value">The new member value.</param>
        void SetValue(object obj, object value);

        /// <summary>
        /// Invokes the method on a specific object (null if static) using the provided parameters.
        /// </summary>
        /// <param name="obj">The instance of the object to invoke its method. Use null for static methods.</param>
        /// <param name="parameters">List of parameters to provide.</param>
        /// <returns>The value returned by the method.</returns>
        object Invoke(object obj, object[] parameters);
    }

    /// <summary>
    /// Interface for custom scripting languages types information containers that can inject types metadata into Editor.
    /// </summary>
    public interface IScriptTypesContainer
    {
        /// <summary>
        /// Tries to get the object type from the given full typename. Searches in-build Flax Engine/Editor assemblies and game assemblies.
        /// </summary>
        /// <param name="typeName">The full name of the type.</param>
        /// <returns>The type or null if failed.</returns>
        ScriptType GetType(string typeName);

        /// <summary>
        /// Gets all the types within all the loaded assemblies.
        /// </summary>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        void GetTypes(List<ScriptType> result, Func<ScriptType, bool> checkFunc);

        /// <summary>
        /// Gets all the derived types from the given base type (excluding that type) within all the loaded assemblies.
        /// </summary>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        void GetDerivedTypes(ScriptType baseType, List<ScriptType> result, Func<ScriptType, bool> checkFunc);
    }
}
