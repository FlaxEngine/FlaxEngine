// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Custom <see cref="ValueContainer"/> for <see cref="IDictionary"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.ValueContainer" />
    [HideInEditor]
    public class DictionaryValueContainer : ValueContainer
    {
        /// <summary>
        /// The key in the collection.
        /// </summary>
        public readonly object Key;

        /// <summary>
        /// Initializes a new instance of the <see cref="DictionaryValueContainer"/> class.
        /// </summary>
        /// <param name="elementType">Type of the collection elements.</param>
        /// <param name="key">The key.</param>
        public DictionaryValueContainer(ScriptType elementType, object key)
        : base(ScriptMemberInfo.Null, elementType)
        {
            Key = key;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DictionaryValueContainer"/> class.
        /// </summary>
        /// <param name="elementType">Type of the collection elements.</param>
        /// <param name="key">The key.</param>
        /// <param name="values">The collection values.</param>
        public DictionaryValueContainer(ScriptType elementType, object key, ValueContainer values)
        : this(elementType, key)
        {
            Capacity = values.Count;
            for (int i = 0; i < values.Count; i++)
            {
                var v = (IDictionary)values[i];
                Add(v[Key]);
            }

            if (values.HasReferenceValue)
            {
                var v = (IDictionary)values.ReferenceValue;
                if (v.Contains(key))
                {
                    _referenceValue = v[key];
                    _hasReferenceValue = true;
                }
            }
        }

        /// <inheritdoc />
        public override void Refresh(ValueContainer instanceValues)
        {
            if (instanceValues == null || instanceValues.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                var v = (IDictionary)instanceValues[i];
                this[i] = v[Key];
            }
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues, object value)
        {
            if (instanceValues == null || instanceValues.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                var v = (IDictionary)instanceValues[i];
                v[Key] = value;
                this[i] = value;
            }
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues, ValueContainer values)
        {
            if (instanceValues == null || instanceValues.Count != Count)
                throw new ArgumentException();
            if (values == null || values.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                var v = (IDictionary)instanceValues[i];
                var value = ((DictionaryValueContainer)values)[i];
                v[Key] = value;
                this[i] = value;
            }
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues)
        {
            if (instanceValues == null || instanceValues.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                var v = (IDictionary)instanceValues[i];
                v[Key] = this[i];
            }
        }

        /// <inheritdoc />
        public override void RefreshReferenceValue(object instanceValue)
        {
            if (instanceValue is IDictionary v)
            {
                _referenceValue = v[Key];
                _hasReferenceValue = true;
            }
        }
    }
}
