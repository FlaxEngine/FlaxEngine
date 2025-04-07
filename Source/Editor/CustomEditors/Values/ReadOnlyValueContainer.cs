// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Custom <see cref="ValueContainer"/> for read-only values.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.ValueContainer" />
    [HideInEditor]
    public sealed class ReadOnlyValueContainer : ValueContainer
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ReadOnlyValueContainer"/> class.
        /// </summary>
        /// <param name="value">The initial value.</param>
        public ReadOnlyValueContainer(object value)
        : base(ScriptMemberInfo.Null, ScriptType.Object)
        {
            Add(value);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ReadOnlyValueContainer"/> class.
        /// </summary>
        /// <param name="type">The values type.</param>
        /// <param name="value">The initial value.</param>
        public ReadOnlyValueContainer(ScriptType type, object value)
        : base(ScriptMemberInfo.Null, type)
        {
            Add(value);
        }

        /// <inheritdoc />
        public override void Refresh(ValueContainer instanceValues)
        {
            // Not supported
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues, object value)
        {
            // Not supported
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues, ValueContainer values)
        {
            // Not supported
        }

        /// <inheritdoc />
        public override void Set(ValueContainer instanceValues)
        {
            // Not supported
        }

        /// <inheritdoc />
        public override void RefreshReferenceValue(object instanceValue)
        {
            // Not supported
        }
    }
}
