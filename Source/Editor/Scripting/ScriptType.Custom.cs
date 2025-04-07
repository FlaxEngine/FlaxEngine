// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Reflection;
using FlaxEditor.Content;
using FlaxEngine;

namespace FlaxEditor.Scripting
{
    /// <summary>
    /// The implementation of the <see cref="IScriptType"/> for array of custom script type.
    /// </summary>
    [HideInEditor]
    internal sealed class ScriptTypeArray : IScriptType
    {
        /// <summary>
        /// The array element type.
        /// </summary>
        public readonly ScriptType ElementType;

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptTypeArray"/> class.
        /// </summary>
        /// <param name="elementType">Type of the array element.</param>
        public ScriptTypeArray(ScriptType elementType)
        {
            ElementType = elementType;
        }

        /// <inheritdoc />
        public string Name => ElementType + "[]";

        /// <inheritdoc />
        public string Namespace => ElementType.Namespace;

        /// <inheritdoc />
        public string TypeName
        {
            get
            {
                var result = ElementType + "[]";
                var nameSpace = ElementType.Namespace;
                if (nameSpace.Length != 0)
                    result = nameSpace + '.';
                return result;
            }
        }

        /// <inheritdoc />
        public bool IsPublic => ElementType.IsPublic;

        /// <inheritdoc />
        public bool IsAbstract => false;

        /// <inheritdoc />
        public bool IsSealed => true;

        /// <inheritdoc />
        public bool IsEnum => false;

        /// <inheritdoc />
        public bool IsClass => true;

        /// <inheritdoc />
        public bool IsInterface => false;

        /// <inheritdoc />
        public bool IsArray => true;

        /// <inheritdoc />
        public bool IsValueType => false;

        /// <inheritdoc />
        public bool IsGenericType => false;

        /// <inheritdoc />
        public bool IsReference => false;

        /// <inheritdoc />
        public bool IsPointer => false;

        /// <inheritdoc />
        public bool IsStatic => false;

        /// <inheritdoc />
        public bool CanCreateInstance => false;

        /// <inheritdoc />
        public ScriptType BaseType => new ScriptType(typeof(Array));

        /// <inheritdoc />
        public ContentItem ContentItem => null;

        /// <inheritdoc />
        public object CreateInstance()
        {
            return null;
        }

        /// <inheritdoc />
        public bool HasInterface(ScriptType c)
        {
            return false;
        }

        /// <inheritdoc />
        public bool HasAttribute(Type attributeType, bool inherit)
        {
            return false;
        }

        /// <inheritdoc />
        public object[] GetAttributes(bool inherit)
        {
            return Utils.GetEmptyArray<object>();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetMembers(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetFields(BindingFlags bindingAttr)
        {
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetProperties(BindingFlags bindingAttr)
        {
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetMethods(BindingFlags bindingAttr)
        {
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <inheritdoc />
        public void TrackLifetime(Action<ScriptType> disposing)
        {
            ElementType.TrackLifetime(disposing);
        }
    }
}
