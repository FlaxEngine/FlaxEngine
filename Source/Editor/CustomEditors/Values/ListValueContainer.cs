// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using FlaxEditor.Scripting;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Custom <see cref="ValueContainer"/> for <see cref="IList"/> (used for <see cref="Array"/> and <see cref="System.Collections.Generic.List{T}"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.ValueContainer" />
    public class ListValueContainer : ValueContainer
    {
        private readonly object[] _attributes;

        /// <summary>
        /// The index in the collection.
        /// </summary>
        public readonly int Index;

        /// <summary>
        /// Initializes a new instance of the <see cref="ListValueContainer"/> class.
        /// </summary>
        /// <param name="elementType">Type of the collection elements.</param>
        /// <param name="index">The index.</param>
        public ListValueContainer(ScriptType elementType, int index)
        : base(ScriptMemberInfo.Null, elementType)
        {
            Index = index;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ListValueContainer"/> class.
        /// </summary>
        /// <param name="elementType">Type of the collection elements.</param>
        /// <param name="index">The index.</param>
        /// <param name="values">The collection values.</param>
        /// <param name="attributes">The collection property attributes to inherit.</param>
        public ListValueContainer(ScriptType elementType, int index, ValueContainer values, object[] attributes = null)
        : this(elementType, index)
        {
            _attributes = attributes;

            Capacity = values.Count;
            for (int i = 0; i < values.Count; i++)
            {
                var v = (IList)values[i];
                Add(v[index]);
            }

            if (values.HasReferenceValue)
            {
                if (values.ReferenceValue is IList v && v.Count > index)
                {
                    _referenceValue = v[index];
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
                var v = (IList)instanceValues[i];
                this[i] = v[Index];
            }
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues, object value)
        {
            if (instanceValues == null || instanceValues.Count != Count)
                throw new ArgumentException();

            for (int i = 0; i < Count; i++)
            {
                var v = (IList)instanceValues[i];
                v[Index] = value;
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
                var v = (IList)instanceValues[i];
                var value = ((ListValueContainer)values)[i];
                v[Index] = value;
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
                var v = (IList)instanceValues[i];
                v[Index] = this[i];
            }
        }

        /// <inheritdoc />
        public override void RefreshReferenceValue(object instanceValue)
        {
            if (instanceValue is IList v)
            {
                _referenceValue = v[Index];
                _hasReferenceValue = true;
            }
        }

        /// <inheritdoc />
        public override object[] GetAttributes()
        {
            return _attributes ?? base.GetAttributes();
        }
    }
}
