// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit enum value type properties.
    /// </summary>
    [CustomEditor(typeof(Enum)), DefaultEditor]
    public class EnumEditor : CustomEditor
    {
        /// <summary>
        /// The enum element.
        /// </summary>
        protected EnumElement element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var mode = EnumDisplayAttribute.FormatMode.Default;
            var attributes = Values.GetAttributes();
            if (attributes?.FirstOrDefault(x => x is EnumDisplayAttribute) is EnumDisplayAttribute enumDisplay)
            {
                mode = enumDisplay.Mode;
            }

            if (HasDifferentTypes)
            {
                // No support for different enum types
            }
            else
            {
                var enumType = Values.Type.Type != typeof(object) || Values[0] == null ? TypeUtils.GetType(Values.Type) : Values[0].GetType();
                element = layout.Enum(enumType, null, mode);
                element.ComboBox.ValueChanged += OnValueChanged;
            }
        }

        /// <summary>
        /// Called when value get changed. Allows to override default value setter logic.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            SetValue(element.ComboBox.EnumTypeValue);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // No support for different enum values
            }
            else
            {
                element.ComboBox.EnumTypeValue = Values[0];
            }
        }
    }
}
