// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Editable object values.
    /// </summary>
    [HideInEditor]
    public class ValueContainer : List<object>
    {
        /// <summary>
        /// The has default value flag. Set if <see cref="_defaultValue"/> is valid and assigned.
        /// </summary>
        protected bool _hasDefaultValue;

        /// <summary>
        /// The default value used to show difference in the UI compared to the default object values. Used to revert modified properties.
        /// </summary>
        protected object _defaultValue;

        /// <summary>
        /// The has reference value flag. Set if <see cref="_referenceValue"/> is valid and assigned.
        /// </summary>
        protected bool _hasReferenceValue;

        /// <summary>
        /// The reference value used to show difference in the UI compared to the other object. Used by the prefabs system.
        /// </summary>
        protected object _referenceValue;

        /// <summary>
        /// The values source information from reflection. Used to update values.
        /// </summary>
        public readonly ScriptMemberInfo Info;

        /// <summary>
        /// Gets the values type.
        /// </summary>
        public ScriptType Type { get; private set; }

        /// <summary>
        /// Gets a value indicating whether single object is selected.
        /// </summary>
        public bool IsSingleObject => Count == 1;

        /// <summary>
        /// Gets a value indicating whether selected objects are different values.
        /// </summary>
        public bool HasDifferentValues
        {
            get
            {
                for (int i = 1; i < Count; i++)
                {
                    if (!Equals(this[0], this[i]))
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether selected objects are different types.
        /// </summary>
        public bool HasDifferentTypes
        {
            get
            {
                if (Count < 2)
                    return false;
                var theFirstType = TypeUtils.GetObjectType(this[0]);
                for (int i = 1; i < Count; i++)
                {
                    if (theFirstType != TypeUtils.GetObjectType(this[i]))
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether any value in the collection is null. Returns false if collection is empty.
        /// </summary>
        public bool HasNull
        {
            get
            {
                for (int i = 0; i < Count; i++)
                {
                    if (this[i] == null)
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether all values in the collection are null. Returns true if collection is empty.
        /// </summary>
        public bool IsNull
        {
            get
            {
                for (int i = 0; i < Count; i++)
                {
                    if (this[i] != null)
                        return false;
                }
                return true;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this any value in the collection is of value type (eg. a structure, not a class type). Returns false if collection is empty.
        /// </summary>
        public bool HasValueType
        {
            get
            {
                for (int i = 0; i < Count; i++)
                {
                    if (this[i] != null && TypeUtils.GetObjectType(this[i]).IsValueType)
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this values container type is array.
        /// </summary>
        public bool IsArray => Type != ScriptType.Null && Type.IsArray;

        /// <summary>
        /// True if member or type has <see cref="System.ObsoleteAttribute"/> that marks it as obsolete.
        /// </summary>
        public bool IsObsolete { get; }

        /// <summary>
        /// Gets the values types array (without duplicates).
        /// </summary>
        public ScriptType[] ValuesTypes
        {
            get
            {
                if (Count == 1)
                    return new[] { TypeUtils.GetObjectType(this[0]) };
                return ConvertAll(TypeUtils.GetObjectType).Distinct().ToArray();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueContainer"/> class.
        /// </summary>
        /// <param name="info">The member info.</param>
        public ValueContainer(ScriptMemberInfo info)
        {
            Info = info;
            Type = Info.ValueType;
            IsObsolete = Info.HasAttribute(typeof(ObsoleteAttribute), true);
        }

        /// <summary>
        /// Gets a value indicating whether this instance has reference value assigned (see <see cref="ReferenceValue"/>).
        /// </summary>
        public bool HasReferenceValue => _hasReferenceValue;

        /// <summary>
        /// Gets the reference value used to show difference in the UI compared to the other object. Used by the prefabs system.
        /// </summary>
        public object ReferenceValue => _referenceValue;

        /// <summary>
        /// Gets a value indicating whether this instance has reference value and the any of the values in the contains is modified (compared to the reference).
        /// </summary>
        /// <remarks>
        /// For prefabs system it means that object property has been modified compared to the prefab value.
        /// </remarks>
        public bool IsReferenceValueModified
        {
            get
            {
                if (_hasReferenceValue)
                {
                    if (_referenceValue is SceneObject referenceSceneObject && referenceSceneObject && referenceSceneObject.HasPrefabLink)
                    {
                        for (int i = 0; i < Count; i++)
                        {
                            if ((SceneObject)this[i] == referenceSceneObject)
                                continue;

                            if (this[i] == null || (this[i] is SceneObject valueSceneObject && valueSceneObject && valueSceneObject.PrefabObjectID != referenceSceneObject.PrefabObjectID))
                                return true;
                        }
                    }
                    else
                    {
                        for (int i = 0; i < Count; i++)
                        {
                            if (!ValueEquals(this[i], _referenceValue))
                                return true;
                        }
                    }
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this instance has default value assigned (see <see cref="DefaultValue"/>).
        /// </summary>
        public bool HasDefaultValue => _hasDefaultValue;

        /// <summary>
        /// Gets the default value used to show difference in the UI compared to the default value object. Used to revert modified properties.
        /// </summary>
        public object DefaultValue => _defaultValue;

        /// <summary>
        /// Gets a value indicating whether this instance has default value and the any of the values in the contains is modified (compared to the reference).
        /// </summary>
        public bool IsDefaultValueModified
        {
            get
            {
                if (_hasDefaultValue)
                {
                    for (int i = 0; i < Count; i++)
                    {
                        if (!ValueEquals(this[i], _defaultValue))
                            return true;
                    }
                }
                return false;
            }
        }

        private static bool ValueEquals(object objA, object objB)
        {
            // Special case for String (null string is kind of equal to empty string from the user perspective)
            if (objA == null && objB is string objBStr && objBStr.Length == 0)
                return true;

            return FlaxEngine.Json.JsonSerializer.ValueEquals(objA, objB);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueContainer"/> class.
        /// </summary>
        /// <param name="info">The member info.</param>
        /// <param name="instanceValues">The parent values.</param>
        public ValueContainer(ScriptMemberInfo info, ValueContainer instanceValues)
        : this(info)
        {
            Capacity = instanceValues.Count;

            for (int i = 0; i < instanceValues.Count; i++)
            {
                Add(Info.GetValue(instanceValues[i]));
            }

            if (instanceValues._hasDefaultValue)
            {
                _defaultValue = Info.GetValue(instanceValues._defaultValue);
                _hasDefaultValue = true;
            }
            else
            {
                var defaultValueAttribute = Info.GetAttribute<DefaultValueAttribute>();
                if (defaultValueAttribute != null)
                {
                    _defaultValue = defaultValueAttribute.Value;
                    _hasDefaultValue = true;

                    if (_defaultValue != null && _defaultValue.GetType() != Type.Type)
                    {
                        // Workaround for DefaultValueAttribute that doesn't support certain value types storage
                        if (Type.Type == typeof(sbyte))
                            _defaultValue = Convert.ToSByte(_defaultValue);
                        else if (Type.Type == typeof(short))
                            _defaultValue = Convert.ToInt16(_defaultValue);
                        else if (Type.Type == typeof(ushort))
                            _defaultValue = Convert.ToUInt16(_defaultValue);
                        else if (Type.Type == typeof(uint))
                            _defaultValue = Convert.ToUInt32(_defaultValue);
                        else if (Type.Type == typeof(ulong))
                            _defaultValue = Convert.ToUInt64(_defaultValue);
                        else if (Type.Type == typeof(long))
                            _defaultValue = Convert.ToInt64(_defaultValue);
                    }
                }
            }
            if (instanceValues._hasReferenceValue)
            {
                // If the reference value is set for the parent values but it's null object then skip it
                if (instanceValues._referenceValue == null && !instanceValues.Type.IsValueType)
                    return;

                try
                {
                    _referenceValue = Info.GetValue(instanceValues._referenceValue);
                    _hasReferenceValue = true;
                }
                catch
                {
                    // Ignore error if reference value has different type or is invalid for this member
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueContainer"/> class.
        /// </summary>
        /// <param name="customType">The target custom type of the container values. Used to override the data.</param>
        /// <param name="other">The other values container to clone.</param>
        public ValueContainer(ScriptType customType, ValueContainer other)
        {
            Capacity = other.Capacity;
            AddRange(other);
            Info = other.Info;
            Type = customType;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueContainer"/> class.
        /// </summary>
        /// <param name="info">The member info.</param>
        /// <param name="type">The type.</param>
        protected ValueContainer(ScriptMemberInfo info, ScriptType type)
        {
            Info = info;
            Type = type;
        }

        /// <summary>
        /// Sets the type. Use with caution.
        /// </summary>
        /// <param name="type">The type.</param>
        public void SetType(ScriptType type)
        {
            Type = type;
        }

        /// <summary>
        /// Gets the custom attributes defined for the values source member.
        /// </summary>
        /// <returns>The attributes objects array.</returns>
        public virtual object[] GetAttributes()
        {
            return Info.GetAttributes(true);
        }

        /// <summary>
        /// Refreshes the specified instance values.
        /// </summary>
        /// <param name="instanceValues">The parent values.</param>
        public virtual void Refresh(ValueContainer instanceValues)
        {
            if (instanceValues == null)
                throw new ArgumentNullException();
            if (instanceValues.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
                this[i] = Info.GetValue(instanceValues[i]);
        }

        /// <summary>
        /// Sets the specified instance values. Refreshes this values container.
        /// </summary>
        /// <param name="instanceValues">The parent values.</param>
        /// <param name="value">The value.</param>
        public virtual void Set(ValueContainer instanceValues, object value)
        {
            if (instanceValues == null)
                throw new ArgumentNullException();
            if (instanceValues.Count != Count)
                throw new ArgumentException();

            if (value is IList l && l.Count == Count && Count > 1)
            {
                for (int i = 0; i < Count; i++)
                {
                    Info.SetValue(instanceValues[i], l[i]);
                    this[i] = Info.GetValue(instanceValues[i]);
                }
            }
            else
            {
                for (int i = 0; i < Count; i++)
                {
                    Info.SetValue(instanceValues[i], value);
                    this[i] = Info.GetValue(instanceValues[i]);
                }
            }
        }

        /// <summary>
        /// Sets the specified instance values. Refreshes this values container.
        /// </summary>
        /// <param name="instanceValues">The parent values.</param>
        /// <param name="values">The other values to set this container to.</param>
        public virtual void Set(ValueContainer instanceValues, ValueContainer values)
        {
            if (instanceValues == null || values == null)
                throw new ArgumentNullException();
            if (instanceValues.Count != Count || values.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                Info.SetValue(instanceValues[i], values[i]);
                this[i] = Info.GetValue(instanceValues[i]);
            }
        }

        /// <summary>
        /// Sets the specified instance values with the container values.
        /// </summary>
        /// <param name="instanceValues">The parent values.</param>
        public virtual void Set(ValueContainer instanceValues)
        {
            if (instanceValues == null)
                throw new ArgumentNullException();
            if (instanceValues.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                Info.SetValue(instanceValues[i], this[i]);
            }
        }

        /// <summary>
        /// Sets the default value of the container.
        /// </summary>
        /// <param name="value">The value.</param>
        public virtual void SetDefaultValue(object value)
        {
            _defaultValue = value;
            _hasDefaultValue = true;
        }

        /// <summary>
        /// Refreshes the default value of the container.
        /// </summary>
        /// <param name="instanceValue">The parent value.</param>
        public virtual void RefreshDefaultValue(object instanceValue)
        {
            _defaultValue = Info.GetValue(instanceValue);
            _hasDefaultValue = true;
        }

        /// <summary>
        /// Clears the default value of the container.
        /// </summary>
        public virtual void ClearDefaultValue()
        {
            _defaultValue = null;
            _hasDefaultValue = false;
        }

        /// <summary>
        /// Sets the reference value of the container.
        /// </summary>
        /// <param name="value">The value.</param>
        public virtual void SetReferenceValue(object value)
        {
            _referenceValue = value;
            _hasReferenceValue = true;
        }

        /// <summary>
        /// Refreshes the reference value of the container.
        /// </summary>
        /// <param name="instanceValue">The parent value.</param>
        public virtual void RefreshReferenceValue(object instanceValue)
        {
            _referenceValue = Info.GetValue(instanceValue);
            _hasReferenceValue = true;
        }

        /// <summary>
        /// Clears the reference value of the container.
        /// </summary>
        public virtual void ClearReferenceValue()
        {
            _referenceValue = null;
            _hasReferenceValue = false;
        }
    }
}
