// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using FlaxEditor.Content;
using FlaxEngine;

namespace FlaxEditor.Scripting
{
    /// <summary>
    /// The structure that wraps the script type member info with support for native C++ scripts, managed C# scripts, Visual Scripts and custom scripts.
    /// </summary>
    [HideInEditor]
    public readonly struct ScriptMemberInfo : IEquatable<ScriptMemberInfo>
    {
        private readonly MemberInfo _managed;
        private readonly IScriptMemberInfo _custom;

        /// <summary>
        /// A <see cref="ScriptMemberInfo" /> that is null (invalid).
        /// </summary>
        public static readonly ScriptMemberInfo Null;

        /// <summary>
        /// Gets the type of the member as <see cref="System.Type"/>.
        /// </summary>
        public MemberInfo Type => _managed;

        /// <summary>
        /// Gets the type of the member as <see cref="FlaxEditor.Scripting.IScriptMemberInfo"/>.
        /// </summary>
        public IScriptMemberInfo IScriptMemberInfo => _custom;

        /// <summary>
        /// Gets a member name (eg. name of the field or method without leading class name nor namespace prefixes).
        /// </summary>
        public string Name => _managed?.Name ?? _custom?.Name;

        /// <summary>
        /// Gets a value indicating whether the type is declared public.
        /// </summary>
        public bool IsPublic
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return methodInfo.IsPublic;
                if (_managed is FieldInfo fieldInfo)
                    return fieldInfo.IsPublic;
                if (_managed is PropertyInfo propertyInfo)
                    return (propertyInfo.GetMethod == null || propertyInfo.GetMethod.IsPublic) && (propertyInfo.SetMethod == null || propertyInfo.SetMethod.IsPublic);
                if (_managed is EventInfo eventInfo)
                    return eventInfo.GetAddMethod().IsPublic;
                if (_custom != null)
                    return _custom.IsPublic;
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the type is declared in static scope.
        /// </summary>
        public bool IsStatic
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return methodInfo.IsStatic;
                if (_managed is FieldInfo fieldInfo)
                    return fieldInfo.IsStatic;
                if (_managed is PropertyInfo propertyInfo)
                    return (propertyInfo.GetMethod == null || propertyInfo.GetMethod.IsStatic) && (propertyInfo.SetMethod == null || propertyInfo.SetMethod.IsStatic);
                if (_managed is EventInfo eventInfo)
                    return eventInfo.GetAddMethod().IsStatic;
                if (_custom != null)
                    return _custom.IsStatic;
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this method is declared as virtual (can be overriden).
        /// </summary>
        public bool IsVirtual
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return methodInfo.IsVirtual;
                if (_custom != null)
                    return _custom.IsVirtual;
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this method is declared as abstract (has to be overriden).
        /// </summary>
        public bool IsAbstract
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return methodInfo.IsAbstract;
                if (_custom != null)
                    return _custom.IsAbstract;
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this method is declared as generic (needs to be inflated with type arguments).
        /// </summary>
        public bool IsGeneric
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return methodInfo.IsGenericMethod;
                if (_custom != null)
                    return _custom.IsGeneric;
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this member is a field.
        /// </summary>
        public bool IsField => _managed is FieldInfo || (_custom?.IsField ?? false);

        /// <summary>
        /// Gets a value indicating whether this member is a property.
        /// </summary>
        public bool IsProperty => _managed is PropertyInfo || (_custom?.IsProperty ?? false);

        /// <summary>
        /// Gets a value indicating whether this member is a method.
        /// </summary>
        public bool IsMethod => _managed is MethodInfo || (_custom?.IsMethod ?? false);

        /// <summary>
        /// Gets a value indicating whether this member is an event.
        /// </summary>
        public bool IsEvent => _managed is EventInfo || (_custom?.IsEvent ?? false);

        /// <summary>
        /// Gets a value indicating whether this member value can be gathered (via getter method or directly from the field).
        /// </summary>
        public bool HasGet
        {
            get
            {
                if (_managed is PropertyInfo propertyInfo)
                    return propertyInfo.GetMethod != null;
                if (_managed is FieldInfo)
                    return true;
                if (_custom != null)
                    return _custom.HasGet;
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this member value can be set (via setter method or directly to the field).
        /// </summary>
        public bool HasSet
        {
            get
            {
                if (_managed is PropertyInfo propertyInfo)
                    return propertyInfo.SetMethod != null;
                if (_managed is FieldInfo fieldInfo)
                    return !fieldInfo.IsInitOnly;
                if (_custom != null)
                    return _custom.HasSet;
                return false;
            }
        }

        /// <summary>
        /// Gets the method parameters count (valid for methods and events only).
        /// </summary>
        public int ParametersCount
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return methodInfo.GetParameters().Length;
                if (_managed is EventInfo eventInfo)
                    return eventInfo.EventHandlerType.GetMethod("Invoke").GetParameters().Length;
                if (_custom != null)
                    return _custom.ParametersCount;
                return 0;
            }
        }

        /// <summary>
        /// Gets the type that declares this member.
        /// </summary>
        public ScriptType DeclaringType
        {
            get
            {
                if (_managed != null)
                    return new ScriptType(_managed.DeclaringType);
                if (_custom != null)
                    return _custom.DeclaringType;
                return ScriptType.Null;
            }
        }

        /// <summary>
        /// Gets the type of the value (field type, property type or method return value).
        /// </summary>
        public ScriptType ValueType
        {
            get
            {
                if (_managed is MethodInfo methodInfo)
                    return new ScriptType(methodInfo.ReturnType);
                if (_managed is FieldInfo fieldInfo)
                    return new ScriptType(fieldInfo.FieldType);
                if (_managed is PropertyInfo propertyInfo)
                    return new ScriptType(propertyInfo.PropertyType);
                if (_custom != null)
                    return _custom.ValueType;
                return ScriptType.Null;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptMemberInfo"/> struct.
        /// </summary>
        /// <param name="type">The type.</param>
        public ScriptMemberInfo(MemberInfo type)
        {
            _managed = type;
            _custom = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptMemberInfo"/> struct.
        /// </summary>
        /// <param name="type">The type.</param>
        public ScriptMemberInfo(IScriptMemberInfo type)
        {
            _managed = null;
            _custom = type;
        }

        /// <summary>
        /// Checks the script member whether it matches a given.
        /// </summary>
        /// <param name="bindingAttr">The binding attribute.</param>
        /// <returns>True if member passed the filter (matches), otherwise false.</returns>
        public bool Filter(BindingFlags bindingAttr)
        {
            if ((bindingAttr & BindingFlags.Instance) == 0 && !IsStatic)
                return false;
            if ((bindingAttr & BindingFlags.Static) == 0 && IsStatic)
                return false;
            if ((bindingAttr & BindingFlags.Public) == 0 && IsPublic)
                return false;
            if ((bindingAttr & BindingFlags.NonPublic) == 0 && !IsPublic)
                return false;
            return true;
        }

        /// <summary>
        /// Checks the script member whether it matches a given.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="bindingAttr">The binding attribute.</param>
        /// <returns>True if member passed the filter (matches), otherwise false.</returns>
        public bool Filter(string name, BindingFlags bindingAttr)
        {
            if (!Filter(bindingAttr))
                return false;
            if ((bindingAttr & BindingFlags.IgnoreCase) == 0 && !string.Equals(name, Name, StringComparison.Ordinal))
                return false;
            if ((bindingAttr & BindingFlags.IgnoreCase) != 0 && !string.Equals(name, Name, StringComparison.OrdinalIgnoreCase))
                return false;
            return true;
        }

        /// <summary>
        /// The method parameter info.
        /// </summary>
        public struct Parameter
        {
            /// <summary>
            /// The parameter name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The parameter type.
            /// </summary>
            public ScriptType Type;

            /// <summary>
            /// Gets a value that indicates whether this parameter is returned by reference (output parameter).
            /// </summary>
            public bool IsOut;

            /// <summary>
            /// Gets a value that indicates whether this parameter has a default value.
            /// </summary>
            public bool HasDefaultValue;

            /// <summary>
            /// The default value for the parameter.
            /// </summary>
            public object DefaultValue;

            /// <summary>
            /// The parameter attributes collection (optional, null if unused).
            /// </summary>
            public object[] Attributes;

            /// <summary>
            /// Formats the parameters to the string representation for UI.
            /// </summary>
            /// <param name="sb">The output String Builder to write to.</param>
            public void ToString(StringBuilder sb)
            {
                if (IsOut)
                    sb.Append("out ");

                if (Type.Type == typeof(float))
                    sb.Append("Float");
                else if (Type.Type == typeof(int))
                    sb.Append("Int");
                else if (Type.Type == typeof(uint))
                    sb.Append("Uint");
                else if (Type.Type == typeof(bool))
                    sb.Append("Bool");
                else
                    sb.Append(Type.Name);

                sb.Append(' ');
                sb.Append(Name);

                if (HasDefaultValue)
                {
                    sb.Append(' ');
                    sb.Append('=');
                    sb.Append(' ');
                    if (DefaultValue is int asInt)
                    {
                        if (asInt == int.MaxValue)
                            sb.Append("Int.MaxValue");
                        else if (asInt == int.MinValue)
                            sb.Append("Int.MinValue");
                        else
                            sb.Append(asInt);
                    }
                    else if (DefaultValue is uint asUint)
                    {
                        if (asUint == uint.MaxValue)
                            sb.Append("Uint.MaxValue");
                        else if (asUint == uint.MinValue)
                            sb.Append("Uint.MinValue");
                        else
                            sb.Append(asUint);
                    }
                    else if (DefaultValue is float asFloat)
                    {
                        if (asFloat == float.MaxValue)
                            sb.Append("Float.MaxValue");
                        else if (asFloat == float.MinValue)
                            sb.Append("Float.MinValue");
                        else
                            sb.Append(asFloat);
                    }
                    else
                    {
                        sb.Append(DefaultValue?.ToString() ?? "null");
                    }
                }
            }

            /// <inheritdoc />
            public override string ToString()
            {
                var sb = new StringBuilder();
                ToString(sb);
                return sb.ToString();
            }
        }

        /// <summary>
        /// Gets the method parameters metadata (or event delegate signature parameters).
        /// </summary>
        public Parameter[] GetParameters()
        {
            if (_managed is MethodInfo methodInfo)
            {
                var parameters = methodInfo.GetParameters();
                if (parameters.Length == 0)
                    return Utils.GetEmptyArray<Parameter>();
                var result = new Parameter[parameters.Length];
                for (int i = 0; i < result.Length; i++)
                {
                    var parameter = parameters[i];
                    var p = new Parameter
                    {
                        Name = parameter.Name,
                        Type = new ScriptType(parameter.ParameterType),
                        IsOut = parameter.IsOut,
                        HasDefaultValue = parameter.HasDefaultValue,
                    };
                    if (p.HasDefaultValue)
                        p.DefaultValue = parameter.DefaultValue;
                    var attributes = parameter.GetCustomAttributes();
                    if (attributes is Attribute[] asArray)
                        p.Attributes = asArray;
                    else if (attributes.Count() != 0)
                        p.Attributes = attributes.ToArray();
                    else
                        p.Attributes = null;
                    result[i] = p;
                }
                return result;
            }
            if (_managed is EventInfo eventInfo)
            {
                var invokeMethod = eventInfo.EventHandlerType.GetMethod("Invoke");
                return new ScriptMemberInfo(invokeMethod).GetParameters();
            }
            return _custom.GetParameters();
        }

        /// <summary>
        /// Checks if the type is not null.
        /// </summary>
        /// <param name="type">The type to check.</param>
        /// <returns>True if type is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(ScriptMemberInfo type)
        {
            return type._managed != null || type._custom != null;
        }

        /// <summary>
        /// Implements the equality operator.
        /// </summary>
        /// <param name="a">The left operand.</param>
        /// <param name="b">The right operand.</param>
        /// <returns>True if both types are equal, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(ScriptMemberInfo a, ScriptMemberInfo b)
        {
            return EqualityComparer<MemberInfo>.Default.Equals(a._managed, b._managed) && a._custom == b._custom;
        }

        /// <summary>
        /// Implements the non equality operator.
        /// </summary>
        /// <param name="a">The left operand.</param>
        /// <param name="b">The right operand.</param>
        /// <returns>True if both types are not equal, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(ScriptMemberInfo a, ScriptMemberInfo b)
        {
            return !EqualityComparer<MemberInfo>.Default.Equals(a._managed, b._managed) || a._custom != b._custom;
        }

        /// <inheritdoc />
        public bool Equals(ScriptMemberInfo other)
        {
            return EqualityComparer<MemberInfo>.Default.Equals(_managed, other._managed) && _custom == other._custom;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is ScriptMemberInfo other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return ((_managed != null ? EqualityComparer<MemberInfo>.Default.GetHashCode(_managed) : 0) * 397) ^ (_custom != null ? _custom.GetHashCode() : 0);
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            if (_managed != null)
                return _managed.Name;
            if (_custom != null)
                return _custom.Name;
            return "<null>";
        }

        /// <summary>
        /// Determines whether the specified attribute was defined for this type.
        /// </summary>
        /// <param name="attributeType">The attribute type.</param>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns><c>true</c> if the specified type has attribute; otherwise, <c>false</c>.</returns>
        public bool HasAttribute(Type attributeType, bool inherit = false)
        {
            if (_managed != null)
                return Attribute.IsDefined(_managed, attributeType, inherit);
            if (_custom != null)
                return _custom.HasAttribute(attributeType, inherit);
            return false;
        }

        /// <summary>
        /// When overridden in a derived class, returns an array of all custom attributes applied to this member.
        /// </summary>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns>An array that contains all the custom attributes applied to this member, or an array with zero elements if no attributes are defined.</returns>
        public object[] GetAttributes(bool inherit = false)
        {
            if (_managed != null)
            {
                if (_managed is MethodInfo method)
                {
                    try
                    {
                        // Special case for properties getter/setter methods to use the property attributes instead own
                        var name = _managed.Name;
                        if (name.StartsWith("get_") || name.StartsWith("set_"))
                        {
                            var flags = method.IsStatic ? BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly : BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;
                            var property = method.DeclaringType.GetProperty(name.Substring(4), flags);
                            if (property != null)
                                return property.GetCustomAttributes(inherit);
                        }
                    }
                    catch
                    {
                        // Silence exceptions
                    }
                }
                return _managed.GetCustomAttributes(inherit);
            }
            if (_custom != null)
                return _custom.GetAttributes(inherit);
            return Utils.GetEmptyArray<object>();
        }

        /// <summary>
        /// Retrieves a custom attribute of a specified type that is applied to a specified member.
        /// </summary>
        /// <param name="type">The type of attribute to search for.</param>
        /// <returns>A custom attribute that matches type, or <see langword="null" /> if no such attribute is found.</returns>
        public object GetAttribute(Type type)
        {
            var attr = GetAttributes(true);
            for (int i = 0; i < attr.Length; i++)
            {
                if (attr[i].GetType() == type)
                    return attr[i];
            }
            return null;
        }

        /// <summary>
        /// Retrieves a custom attribute of a specified type that is applied to a specified member.
        /// </summary>
        /// <typeparam name="T">The type of attribute to search for.</typeparam>
        /// <returns>A custom attribute that matches type, or <see langword="null" /> if no such attribute is found.</returns>
        public T GetAttribute<T>() where T : Attribute => (T)GetAttribute(typeof(T));

        /// <summary>
        /// Gets the full member signature name (can be displayed in UI).
        /// </summary>
        /// <returns>The full signature of the method including access level, type, name and method parameters.</returns>
        public string GetSignature()
        {
            var sb = new StringBuilder();
            if (IsPublic)
                sb.Append("public ");
            if (IsStatic)
                sb.Append("static ");
            else if (IsVirtual)
                sb.Append("virtual ");
            sb.Append(ValueType.Name);
            sb.Append(' ');
            sb.Append(Name);
            if (IsMethod)
            {
                sb.Append('(');
                var parameters = GetParameters();
                for (int i = 0; i < parameters.Length; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    ref var param = ref parameters[i];
                    param.ToString(sb);
                }
                sb.Append(')');
            }
            return sb.ToString();
        }

        /// <summary>
        /// Gets the member value of a specified object.
        /// </summary>
        /// <param name="obj">The object whose member value will be returned.</param>
        /// <returns>The member value of the specified object.</returns>
        public object GetValue(object obj)
        {
            if (_managed is PropertyInfo propertyInfo)
                return propertyInfo.GetValue(obj, null);
            if (_managed is FieldInfo fieldInfo)
                return fieldInfo.GetValue(obj);
            return _custom.GetValue(obj);
        }

        /// <summary>
        /// Sets the member value of a specified object.
        /// </summary>
        /// <param name="obj">The object whose member value will be modified.</param>
        /// <param name="value">The new member value.</param>
        public void SetValue(object obj, object value)
        {
            if (_managed is PropertyInfo propertyInfo)
                propertyInfo.SetValue(obj, value);
            else if (_managed is FieldInfo fieldInfo)
                fieldInfo.SetValue(obj, value);
            else
                _custom.SetValue(obj, value);
        }
    }

    /// <summary>
    /// The structure that wraps the script type with support for native C++ scripts, managed C# scripts, Visual Scripts and custom scripts.
    /// </summary>
    [HideInEditor]
    public readonly struct ScriptType : IEquatable<ScriptType>
    {
        private readonly Type _managed;
        private readonly IScriptType _custom;

        /// <summary>
        /// A <see cref="ScriptType" /> that is null (invalid).
        /// </summary>
        public static readonly ScriptType Null;

        /// <summary>
        /// A <see cref="ScriptType" /> that is System.Void.
        /// </summary>
        public static readonly ScriptType Void = new ScriptType(typeof(void));

        /// <summary>
        /// Gets the type of the script as <see cref="System.Type"/>.
        /// </summary>
        public Type Type => _managed;

        /// <summary>
        /// Gets the type of the script as <see cref="FlaxEditor.Scripting.IScriptType"/>.
        /// </summary>
        public IScriptType IScriptType => _custom;

        /// <summary>
        /// Gets a type name (eg. name of the class or enum without leading namespace).
        /// </summary>
        public string Name
        {
            get
            {
                if (_custom != null)
                    return _custom.Name;
                if (_managed == null)
                    return string.Empty;
                if (_managed == typeof(float))
                    return "Float";
                if (_managed == typeof(int))
                    return "Int";
                if (_managed == typeof(uint))
                    return "Uint";
                if (_managed == typeof(bool))
                    return "Bool";
                return _managed.Name;
            }
        }

        /// <summary>
        /// Gets a full namespace of the type.
        /// </summary>
        public string Namespace => _managed?.Namespace ?? _custom?.Namespace;

        /// <summary>
        /// Gets a full name of the type including leading namespace and any nested types names. Uniquely identifies the type and can be used to find it via <see cref="TypeUtils.GetType(string)"/>.
        /// </summary>
        public string TypeName => _managed?.FullName ?? _custom?.TypeName;

        /// <summary>
        /// Gets a value indicating whether the type is declared public.
        /// </summary>
        public bool IsPublic => _managed != null ? _managed.IsPublic : _custom != null && _custom.IsPublic;

        /// <summary>
        /// Gets a value indicating whether the type is abstract and must be overridden.
        /// </summary>
        public bool IsAbstract => _managed != null ? _managed.IsAbstract && !_managed.IsSealed : _custom != null && _custom.IsAbstract;

        /// <summary>
        /// Gets a value indicating whether the type is declared sealed and cannot be overridden.
        /// </summary>
        public bool IsSealed => _managed != null ? _managed.IsSealed : _custom != null && _custom.IsSealed;

        /// <summary>
        /// Gets a value indicating whether the type represents a primitive type (eg. bool, int, float).
        /// </summary>
        public bool IsPrimitive => _managed != null && _managed.IsPrimitive;

        /// <summary>
        /// Gets a value indicating whether the type represents an enumeration.
        /// </summary>
        public bool IsEnum => _managed != null ? _managed.IsEnum : _custom != null && _custom.IsEnum;

        /// <summary>
        /// Gets a value indicating whether the type is a class or a delegate; that is, not a value type or interface.
        /// </summary>
        public bool IsClass => _managed != null ? _managed.IsClass : _custom != null && _custom.IsClass;

        /// <summary>
        /// Gets a value indicating whether the type is an array.
        /// </summary>
        public bool IsArray => _managed != null ? _managed.IsArray : _custom != null && _custom.IsArray;

        /// <summary>
        /// Gets a value indicating whether the type is a value type (basic type, enumeration or a structure).
        /// </summary>
        public bool IsValueType => _managed != null ? _managed.IsValueType : _custom != null && _custom.IsValueType;

        /// <summary>
        /// Gets a value indicating whether the type is a structure (value type but not enum nor primitive type).
        /// </summary>
        public bool IsStructure => IsValueType && !IsEnum && !IsPrimitive;

        /// <summary>
        /// Gets a value indicating whether the type is generic type and is used as a template.
        /// </summary>
        public bool IsGenericType => _managed != null ? _managed.IsGenericType : _custom != null && _custom.IsGenericType;

        /// <summary>
        /// Gets a value indicating whether the type is a reference and value is represented by the valid pointer to the data.
        /// </summary>
        public bool IsReference => _managed != null ? _managed.IsByRef : _custom != null && _custom.IsReference;

        /// <summary>
        /// Gets a value indicating whether the type is a pointer and value is represented by the pointer to the data.
        /// </summary>
        public bool IsPointer => _managed != null ? _managed.IsPointer : _custom != null && _custom.IsPointer;

        /// <summary>
        /// Gets a value indicating whether this type is void.
        /// </summary>
        public bool IsVoid => _managed == typeof(void);

        /// <summary>
        /// Gets a value indicating whether the type is static.
        /// </summary>
        public bool IsStatic => _managed != null ? _managed.IsSealed && _managed.IsAbstract : _custom != null && _custom.IsStatic;

        /// <summary>
        /// Gets a value indicating whether can create default instance of this type via <see cref="CreateInstance"/> method.
        /// </summary>
        public bool CanCreateInstance
        {
            get
            {
                if (_managed != null)
                    return _managed.GetConstructor(Type.EmptyTypes) != null;
                return _custom.CanCreateInstance;
            }
        }

        /// <summary>
        /// Gets the type from which the current type directly inherits.
        /// </summary>
        public ScriptType BaseType
        {
            get
            {
                if (_managed != null)
                    return new ScriptType(_managed.BaseType);
                if (_custom != null)
                    return _custom.BaseType;
                return Null;
            }
        }

        /// <summary>
        /// Gets the editor content item that corresponds to this script type. Can be null.
        /// </summary>
        public ContentItem ContentItem
        {
            get
            {
                if (_managed != null)
                    return Editor.Instance.ContentDatabase.FindScriptWitScriptName(this);
                return _custom?.ContentItem;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptType"/> struct.
        /// </summary>
        /// <param name="type">The type.</param>
        public ScriptType(Type type)
        {
            _managed = type;
            _custom = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptType"/> struct.
        /// </summary>
        /// <param name="type">The type.</param>
        public ScriptType(IScriptType type)
        {
            _managed = null;
            _custom = type;
        }

        /// <summary>
        /// Creates the instance of the object of this type (or throws exception in case of error).
        /// </summary>
        /// <returns>The created instance of the object.</returns>
        public object CreateInstance()
        {
            if (_managed != null)
                return Activator.CreateInstance(_managed);
            return _custom.CreateInstance();
        }

        /// <summary>
        /// Checks if the type is not null.
        /// </summary>
        /// <param name="type">The type to check.</param>
        /// <returns>True if type is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(ScriptType type)
        {
            return type._managed != null || type._custom != null;
        }

        /// <summary>
        /// Implements the equality operator.
        /// </summary>
        /// <param name="a">The left operand.</param>
        /// <param name="b">The right operand.</param>
        /// <returns>True if both types are equal, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(ScriptType a, ScriptType b)
        {
            return EqualityComparer<MemberInfo>.Default.Equals(a._managed, b._managed) && a._custom == b._custom;
        }

        /// <summary>
        /// Implements the non equality operator.
        /// </summary>
        /// <param name="a">The left operand.</param>
        /// <param name="b">The right operand.</param>
        /// <returns>True if both types are not equal, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(ScriptType a, ScriptType b)
        {
            return !EqualityComparer<MemberInfo>.Default.Equals(a._managed, b._managed) || a._custom != b._custom;
        }

        /// <summary>
        /// Implements the equality operator.
        /// </summary>
        /// <param name="a">The left operand.</param>
        /// <param name="b">The right operand.</param>
        /// <returns>True if both types are equal, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(ScriptType a, Type b)
        {
            return EqualityComparer<MemberInfo>.Default.Equals(a._managed, b);
        }

        /// <summary>
        /// Implements the non equality operator.
        /// </summary>
        /// <param name="a">The left operand.</param>
        /// <param name="b">The right operand.</param>
        /// <returns>True if both types are not equal, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(ScriptType a, Type b)
        {
            return !EqualityComparer<MemberInfo>.Default.Equals(a._managed, b);
        }

        /// <inheritdoc />
        public bool Equals(ScriptType other)
        {
            return _managed == other._managed && _custom == other._custom;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is ScriptType other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return ((_managed != null ? _managed.GetHashCode() : 0) * 397) ^ (_custom != null ? _custom.GetHashCode() : 0);
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            if (_managed != null)
                return _managed.FullName ?? string.Empty;
            if (_custom != null)
                return _custom.TypeName;
            return "<null>";
        }

        /// <summary>
        /// Determines whether the current type derives from the specified type.
        /// </summary>
        /// <param name="c">The type to compare with the current type. </param>
        /// <returns><see langword="true" /> if the current type derives from <paramref name="c" />; otherwise, <see langword="false" />. This method also returns <see langword="false" /> if <paramref name="c" /> and the current <see langword="Type" /> are equal.</returns>
        public bool IsSubclassOf(ScriptType c)
        {
            var type = this;
            if (type == c)
                return false;
            for (; type != Null; type = type.BaseType)
            {
                if (type == c)
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Determines whether the specified object is an instance of the current type.
        /// </summary>
        /// <param name="o">The object to compare with the current type. </param>
        /// <returns>
        /// <see langword="true" /> if the current <see langword="Type" /> is in the inheritance hierarchy of the object represented by <paramref name="o" />, or if the current type is an interface that <paramref name="o" /> implements. <see langword="false" /> if neither of these conditions is the case, if <paramref name="o" /> is <see langword="null" />, or if the current <see langword="Type" /> is an open generic type.</returns>
        public bool IsInstanceOfType(object o) => o != null && IsAssignableFrom(new ScriptType(o.GetType()));

        /// <summary>
        /// Determines whether an instance of a specified type can be assigned to an instance of the current type.
        /// </summary>
        /// <param name="c">The type to compare with the current type.</param>
        /// <returns>
        /// <c>true</c> if any of the following conditions is true:
        ///     <paramref name="c" /> and the current instance represent the same type.
        ///     <paramref name="c" /> is derived either directly or indirectly from the current instance. <paramref name="c" /> is derived directly from the current instance if it inherits from the current instance; <paramref name="c" /> is derived indirectly from the current instance if it inherits from a succession of one or more classes that inherit from the current instance. The current instance is an interface that <paramref name="c" /> implements.
        /// <c>false</c> if none of these conditions are true, or if <paramref name="c" /> is <see cref="Null"/>.</returns>
        public bool IsAssignableFrom(ScriptType c)
        {
            while (c != Null)
            {
                if (c == this)
                    return true;
                c = c.BaseType;
            }
            return false;
        }

        /// <summary>
        /// Determines whether the specified attribute was defined for this type.
        /// </summary>
        /// <param name="attributeType">The attribute type.</param>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns><c>true</c> if the specified type has attribute; otherwise, <c>false</c>.</returns>
        public bool HasAttribute(Type attributeType, bool inherit)
        {
            if (_managed != null)
                return Attribute.IsDefined(_managed, attributeType, inherit);
            if (_custom != null)
                return _custom.HasAttribute(attributeType, inherit);
            return false;
        }

        /// <summary>
        /// When overridden in a derived class, returns an array of all custom attributes applied to this member.
        /// </summary>
        /// <param name="inherit"><c>true</c> to search this member's inheritance chain to find the attributes; otherwise, <c>false</c>.</param>
        /// <returns>An array that contains all the custom attributes applied to this member, or an array with zero elements if no attributes are defined.</returns>
        public object[] GetAttributes(bool inherit)
        {
            if (_managed != null)
                return _managed.GetCustomAttributes(inherit);
            if (_custom != null)
                return _custom.GetAttributes(inherit);
            return Utils.GetEmptyArray<object>();
        }

        /// <summary>
        /// When overridden in a derived class, returns the type of the object encompassed or referred to by the current array, pointer or reference type.
        /// </summary>
        /// <returns>The type of the object encompassed or referred to by the current array, pointer, or reference type, or <see langword="null" /> if the current type is not an array or a pointer, or is not passed by reference, or represents a generic type or a type parameter in the definition of a generic type or generic method.</returns>
        public Type GetElementType()
        {
            if (_managed != null)
                return _managed.GetElementType();
            throw new NotImplementedException("TODO: Script.Type.GetElementType for custom types");
        }

        /// <summary>
        /// Returns an array of type objects that represent the type arguments of a closed generic type or the type parameters of a generic type definition.
        /// </summary>
        /// <returns>An array of <see cref="T:System.Type" /> objects that represent the type arguments of a generic type. Returns an empty array if the current type is not a generic type.</returns>
        public Type[] GetGenericArguments()
        {
            if (_managed != null)
                return _managed.GetGenericArguments();
            throw new NotImplementedException("TODO: Script.Type.GetGenericArguments for custom types");
        }

        /// <summary>
        /// Returns a type object that represents a generic type definition from which the current generic type can be constructed.
        /// </summary>
        /// <returns>A type object representing a generic type from which the current type can be constructed.</returns>
        public Type GetGenericTypeDefinition()
        {
            if (_managed != null)
                return _managed.GetGenericTypeDefinition();
            throw new NotImplementedException("TODO: Script.Type.GetGenericTypeDefinition for custom types");
        }

        /// <summary>
        /// Searches for the specified members of the specified member type, using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the members to get.</param>
        /// <param name="type">The value to search for.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero, to return an empty array.</param>
        /// <param name="managedMembers">The output array of members for C# types.</param>
        /// <returns>An array of member objects representing the public members with the specified name, if found; otherwise, an empty array.</returns>
        public ScriptMemberInfo[] GetMembers(string name, MemberTypes type, BindingFlags bindingAttr, out MemberInfo[] managedMembers)
        {
            if (_managed != null)
            {
                managedMembers = _managed.GetMember(name, type, bindingAttr);
                return Utils.GetEmptyArray<ScriptMemberInfo>();
            }
            managedMembers = null;
            if (_custom != null)
                return _custom.GetMembers(name, type, bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// Searches for the specified members of the specified member type, using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the members to get.</param>
        /// <param name="type">The value to search for.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero, to return an empty array.</param>
        /// <returns>An array of member objects representing the public members with the specified name, if found; otherwise, an empty array.</returns>
        public ScriptMemberInfo[] GetMembers(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            if (_managed != null)
            {
                var managedMembers = _managed.GetMember(name, type, bindingAttr);
                var members = new ScriptMemberInfo[managedMembers.Length];
                for (int i = 0; i < members.Length; i++)
                    members[i] = new ScriptMemberInfo(managedMembers[i]);
                return members;
            }
            if (_custom != null)
                return _custom.GetMembers(name, type, bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// Searches for the specified member of the specified member type, using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the member to get.</param>
        /// <param name="type">The value to search for.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero, to return an empty item.</param>
        /// <returns>The member objects representing the public member with the specified name, if found; otherwise, an empty array.</returns>
        public ScriptMemberInfo GetMember(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            ScriptMemberInfo result;
            if ((type & MemberTypes.Field) == MemberTypes.Field)
            {
                result = GetField(name, bindingAttr);
                if (result)
                    return result;
            }
            if ((type & MemberTypes.Property) == MemberTypes.Property)
            {
                result = GetProperty(name, bindingAttr);
                if (result)
                    return result;
            }
            if ((type & MemberTypes.Method) == MemberTypes.Method)
            {
                result = GetMethod(name, bindingAttr);
                if (result)
                    return result;
            }
            return GetMembers(name, type, bindingAttr).FirstOrDefault();
        }

        /// <summary>
        /// Returns all the public members of the current type.
        /// </summary>
        /// <param name="managedMembers">The output array of members for C# types.</param>
        /// <returns>An array of member objects representing all the public members of the current type.-or- An empty array of type member, if the current type does not have public members.</returns>
        public ScriptMemberInfo[] GetMembers(out MemberInfo[] managedMembers)
        {
            return GetMembers(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, out managedMembers);
        }

        /// <summary>
        /// Returns all the public members of the current type.
        /// </summary>
        /// <returns>An array of member objects representing all the public members of the current type.-or- An empty array of type member, if the current type does not have public members.</returns>
        public ScriptMemberInfo[] GetMembers()
        {
            return GetMembers(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
        }

        /// <summary>
        /// When overridden in a derived class, searches for the members defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty array.</param>
        /// <param name="managedMembers">The output array of members for C# types.</param>
        /// <returns>An array of member objects representing all members defined for the current type that match the specified binding constraints.-or- An empty array of type member, if no members are defined for the current type, or if none of the defined members match the binding constraints.</returns>
        public ScriptMemberInfo[] GetMembers(BindingFlags bindingAttr, out MemberInfo[] managedMembers)
        {
            if (_managed != null)
            {
                managedMembers = _managed.GetMembers(bindingAttr);
                return Utils.GetEmptyArray<ScriptMemberInfo>();
            }
            managedMembers = null;
            if (_custom != null)
                return _custom.GetMembers(bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// When overridden in a derived class, searches for the members defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty array.</param>
        /// <returns>An array of member objects representing all members defined for the current type that match the specified binding constraints.-or- An empty array of type member, if no members are defined for the current type, or if none of the defined members match the binding constraints.</returns>
        public ScriptMemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            if (_managed != null)
            {
                var managedMembers = _managed.GetMembers(bindingAttr);
                var members = new ScriptMemberInfo[managedMembers.Length];
                for (int i = 0; i < members.Length; i++)
                    members[i] = new ScriptMemberInfo(managedMembers[i]);
                return members;
            }
            if (_custom != null)
                return _custom.GetMembers(bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// When overridden in a derived class, searches for the fields defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty item.</param>
        /// <returns>The member object representing the field defined for the current type that matches the specified name. Returns <see cref="ScriptMemberInfo.Null"/> if failed.</returns>
        public ScriptMemberInfo[] GetFields(BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public)
        {
            if (_managed != null)
            {
                var managedMembers = _managed.GetFields(bindingAttr);
                var members = new ScriptMemberInfo[managedMembers.Length];
                for (int i = 0; i < members.Length; i++)
                    members[i] = new ScriptMemberInfo(managedMembers[i]);
                return members;
            }
            if (_custom != null)
                return _custom.GetFields(bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// When overridden in a derived class, searches for the field defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the members to get.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty item.</param>
        /// <returns>The member object representing the field defined for the current type that matches the specified name. Returns <see cref="ScriptMemberInfo.Null"/> if failed.</returns>
        public ScriptMemberInfo GetField(string name, BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public)
        {
            if (_managed != null)
            {
                var managedMember = _managed.GetField(name, bindingAttr);
                return new ScriptMemberInfo(managedMember);
            }
            if (_custom != null)
                return _custom.GetMembers(name, MemberTypes.Field, bindingAttr).FirstOrDefault();
            return ScriptMemberInfo.Null;
        }

        /// <summary>
        /// When overridden in a derived class, searches for the property defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty item.</param>
        /// <returns>The member object representing the property defined for the current type that matches the specified name. Returns <see cref="ScriptMemberInfo.Null"/> if failed.</returns>
        public ScriptMemberInfo[] GetProperties(BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public)
        {
            if (_managed != null)
            {
                var managedMembers = _managed.GetProperties(bindingAttr);
                var members = new ScriptMemberInfo[managedMembers.Length];
                for (int i = 0; i < members.Length; i++)
                    members[i] = new ScriptMemberInfo(managedMembers[i]);
                return members;
            }
            if (_custom != null)
                return _custom.GetProperties(bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// When overridden in a derived class, searches for the property defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the members to get.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty item.</param>
        /// <returns>The member object representing the property defined for the current type that matches the specified name. Returns <see cref="ScriptMemberInfo.Null"/> if failed.</returns>
        public ScriptMemberInfo GetProperty(string name, BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public)
        {
            if (_managed != null)
            {
                var managedMember = _managed.GetProperty(name, bindingAttr);
                return new ScriptMemberInfo(managedMember);
            }
            if (_custom != null)
                return _custom.GetMembers(name, MemberTypes.Property, bindingAttr).FirstOrDefault();
            return ScriptMemberInfo.Null;
        }

        /// <summary>
        /// When overridden in a derived class, searches for the method defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty item.</param>
        /// <returns>The member object representing the method defined for the current type that matches the specified name. Returns <see cref="ScriptMemberInfo.Null"/> if failed.</returns>
        public ScriptMemberInfo[] GetMethods(BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public)
        {
            if (_managed != null)
            {
                var managedMembers = _managed.GetMethods(bindingAttr);
                var members = new ScriptMemberInfo[managedMembers.Length];
                for (int i = 0; i < members.Length; i++)
                    members[i] = new ScriptMemberInfo(managedMembers[i]);
                return members;
            }
            if (_custom != null)
                return _custom.GetMethods(bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <summary>
        /// When overridden in a derived class, searches for the method defined for the current type using the specified binding constraints.
        /// </summary>
        /// <param name="name">The string containing the name of the members to get.</param>
        /// <param name="bindingAttr">A bitmask comprised of one or more <see cref="T:System.Reflection.BindingFlags" /> that specify how the search is conducted.-or- Zero (<see cref="F:System.Reflection.BindingFlags.Default" />), to return an empty item.</param>
        /// <returns>The member object representing the method defined for the current type that matches the specified name. Returns <see cref="ScriptMemberInfo.Null"/> if failed.</returns>
        public ScriptMemberInfo GetMethod(string name, BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public)
        {
            if (_managed != null)
            {
                var managedMember = _managed.GetMethod(name, bindingAttr);
                return new ScriptMemberInfo(managedMember);
            }
            if (_custom != null)
                return _custom.GetMembers(name, MemberTypes.Method, bindingAttr).FirstOrDefault();
            return ScriptMemberInfo.Null;
        }
    }

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
        /// Gets all the derived types from the given base type (excluding that type) within all the loaded assemblies.
        /// </summary>
        /// <param name="baseType">The base type.</param>
        /// <param name="result">The result collection. Elements will be added to it. Clear it before usage.</param>
        /// <param name="checkFunc">Additional callback used to check if the given type is valid. Returns true if add type, otherwise false.</param>
        void GetDerivedTypes(ScriptType baseType, List<ScriptType> result, Func<ScriptType, bool> checkFunc);
    }
}
