// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    /// <summary>
    /// Interface for control references access.
    /// </summary>
    public interface IControlReference
    {
        /// <summary>
        /// Gets or sets the reference to <see cref="FlaxEngine.UIControl"/> actor.
        /// </summary>
        public UIControl UIControl { get; set; }

        /// <summary>
        /// Gets the type of the control the interface uses.
        /// </summary>
        public Type ControlType { get; }

        /// <summary>
        /// Sets control ref by force - used during loading when <see cref="UIControl.Control"/> is not loaded yet.
        /// </summary>
        /// <param name="control">The reference.</param>
        internal void Load(UIControl control);
    }

    /// <summary>
    /// UI Control reference utility. References UI Control actor with a typed control type.
    /// </summary>
    /// <typeparam name="T">Type of the UI control object.</typeparam>
#if FLAX_EDITOR
    [TypeConverter(typeof(TypeConverters.ControlReferenceConverter))]
#endif
    public struct ControlReference<T> : IControlReference, IComparable, IComparable<ControlReference<T>>, IEquatable<ControlReference<T>> where T : Control
    {
        private UIControl _uiControl;

        /// <summary>
        /// Gets the typed UI control object owned by the referenced <see cref="FlaxEngine.UIControl"/> actor.
        /// </summary>
        [HideInEditor]
        public T Control
        {
            get
            {
                var control = _uiControl?.Control;
                if (control == null)
                    return null;
                if (control is T t)
                    return t;
                Debug.Write(LogType.Warning, $"Trying to get Control from ControlReference but UIControl.Control is not correct type. It should be {typeof(T)} but is {control.GetType()}.");
                return null;
            }
        }

        /// <inheritdoc />
        public UIControl UIControl
        {
            get => _uiControl;
            set
            {
                var control = value?.Control;
                if (value == null)
                {
                    _uiControl = null;
                }
                else if (control is T)
                {
                    _uiControl = value;
                }
                else
                {
                    Debug.Write(LogType.Warning, $"Trying to set UIControl but UIControl.Control is not the correct type. It should be {typeof(T)} but is {control.GetType()}.");
                }
            }
        }

        /// <inheritdoc />
        public Type ControlType => typeof(T);

        /// <inheritdoc />
        public void Load(UIControl value)
        {
            _uiControl = value;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return _uiControl?.ToString() ?? "null";
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return _uiControl?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is IControlReference other)
                return Json.JsonSerializer.SceneObjectEquals(_uiControl, other.UIControl) ? 0 : 1;
            return 1;
        }

        /// <inheritdoc />
        public int CompareTo(ControlReference<T> other)
        {
            return Json.JsonSerializer.SceneObjectEquals(_uiControl, other._uiControl) ? 0 : 1;
        }

        /// <inheritdoc />
        public bool Equals(ControlReference<T> other)
        {
            return Json.JsonSerializer.SceneObjectEquals(_uiControl, other._uiControl);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is ControlReference<T> other && Json.JsonSerializer.SceneObjectEquals(_uiControl, other._uiControl);
        }

        /// <summary>
        /// The implicit operator for the Control.
        /// </summary>
        /// <param name="reference">The control reference.</param>
        /// <returns>The control object.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator T(ControlReference<T> reference) => reference.Control;

        /// <summary>
        /// The implicit operator for the UIControl.
        /// </summary>
        /// <param name="reference">The control reference.</param>
        /// <returns>The control actor.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator UIControl(ControlReference<T> reference) => reference.UIControl;

        /// <summary>
        /// Checks if the object exists (reference is not null and the unmanaged object pointer is valid).
        /// </summary>
        /// <param name="obj">The object to check.</param>
        /// <returns>True if object is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(ControlReference<T> obj) => obj._uiControl;

        /// <summary>
        /// Checks whether the two objects are equal.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(ControlReference<T> left, ControlReference<T> right) => Json.JsonSerializer.SceneObjectEquals(left._uiControl, right._uiControl);

        /// <summary>
        /// Checks whether the two objects are not equal.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(ControlReference<T> left, ControlReference<T> right) => !Json.JsonSerializer.SceneObjectEquals(left._uiControl, right._uiControl);
    }
}

#if FLAX_EDITOR
namespace FlaxEngine.TypeConverters
{
    internal class ControlReferenceConverter : TypeConverter
    {
        /// <inheritdoc />
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (value is string valueStr)
            {
                var result = Activator.CreateInstance(destinationType);
                if (result is IControlReference control)
                {
                    Json.JsonSerializer.ParseID(valueStr, out var id);
                    control.Load(Object.Find<UIControl>(ref id));
                }
                return result;
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <inheritdoc />
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType.Name.StartsWith("ControlReference", StringComparison.Ordinal))
                return true;
            return base.CanConvertTo(context, destinationType);
        }
    }
}
#endif
