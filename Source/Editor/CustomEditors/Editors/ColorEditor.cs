// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.Input;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Color value type properties.
    /// </summary>
    [CustomEditor(typeof(Color)), DefaultEditor]
    public sealed class ColorEditor : CustomEditor
    {
        private CustomElement<ColorValueBox> element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            element = layout.Custom<ColorValueBox>();
            element.CustomControl.ValueChanged += OnValueChanged;
        }

        private void OnValueChanged()
        {
            var token = element.CustomControl.IsSliding ? this : null;
            SetValue(element.CustomControl.Value, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values on ColorValueBox
            }
            else
            {
                var value = Values[0];
                if (value is Color asColor)
                    element.CustomControl.Value = asColor;
                else if (value is Color32 asColor32)
                    element.CustomControl.Value = asColor32;
                else if (value is Float4 asFloat4)
                    element.CustomControl.Value = asFloat4;
                else if (value is Double4 asDouble4)
                    element.CustomControl.Value = (Float4)asDouble4;
                else if (value is Vector4 asVector4)
                    element.CustomControl.Value = asVector4;
            }
        }
    }
}
