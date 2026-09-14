// Copyright (c) Wojciech Figat. All rights reserved.

using System;
#if FLAX_EDITOR
using System.Globalization;
using System.ComponentModel;
#endif

namespace FlaxEngine
{
    /// <summary>
    /// The scripting object reference with interface.
    /// </summary>
    /// <typeparam name="T">The type of the scripting interface.</typeparam>
#if FLAX_EDITOR
    [CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.ScriptingObjectInterfaceReferenceEditor))]
    [TypeConverter(typeof(TypeConverters.ScriptingObjectInterfaceReferenceConverter))]
#endif
    public struct ScriptingObjectInterfaceReference<T> : IComparable, IComparable<ScriptingObjectInterfaceReference<T>> where T : class
    {
        private Object _object;

        /// <summary>
        /// Gets or sets the referenced object that implements the interface.
        /// </summary>
        public Object Object
        {
            get => _object;
            set => _object = value != null && value is T ? value : null;
        }

        /// <summary>
        /// Gets or sets the referenced object that implements the interface.
        /// </summary>
        [NoSerialize]
        public T Interface
        {
            get => _object as T;
            set
            {
                var obj = value as Object;
                if (value == null || obj != null)
                    _object = obj;
                else
                    throw new InvalidCastException($"Cannot use object of type {value.GetType().FullName} for ScriptingObjectInterfaceReference<{typeof(T).FullName}>. It needs to inherit from {typeof(Object).FullName}.");
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference{T}"/> structure.
        /// </summary>
        /// <param name="obj">The object to link.</param>
        public ScriptingObjectInterfaceReference(Object obj)
        {
            Object = obj;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectInterfaceReference{T}"/> structure.
        /// </summary>
        /// <param name="interfaceObj">The interface object to link.</param>
        public ScriptingObjectInterfaceReference(T interfaceObj)
        {
            Interface = interfaceObj;
        }

        /// <summary>
        /// Implicit cast operator to typed interface.
        /// </summary>
        /// <param name="value">Reference</param>
        /// <returns>Interface</returns>
        public static explicit operator T(ScriptingObjectInterfaceReference<T> value)
        {
            return value._object as T;
        }

        /// <summary>
        /// Implicit cast operator from object to reference.
        /// </summary>
        /// <param name="obj">The object to link.</param>
        /// <returns>Reference</returns>
        public static explicit operator ScriptingObjectInterfaceReference<T>(T obj)
        {
            return new ScriptingObjectInterfaceReference<T>(obj);
        }

        /// <summary>
        /// Implicit cast operator to object.
        /// </summary>
        /// <param name="value">Reference</param>
        /// <returns>Object</returns>
        public static implicit operator Object(ScriptingObjectInterfaceReference<T> value)
        {
            return value._object;
        }

        /// <summary>
        /// Implicit cast operator from object to reference.
        /// </summary>
        /// <param name="obj">Object</param>
        /// <returns>Reference</returns>
        public static implicit operator ScriptingObjectInterfaceReference<T>(Object obj)
        {
            return new ScriptingObjectInterfaceReference<T>(obj);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return _object?.ToString() ?? "<null>";
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Object.GetUnmanagedPtr(_object).GetHashCode();
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is ScriptingObjectInterfaceReference<T> other)
                return CompareTo(other);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(ScriptingObjectInterfaceReference<T> other)
        {
            return Object.GetUnmanagedPtr(_object).CompareTo(Object.GetUnmanagedPtr(other._object));
        }
    }
}

#if FLAX_EDITOR
namespace FlaxEngine.TypeConverters
{
    internal class ScriptingObjectInterfaceReferenceConverter : TypeConverter
    {
        /// <inheritdoc />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
                return true;
            return base.CanConvertFrom(context, sourceType);
        }

        /// <inheritdoc />
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
                return false;
            return base.CanConvertTo(context, destinationType);
        }

        /// <inheritdoc />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string str && context is DummyTypeDescriptorContext internalContext)
            {
                var type = internalContext.CurrentType;
                Json.JsonSerializer.ParseID(str, out var id);
                var obj = Object.Find(ref id, type.GetGenericArguments()[0]);
                var objectField = type.GetField("_object", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic);
                value = Activator.CreateInstance(type);
                objectField.SetValue(value, obj);
                return value;
            }
            return base.ConvertFrom(context, culture, value);
        }

        /// <inheritdoc />
        public override unsafe object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                var objectField = value.GetType().GetField("_object", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic);
                var obj = objectField.GetValue(value) as Object;
                if (obj == null)
                    return string.Empty;
                var id = obj.ID;
                return Json.JsonSerializer.GetStringID(&id);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
#endif
